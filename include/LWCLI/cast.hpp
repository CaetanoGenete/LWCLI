#ifndef LWCLI_INCLUDE_LWCLI_CAST_STRING_HPP
#define LWCLI_INCLUDE_LWCLI_CAST_STRING_HPP

#include <concepts> // For access to std::floating_point
#include <string>
#include <vector>

namespace lwcli
{
template<class Type>
struct cast;

/* String casts ------------------------------------------------------------------------------------------------- */

template<>
struct cast<std::string>
{
    [[nodiscard]] constexpr static std::string from_string(const std::string& str) noexcept
    {
        return str;
    }
};

/* Numeric casts ------------------------------------------------------------------------------------------------ */

// TODO: Perhaps call each sto[x] function independentaly
template<std::floating_point FPType>
struct cast<FPType>
{
    [[nodiscard]] constexpr static FPType from_string(const std::string& str)
    {
        return static_cast<FPType>(std::stold(str));
    }
};

template<std::unsigned_integral UIType>
struct cast<UIType>
{
    [[nodiscard]] constexpr static UIType from_string(const std::string& str)
    {
        return static_cast<UIType>(std::stoull(str));
    }
};

template<std::signed_integral SIType>
struct cast<SIType>
{
    [[nodiscard]] constexpr static SIType from_string(const std::string& str)
    {
        return static_cast<SIType>(std::stoll(str));
    }
};

/* Misc casts --------------------------------------------------------------------------------------------------- */

template<class Type, class Alloc>
struct cast<std::vector<Type, Alloc>>
{
    [[nodiscard]] constexpr static std::vector<Type, Alloc> from_string(const std::string& str)
    {
        constexpr auto delim = ',';

        std::vector<Type> result;
        if (!str.empty()) {
            size_t substr_begin = 0;
            for (size_t curr = 0; curr < str.size(); ++curr) {
                if (str[curr] == delim) {
                    result.push_back(cast<Type>::from_string(str.substr(substr_begin, curr - substr_begin)));
                    substr_begin = curr + 1;
                }
            }
            result.push_back(cast<Type>::from_string(str.substr(substr_begin)));
        }
        return result;
    }
};
} // namespace lwcli

#endif // LWCLI_INCLUDE_LWCLI_CAST_STRING_HPP