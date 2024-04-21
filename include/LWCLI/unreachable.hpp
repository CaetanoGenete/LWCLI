#ifndef LWCLI_INCLUDE_LWCLI_UNREACHABLE_HPP
#define LWCLI_INCLUDE_LWCLI_UNREACHABLE_HPP

#include <utility> // For access to std::unreachable (c++23)

namespace lwcli
{

[[noreturn]] inline void _unreachable()
{
#ifdef __cpp_lib_unreachable
    std::unreachable();
#else
    #ifdef __GNUC__         // GCC, Clang, ICC
    __builtin_unreachable();
    #elif defined(_MSC_VER) // MSVC
    __assume(false);
    #endif
#endif
    // Note: If neither __GNUC__ nor _MSC_VER is defined, this function will still trigger undefined behaviour due to
    // the [[noreturn]].
}

} // namespace lwcli

#endif // LWCLI_INCLUDE_LWCLI_UNREACHABLE_HPP
