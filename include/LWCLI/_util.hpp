#ifndef LWCLI_INCLUDE_LWCLI_UTIL_HPP
#define LWCLI_INCLUDE_LWCLI_UTIL_HPP

#include <algorithm>
#include <cstring>
#include <ranges>

namespace lwcli
{

[[nodiscard]] inline bool streq(const char* const lhs, const char* const rhs)
{
    return std::strcmp(lhs, rhs) == 0;
}

template<
    std::ranges::forward_range LHSRange,
    std::ranges::forward_range RHSRange,
    class Predicate = std::ranges::equal_to>
requires std::indirect_binary_predicate<Predicate, std::ranges::iterator_t<LHSRange>, std::ranges::iterator_t<RHSRange>>
[[nodiscard]] constexpr bool contains_any_of(LHSRange&& lhs, RHSRange&& rhs, Predicate&& pred = {})
{
    const auto result = std::ranges::find_first_of(
        std::forward<LHSRange>(lhs),
        std::forward<RHSRange>(rhs),
        std::forward<Predicate>(pred));
    return result != std::ranges::end(lhs);
}

} // namespace lwcli

#endif // LWCLI_INCLUDE_LWCLI_UTIL_HPP
