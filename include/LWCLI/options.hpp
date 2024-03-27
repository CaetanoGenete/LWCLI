#ifndef LWCLI_INCLUDE_LWCLI_OPTIONS_HPP
#define LWCLI_INCLUDE_LWCLI_OPTIONS_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <cassert>
#include <sstream>
#include <cstdint>
#include <functional>
#include <unordered_set>
#include <ranges>

#include "LWCLI/cast.hpp"
#include "LWCLI/type_utility.hpp"

namespace lwcli
{

    struct FlagOption
    {
        using count_t = unsigned int;

        std::vector<std::string> aliases;
        count_t count = 0;
    };

    template<class Type>
    struct KeyValueOption
    {
        using value_t = Type;

        std::vector<std::string> aliases;
        value_t value;
    };

    template<class Type>
    struct PositionalOption
    {
        using value_t = Type;

        unsigned int position;
        value_t value;
    };

    struct _bad_cast
    {
        const char* value;
        std::string type_name;
    };

    struct bad_parse : public std::runtime_error
    {
        bad_parse(const std::string& message):
            std::runtime_error("[FATAL] " + message) {}
    };

    void _on_match_flag(void* const count_ptr)
    {
        auto& count = *static_cast<FlagOption::count_t*>(count_ptr);
        ++count;
    }

    template<class Type>
    void _on_match_valued(
        const char* value,
        void* const result_ptr)
    {
        using naked_type = unwrapped_t<Type>;
        try {
            *static_cast<Type*>(result_ptr) = cast<naked_type>::from_string(value);
        }
        catch (...) {
            throw _bad_cast{ value, typeid(naked_type).name() };
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

        enum class _option_type : uint8_t
        {
            FLAG,
            KEY_VALUE,
            POSITIONAL
        };

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
            void* result_ptr;
        };

    private:
        [[nodiscard]] auto _new_id(_option_type type) noexcept
        {
            const auto id_value = static_cast<_id_type::value_t>(_named_events.size() + _positional_events.size());
            // Explicitely masking `id_value` to avoid GCC -Wconversion error.
            return _id_type{ type, id_value % (1 << _id_type::nvalue_bits) };
        }

    private:
        _id_type _register_named(
            const _option_type type,
            const std::vector<std::string>& aliases,
            const _on_match_t on_match,
            void* const result)
        {
            _named_events.reserve(_named_events.size() + aliases.size());

            const auto id = _new_id(type);
            for (const std::string& key : aliases) {
#ifndef LWCLI_DO_NOT_ENFORCE_PREFIXES
                assert(key.starts_with("-") || key.starts_with("--"));
#endif // LWCLI_DO_NOT_ENFORCE_PREFIXES
                assert(!_named_events.contains(key));

                _named_events.emplace(
                    std::piecewise_construct,
                    std::make_tuple(key),
                    std::make_tuple(id, on_match, result));
            }
            return id;
        }

    public:
        CLIParser& register_option(FlagOption& option)
        {
            _register_named(
                _option_type::FLAG,
                option.aliases,
                { .nullary = _on_match_flag },
                &option.count);

            return *this;
        }

        template<class Type>
        CLIParser& register_option(KeyValueOption<Type>& option)
        {
            const _id_type id = _register_named(
                _option_type::KEY_VALUE,
                option.aliases,
                { .unary = _on_match_valued<Type> },
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
                _on_match_t{ .unary = _on_match_valued<Type> },
                &option.value);
            return *this;
        }

    public:
        void parse(const int argc, const char* const* argv)
        {
            const size_t max_positional = _positional_events.size();

            std::unordered_set not_visted(
                std::begin(_required_events),
                std::end(_required_events));

            const auto argend = argv + argc;
            for (size_t position = 0; ++argv != argend;) {
                _match_event event;
                if (const auto loc = _named_events.find(*argv); loc != _named_events.end()) {
                    event = loc->second;
                    not_visted.erase(event.id.value);
                }
                else {
                    if (position < max_positional) {
                        event = _positional_events[position++];
                    }
                    else {
                        std::stringstream ss;
                        ss << "While parsing '"
                            << *argv
                            << "': program expects at most "
                            << max_positional
                            << " positional arguments, yet at least "
                            << position + 1
                            << " were provided.";
                        throw bad_parse(ss.str());
                    }
                }

                switch (event.id.type) {
                case _option_type::FLAG:
                    std::invoke(event.on_match.nullary, event.result_ptr);
                    break;

                case _option_type::KEY_VALUE:
                    if (++argv == argend) {
                        std::stringstream ss;
                        ss << "Option '"
                            << *std::prev(argv)
                            << "' expected an argument, but none were provided.";
                        throw bad_parse(ss.str());
                    }
                    [[fallthrough]];

                case _option_type::POSITIONAL:
                    try {
                        std::invoke(event.on_match.unary, *argv, event.result_ptr);
                    }
                    catch (const _bad_cast& e) {
                        std::stringstream ss;
                        if (event.id.type == _option_type::KEY_VALUE) {
                            ss << "While parsing '"
                                << *std::prev(argv)
                                << "': ";
                        }
                        ss << "No suitable conversion found from '"
                            << e.value
                            << "' to "
                            << e.type_name;
                        throw bad_parse(ss.str());
                    }
                    break;
                }
            }

            if (!not_visted.empty()) {
                std::unordered_map<_id_type::value_t, std::string> aliases;
                for (const auto& [key, event] : _named_events) {
                    if (not_visted.contains(event.id.value)) {
                        auto [loc, success] = aliases.try_emplace(event.id.value, key);
                        if (!success)
                            loc->second += " | " + key;
                    }
                }

                std::stringstream ss;
                ss << "Arguments:\n";
                for (const std::string& arg_list : aliases | std::views::values)
                    ss << "\t> " << arg_list << "\n";
                ss << "were expected, but not provided.";
                throw bad_parse(ss.str());
            }
        }

    private:
        std::unordered_map<std::string, _match_event> _named_events;
        std::vector<_id_type::value_t> _required_events;

        std::vector<_match_event> _positional_events;
    };

}

#endif // LWCLI_INCLUDE_LWCLI_OPTIONS_HPP