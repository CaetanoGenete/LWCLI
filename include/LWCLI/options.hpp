#ifndef LWCLI_INCLUDE_LWCLI_OPTIONS_HPP
#define LWCLI_INCLUDE_LWCLI_OPTIONS_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <cassert>

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

    class CLIParser
    {
    private:
        union _on_match_t
        {
            void (*nullary)(void*);
            void (*unary)(const char*, void*);
        };

        enum class _option_type
        {
            FLAG,
            KEY_VALUE,
            POSITIONAL
        };

        struct _id_type
        {
            using value_t = uint32_t;

            _option_type type : 2;
            value_t value : 32 - 2;
        };

        struct _match_event
        {
            const _id_type id;
            const _on_match_t on_match;
            void* const result_ptr;
        };

    private:
        [[nodiscard]] _id_type _new_id(_option_type type)
        {
            return { type, static_cast<_id_type::value_t>(_named_events.size() + _positional_events.size()) };
        }

    private:
        static void _on_match_flag(void* const count_ptr)
        {
            auto& count = *static_cast<FlagOption::count_t* const>(count_ptr);
            ++count;
        }

        template<class Type>
        static void _on_match_valued(
            const char* key,
            void* const value_ptr)
        {
            std::cout << "matched valued option: " << typeid(Type).name() << ", " << key << std::endl;
        }

    private:
        CLIParser& _register_named(
            const _option_type type,
            const std::vector<std::string>& aliases,
            const _on_match_t on_match,
            void* const result)
        {
            _named_events.reserve(_named_events.size() + aliases.size());
            for (const std::string& key : aliases) {
                _named_events.emplace(
                    std::piecewise_construct,
                    std::make_tuple(key),
                    std::make_tuple(_new_id(type), on_match, result));
            }
            return *this;
        }

    public:
        CLIParser& register_option(FlagOption& option)
        {
            return _register_named(
                _option_type::FLAG,
                option.aliases,
                { .nullary = _on_match_flag },
                &option.count);
        }

        template<class Type>
        CLIParser& register_option(KeyValueOption<Type>& option)
        {
            return _register_named(
                _option_type::KEY_VALUE,
                option.aliases,
                { .unary = _on_match_valued<Type> },
                &option.value);
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
        void parse(unsigned int argc, const char** argv)
        {
            size_t position = 0;

            const auto argend = argv + argc;
            while (++argv != argend) {
                const auto loc = _named_events.find(*argv);
                if (loc != _named_events.end()) {
                    const auto event = loc->second;
                    assert(event.id.type != _option_type::POSITIONAL);
                    switch (event.id.type) {
                    case _option_type::FLAG:
                        std::invoke(event.on_match.nullary, event.result_ptr);
                        break;
                    case _option_type::KEY_VALUE:
                        if (++argv == argend) {
                            //TODO: Properly handle error
                            return;
                        }
                        std::invoke(event.on_match.unary, *argv, event.result_ptr);
                        break;
                    }
                }
                else {
                    const auto event = _positional_events[position++];
                    std::invoke(event.on_match.unary, *argv, event.result_ptr);
                }
            }
        }

    private:
        std::unordered_map<std::string, _match_event> _named_events;
        std::vector<_match_event> _positional_events;
        // std::vector<_match_event::id_t> _required_events;
    };

}

#endif // LWCLI_INCLUDE_LWCLI_OPTIONS_HPP