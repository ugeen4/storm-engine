#include "lifecycle_diagnostics_service.hpp"

#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>

#include <spdlog/spdlog.h>

#include "storm/fs.h"
#include "v_file_service.h"
#include "spdlog_sinks/syncable_sink.hpp"
#include "watermark.hpp"

#if defined(_UNICODE) && defined(_WIN32)
#include <tchar.h>
#define sentry_options_set_database_path sentry_options_set_database_pathw
#define sentry_options_set_handler_path sentry_options_set_handler_pathw
#define sentry_options_add_attachment sentry_options_add_attachmentw
#else
#include <cstdlib>
#define _T(x) x
#define _tsystem std::system
#endif

#ifdef _WIN32
#include "logging.hpp"
#include "seh_extractor.hpp"
#endif

namespace
{
auto& getExecutableDir()
{
    static const auto executableDir = std::filesystem::path{std::filesystem::u8path(fio->_GetExecutableDirectory())};
    return executableDir;
}
auto &getLogsArchive()
{
    static const auto logsArchive = fs::GetLogsPath().replace_extension(".7z");
    return logsArchive;
}

#ifdef _WIN32 // FIX_LINUX 7za.exe
auto assembleArchiveCmd()
{
    constexpr auto archiverBin = "7za.exe";
    return _T("call \"") + (getExecutableDir() / archiverBin).native() + _T("\" a \"\\\\?\\") +
           getLogsArchive().native() + _T("\" \"\\\\?\\") + fs::GetLogsPath().native() + _T("\"");
}
#endif

}

namespace storm::diag
{

class LoggingService final
{
  public:
    ~LoggingService()
    {
        terminate();
    }

    void initialize()
    {
        if (terminate_)
        {
            terminate_ = false;

            static auto terminate_handler = std::get_terminate();
            std::set_terminate([] {
                terminate_handler();
            });

            create_directories(fs::GetLogsPath());

            std::thread worker{[this] { loggingThread(); }};
            worker.detach();
        }
    }

    void terminate()
    {
        using namespace std::chrono_literals;

        if (!terminate_)
        {
            terminate_ = true;
            {
                std::lock_guard lock(mtx_);
                flushRequested_ = true;
            }
            cv_.notify_one();

            std::unique_lock lock(mtx_);
            cv_.wait_for(lock, 5s, [this] { return !flushRequested_; });
            spdlog::shutdown();
        }
    }

    void flushAsync()
    {
        {
            std::lock_guard lock(mtx_);
            flushRequested_ = true;
        }
        cv_.notify_one();
    }

  private:
    std::mutex mtx_;
    std::condition_variable cv_;
    bool flushRequested_{false};
    std::atomic_bool terminate_{true};

    void flushAll(const bool terminate) const
    {
        spdlog::apply_all([terminate](std::shared_ptr<spdlog::logger> l) {
            l->flush();

            if (terminate)
            {
                for (auto &sink : l->sinks())
                {
                    if (const auto syncable_sink = std::dynamic_pointer_cast<logging::sinks::syncable_sink>(sink))
                    {
                        syncable_sink->terminate_immediately();
                    }
                }
            }
        });
    }

    void loggingThread()
    {
        while (!terminate_)
        {
            std::unique_lock lock(mtx_);
            cv_.wait(lock, [this] { return flushRequested_; });

            flushAll(terminate_);

            flushRequested_ = false;
            cv_.notify_one();
        }
    }
};

LifecycleDiagnosticsService::LifecycleDiagnosticsService() : loggingService_(std::make_unique<LoggingService>())
{
}

LifecycleDiagnosticsService::~LifecycleDiagnosticsService()
{
    terminate();
}

LifecycleDiagnosticsService::Guard LifecycleDiagnosticsService::initialize(const bool enableCrashReports)
{
    loggingService_->initialize();
#if (! defined __LCC__)
    if (!initialized_)
    {
        // TODO: make this crossplatform
        auto *options = sentry_options_new();
        sentry_options_set_dsn(options, "https://1798a1bcfb654cbd8ce157b381964525@o572138.ingest.sentry.io/5721165");
        sentry_options_set_release(options, STORM_BUILD_WATERMARK_STRING);
        sentry_options_set_database_path(options, (fs::GetStashPath() / "sentry-db").c_str());
#ifdef _WIN32
        sentry_options_set_handler_path(options, (getExecutableDir() / "crashpad_handler.exe").c_str());
#else
        sentry_options_set_handler_path(options, (getExecutableDir() / "crashpad_handler").c_str());
#endif
        sentry_options_add_attachment(options, getLogsArchive().c_str());
        sentry_options_set_before_send(options, beforeCrash, this);
        sentry_options_set_system_crash_reporter_enabled(options, enableCrashReports);

        initialized_ = sentry_init(options) == 0;
    }
#endif
    return Guard(*this);
}

void LifecycleDiagnosticsService::terminate() const
{
    loggingService_->terminate();
#if (! defined __LCC__)
    if (initialized_)
    {
        sentry_close();
    }
#endif    
}

void LifecycleDiagnosticsService::notifyAfterRun() const
{
    static auto latestFlushTimePoint = std::chrono::steady_clock::now();

    if (const auto now = std::chrono::steady_clock::now();
        std::chrono::duration_cast<std::chrono::seconds>(now - latestFlushTimePoint) >= getLoggingFlushPeriod())
    {
        loggingService_->flushAsync();
        latestFlushTimePoint = now;
    }
}

void LifecycleDiagnosticsService::setCrashInfoCollector(crash_info_collector f)
{
    collectCrashInfo_ = std::move(f);
}

#if (! defined __LCC__)
sentry_value_t LifecycleDiagnosticsService::beforeCrash(const sentry_value_t event, void * hint, void *closure)
{
    const auto *self = static_cast<LifecycleDiagnosticsService *>(closure);

    // collect engine data
    if (self->collectCrashInfo_)
    {
        self->collectCrashInfo_();
    }

#ifdef _WIN32
    // collect exception data
    if (hint != nullptr)
    {
        if(const seh_extractor seh(static_cast<EXCEPTION_POINTERS *>(hint)); seh.is_abnormal())
        {
            static auto logger = logging::getOrCreateLogger("exceptions");
            logger->set_pattern("%v");
            seh.sink([](const char *msg) { logger->trace(msg); });
        }
    }
#endif
    
    // terminate logging
    self->loggingService_->terminate();

#ifdef _WIN32 // FIX_LINUX 7za.exe
    // archive logs for sentry backend
    _tsystem(assembleArchiveCmd().c_str());
#endif

    return event;
}
#endif
} // namespace storm::diag
