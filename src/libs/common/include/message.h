#pragma once

#include <cstdarg>
#include <cstring>
#include <variant>

#include "c_vector.h"
#include "entity.h"

class VDATA;

namespace storm
{
#if (! defined __LCC__)
//using MessageParam = std::variant<uint8_t, uint16_t, uint32_t, int32_t, float, double, ATTRIBUTES *, entid_t, VDATA *,
                                  CVECTOR, std::string>;
#else
using MessageParam = std::variant<uint8_t, uint16_t, uint32_t, int32_t, float, double, ATTRIBUTES *, entid_t, VDATA *,
                                  std::string>;
#endif

namespace detail {

template<typename T>
MessageParam convertMessageParam(T value)
{
    return MessageParam(value);
}

template<>
inline MessageParam convertMessageParam(const bool value)
{
    return {value ? 1 : 0};
}

// Convert uint32_t to int32_t to prevent conversion issues
template<>
inline MessageParam convertMessageParam(const uint32_t value)
{
    return MessageParam(static_cast<int32_t>(value));
}

} // namespace detail

} // namespace storm

class MESSAGE final
{
  public:
    void Move2Start()
    {
        index = 0;
    };

    uint8_t Byte()
    {
        ValidateFormat('b');
        return std::get<uint8_t>(params_[index - 1]);
    }

    uint16_t Word()
    {
        ValidateFormat('w');
        return std::get<uint16_t>(params_[index - 1]);
    }

    int32_t Long()
    {
        ValidateFormat('l');
        return std::get<int32_t>(params_[index - 1]);
    }

    int32_t Dword()
    {
        ValidateFormat('u');
        return static_cast<int32_t>(std::get<uint32_t>(params_[index - 1]));
    }

    float Float()
    {
        ValidateFormat('f');
        return std::get<float>(params_[index - 1]);
    }

    double Double()
    {
        ValidateFormat('d');
        return std::get<double>(params_[index - 1]);
    }

    uintptr_t Pointer()
    {
        ValidateFormat('p');
        return std::get<uintptr_t>(params_[index - 1]);
    }

    ATTRIBUTES *AttributePointer()
    {
        ValidateFormat('a');
        return std::get<ATTRIBUTES *>(params_[index - 1]);
    }

    entid_t EntityID()
    {
        ValidateFormat('i');
        return std::get<entid_t>(params_[index - 1]);
    }

    VDATA *ScriptVariablePointer()
    {
        ValidateFormat('e');
        return std::get<VDATA *>(params_[index - 1]);
    }

#if (! defined __LCC__)
    CVECTOR CVector()
    {
        ValidateFormat('c');
        return std::get<CVECTOR>(params_[index - 1]);
    }
#endif

    const std::string &String()
    {
        ValidateFormat('s');
        return std::get<std::string>(params_[index - 1]);
    }

    bool Set(int32_t value)
    {
        ValidateFormat('l');
        params_[index - 1] = value;
        return true;
    }

    bool Set(uintptr_t value)
    {
        ValidateFormat('p');
        params_[index - 1] = value;
        return true;
    }

    bool Set(float value)
    {
        ValidateFormat('f');
        params_[index - 1] = value;
        return true;
    }

    bool Set(std::string value)
    {
        ValidateFormat('s');
        params_[index - 1] = std::move(value);
        return true;
    }

    bool SetEntity(entid_t value)
    {
        ValidateFormat('i');
        params_[index - 1] = value;
        return true;
    }

    bool Set(VDATA *value)
    {
        ValidateFormat('e');
        params_[index - 1] = value;
        return true;
    }

    bool Set(ATTRIBUTES *value)
    {
        ValidateFormat('a');
        params_[index - 1] = value;
        return true;
    }

    void ValidateFormat(char c)
    {
        if (format_.empty())
            throw std::runtime_error("Read from empty message");
        if (format_[index] != c)
            throw std::runtime_error("Incorrect message data");
        index++;
    }

    void Reset(const std::string_view &format)
    {
        format_ = format;
        params_.resize(format_.size());
        index = 0;
    }

    template<typename... Args>
    void Reset(const std::string_view &format, Args... args)
    {
        assert(format.size() == sizeof...(args));
        index = 0;
        format_ = format;
        params_ = {storm::detail::convertMessageParam(args)... };
    }

    void ResetVA(const std::string_view &format, va_list &args)
    {
        index = 0;
        format_ = format;
        params_.resize(format_.size());
        std::transform(format_.begin(), format_.end(), params_.begin(),
                       [&](const char c) { return GetParamValue(c, args); });
    }

    char GetCurrentFormatType()
    {
        return format_[index];
    };

    const char *StringPointer()
    {
        return String().c_str();
    }

    [[nodiscard]] std::string_view GetFormat() const
    {
        return format_;
    }

  private:
    static storm::MessageParam GetParamValue(const char c, va_list &args)
    {
        switch (c)
        {
        case 'b':
            return va_arg(args, uint8_t);
        case 'w':
            return va_arg(args, uint16_t);
        case 'l':
            return va_arg(args, int32_t);
        case 'u':
            return va_arg(args, uint32_t);
        case 'f':
            return static_cast<float>(va_arg(args, double));
        case 'd':
            return va_arg(args, double);
        case 'p':
            return va_arg(args, uintptr_t);
        case 'a':
            return va_arg(args, ATTRIBUTES *);
        case 'i':
            return va_arg(args, entid_t);
        case 'e':
            return va_arg(args, VDATA *);
#if (! defined __LCC__)            
        case 'c':
            return va_arg(args, CVECTOR);
#endif            
        case 's': {
            char *ptr = va_arg(args, char *);
            return std::string(ptr);
        }
        default:
            throw std::runtime_error(fmt::format("Unknown message format: '{}'", c));
        }
    }

    std::string format_;
    std::vector<storm::MessageParam> params_;
    int32_t index{};
};
