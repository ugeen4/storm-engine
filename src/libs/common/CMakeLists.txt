if (NOT WIN32)
set(SYSTEM_DEPS "libsafec")
endif()

STORM_SETUP(
    TARGET_NAME common
    TYPE library
    DEPENDENCIES shared_headers util ${Spdlog_LIBRARY} directx ${SYSTEM_DEPS}
)