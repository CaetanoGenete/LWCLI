#ifndef LWCLI_INCLUDE_LWCLI_CAST_STRING_HPP
#define LWCLI_INCLUDE_LWCLI_CAST_STRING_HPP

#include <string>
#include <concepts>

namespace lwcli
{

    template<class Type>
    struct cast;

    /* String casts ------------------------------------------------------------------------------------------------- */

    template<>
    struct cast<std::string>
    {
        [[nodiscard]] constexpr static std::string from_string(const char* str)
        {
            return str;
        }
    };

    /* Numeric casts ------------------------------------------------------------------------------------------------ */

    //TODO: Perhaps call each sto... function independentaly
    template<std::floating_point FPType>
    struct cast<FPType>
    {
        [[nodiscard]] constexpr static FPType from_string(const char* str)
        {
            return static_cast<FPType>(std::stold(str));
        }
    };

    template<std::unsigned_integral UIType>
    struct cast<UIType>
    {
        [[nodiscard]] constexpr static UIType from_string(const char* str)
        {
            return static_cast<UIType>(std::stoull(str));
        }
    };

    template<std::signed_integral SIType>
    struct cast<SIType>
    {
        [[nodiscard]] constexpr static SIType from_string(const char* str)
        {
            return static_cast<SIType>(std::stoll(str));
        }
    };
}

#endif // LWCLI_INCLUDE_LWCLI_CAST_STRING_HPP