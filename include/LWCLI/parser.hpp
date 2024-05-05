#ifndef LWCLI_INCLUDE_LWCLI_PARSER_HPP
#define LWCLI_INCLUDE_LWCLI_PARSER_HPP

#include <cassert>       // For access to assert
#include <cstdint>       // For access to size_t
#include <iostream>      // For access to std::cout
#include <sstream>       // For access to std::stringstream
#include <string>        // For access to std::string
#include <unordered_set> // For access to std::unordered_set
#include <vector>        // For access to std::vector

#include "LWCLI/_options_stores.hpp"
#include "LWCLI/exceptions.hpp"
#include "LWCLI/options.hpp"
#include "LWCLI/unreachable.hpp"

namespace lwcli
{

class CLIParser
{
public:
    CLIParser& register_option(FlagOption& option)
    {
        _named_options.register_flag(option);
        return *this;
    }

    template<class Type>
    CLIParser& register_option(KeyValueOption<Type>& option)
    {
        const _named_id id = _named_options.register_key_value(option);
        if constexpr (!is_optional_v<Type>)
            _required_options.push_back(id);

        return *this;
    }

    template<class Type>
    CLIParser& register_option(PositionalOption<Type>& option)
    {
        _positional_options.register_option(option);
        return *this;
    }

private:
    void _print_help_message()
    {
        for (const _positional_description& desc : _positional_options.descriptions()) {
            std::cout << desc.name_ptr << ":\n";
            if (!desc.description_ptr->empty())
                std::cout << "  " << *desc.description_ptr << "\n\n";
            else
                std::cout << "\n";
        }
    }

public:
    void parse(const int argc, const char* const* argv)
    {
        std::unordered_set not_visited(std::begin(_required_options), std::end(_required_options));

        size_t position = 0;
        for (int i = 1; i < argc; ++i) {
            const auto& arg = argv[i];
            if (std::strcmp(arg, "-h") == 0 || std::strcmp(arg, "--help") == 0) {
                _print_help_message();
            }
            // Named option
            else if (const auto id = _named_options.id_of(argv[i]); id != _invalid_id) {
                not_visited.erase(id);
                switch (id.type()) {
                case _named_id::Type::FLAG:
                    _named_options.invoke_flag_option(id);
                    break;

                case _named_id::Type::KEY_VALUE:
                    if (++i == argc)
                        throw bad_key_value_format(arg);

                    try {
                        _named_options.invoke_key_value_option(id, argv[i]);
                        break;
                    }
                    catch (const _bad_cast& e) {
                        throw bad_value_conversion(arg, e);
                    }

                default:
                    _unreachable();
                }
            }
            // Positional option
            else {
                try {
                    _positional_options.invoke_at(position++, arg);
                }
                catch (const _bad_cast& e) {
                    throw bad_positional_conversion(e);
                }
            }
        }

        if (!not_visited.empty()) [[unlikely]] {
            std::unordered_map<_named_id, std::string> aliases;
            for (const auto& [alias, id] : _named_options.alias_to_id()) {
                if (not_visited.contains(id)) {
                    const auto [loc, success] = aliases.try_emplace(id, alias);
                    if (!success)
                        loc->second += " | " + alias;
                }
            }
            throw bad_required_options(aliases | std::views::values);
        }
    }

private:
    _named_option_store _named_options;
    _positional_options_store _positional_options;

    std::vector<_named_id> _required_options;
};

} // namespace lwcli

#endif // LWCLI_INCLUDE_LWCLI_PARSER_HPP
