#ifndef LWCLI_INCLUDE_LWCLI_TYPE_UTILITY_HPP
#define LWCLI_INCLUDE_LWCLI_TYPE_UTILITY_HPP

#include <optional>

namespace lwcli
{

    template<class Type>
    struct unwrapped { using type = Type; };

    template<class Type>
    struct unwrapped<std::optional<Type>> { using type = Type; };

    template<class Type>
    using unwrapped_t = unwrapped<Type>::type;
}

#endif // LWCLI_INCLUDE_LWCLI_TYPE_UTILITY_HPP