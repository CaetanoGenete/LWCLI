#ifndef LWCLI_INCLUDE_LWCLI_OPTIONS_HPP
#define LWCLI_INCLUDE_LWCLI_OPTIONS_HPP

#include <string> // For access to std::string
#include <vector> // For access to std::

namespace lwcli
{

struct FlagOption
{
    using count_t = unsigned int;

    std::vector<std::string> aliases;
    std::string description{};
    count_t count = 0;
};

template<class Type>
struct KeyValueOption
{
    using value_t = Type;

    std::vector<std::string> aliases;
    std::string description{};
    value_t value{};
};

template<class Type>
struct PositionalOption
{
    using value_t = Type;

    std::string name{};
    std::string description{};
    value_t value{};
};

} // namespace lwcli

#endif // LWCLI_INCLUDE_LWCLI_OPTIONS_HPP
