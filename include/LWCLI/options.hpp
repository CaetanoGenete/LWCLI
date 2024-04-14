#ifndef LWCLI_INCLUDE_LWCLI_OPTIONS_HPP
#define LWCLI_INCLUDE_LWCLI_OPTIONS_HPP

#include <cassert>
#include <cstdint>
#include <functional>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "LWCLI/cast.hpp"
#include "LWCLI/exceptions.hpp"
#include "LWCLI/type_utility.hpp"

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

    std::string name;
    std::string description{};
    value_t value{};
};

void _on_match_flag(void* const count_ptr)
{
    auto& count = *static_cast<FlagOption::count_t*>(count_ptr);
    ++count;
}

template<class Type>
void _on_match_valued(const char* value, void* const result_ptr)
{
    using naked_type = unwrapped_t<Type>;
    try {
        *static_cast<Type*>(result_ptr) = cast<naked_type>::from_string(value);
    }
    catch (...) {
        throw _bad_cast{value, typeid(naked_type).name()};
    }
}

class CLIParser
{
private:
    union _on_match_t
    {
        void (*nullary)(void*);
        void (*unary)(const char*, void*);
    };

    enum class _option_type : uint8_t { FLAG, KEY_VALUE, POSITIONAL };

    struct _id_type
    {
        using value_t = uint32_t;

        static constexpr uint8_t ntype_bits = 2;
        static constexpr uint8_t nvalue_bits = sizeof(value_t) * 8 - ntype_bits;

        _option_type type : ntype_bits;
        value_t value : nvalue_bits;
    };

    struct _match_event
    {
        _id_type id;
        _on_match_t on_match;

        const std::string* description;
        void* result_ptr;
    };

private:
    [[nodiscard]] auto _new_id(_option_type type) noexcept
    {
        const auto id_value = static_cast<_id_type::value_t>(_named_events.size() + _positional_events.size());
        // Explicitely masking `id_value` to avoid GCC -Wconversion error.
        return _id_type{type, id_value % (1 << _id_type::nvalue_bits)};
    }

private:
    _id_type _register_named(
        const _option_type type,
        const std::vector<std::string>& aliases,
        const _on_match_t on_match,
        const std::string* const description,
        void* const result)
    {
        assert(!aliases.empty() && "Named options must define atleast one identifier.");
        _named_events.reserve(_named_events.size() + aliases.size());

        const auto id = _new_id(type);
        for (const std::string& key : aliases) {
#ifndef LWCLI_DO_NOT_ENFORCE_PREFIXES
            assert(key.starts_with("-") || key.starts_with("--"));
#endif // LWCLI_DO_NOT_ENFORCE_PREFIXES
            assert(key.find(' ') == std::string::npos && "Aliases should contain no spaces.");
            assert(!_named_events.contains(key));

            _named_events.emplace(
                std::piecewise_construct,
                std::make_tuple(key),
                std::make_tuple(id, on_match, description, result));
        }
        return id;
    }

public:
    CLIParser& register_option(FlagOption& option)
    {
        _register_named(
            _option_type::FLAG,
            option.aliases,
            {.nullary = _on_match_flag},
            &option.description,
            &option.count);

        return *this;
    }

    template<class Type>
    CLIParser& register_option(KeyValueOption<Type>& option)
    {
        const _id_type id = _register_named(
            _option_type::KEY_VALUE,
            option.aliases,
            {.unary = _on_match_valued<Type>},
            &option.description,
            &option.value);

        if constexpr (!lwcli::is_optional_v<Type>)
            _required_events.push_back(id.value);

        return *this;
    }

    template<class Type>
    CLIParser& register_option(PositionalOption<Type>& option)
    {
        _positional_events.emplace_back(
            _new_id(_option_type::POSITIONAL),
            _on_match_t{.unary = _on_match_valued<Type>},
            &option.description,
            &option.value);

        return *this;
    }

public:
    void parse(const int argc, const char* const* argv)
    {
        const size_t max_positional = _positional_events.size();

        std::unordered_set not_visted(std::begin(_required_events), std::end(_required_events));

        const auto argend = argv + argc;
        for (size_t position = 0; ++argv != argend;) {
            _match_event event;
            if (const auto loc = _named_events.find(*argv); loc != _named_events.end()) {
                event = loc->second;
                not_visted.erase(event.id.value);
            }
            else {
                if (position < max_positional)
                    event = _positional_events[position++];
                else
                    throw bad_positional_count(*argv, max_positional);
            }

            switch (event.id.type) {
            case _option_type::FLAG:
                std::invoke(event.on_match.nullary, event.result_ptr);
                break;

            case _option_type::KEY_VALUE:
                if (++argv == argend)
                    throw bad_key_value_format(*std::prev(argv));
                [[fallthrough]];

            case _option_type::POSITIONAL:
                try {
                    std::invoke(event.on_match.unary, *argv, event.result_ptr);
                }
                catch (const _bad_cast& e) {
                    if (event.id.type == _option_type::KEY_VALUE)
                        throw bad_value_conversion(*std::prev(argv), e);
                    else
                        throw bad_positional_conversion(e);
                }
                break;
            }
        }

        if (!not_visted.empty()) [[unlikely]] {
            std::unordered_map<_id_type::value_t, std::string> aliases;
            for (const auto& [key, event] : _named_events) {
                if (not_visted.contains(event.id.value)) {
                    auto [loc, success] = aliases.try_emplace(event.id.value, key);
                    if (!success)
                        loc->second += " | " + key;
                }
            }

            throw bad_required_options(aliases | std::views::values);
        }
    }

private:
    std::unordered_map<std::string, _match_event> _named_events;
    std::vector<_id_type::value_t> _required_events;

    std::vector<_match_event> _positional_events;
};

} // namespace lwcli

#endif // LWCLI_INCLUDE_LWCLI_OPTIONS_HPP
