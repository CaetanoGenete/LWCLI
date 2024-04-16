#ifndef LWCLI_INCLUDE_LWCLI_UNREACHABLE_HPP
#define LWCLI_INCLUDE_LWCLI_UNREACHABLE_HPP

#include <utility>

namespace lwcli
{

[[noreturn]] inline void unreachable()
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
}

} // namespace lwcli

#endif // LWCLI_INCLUDE_LWCLI_UNREACHABLE_HPP
