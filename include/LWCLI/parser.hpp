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

/// @brief Handles the parsing and help-text generation of a command-line interface.
///
/// @warning When registering an option, CLIParser assumes all registered options remain valid until the last invocation
/// of CLIParser::parse(...). If the memory representing an option is freed, at any point before the last invocation of
/// CLIParser::parse(...), the correct behaviour of the function can no longer be guaranteed (And can potentially lead
/// to undefined behaviour).
class CLIParser
{
public:
    /// @brief Registers a flag option to be parsed from the command-line.
    ///
    /// @warning This function will raise an assertion in the event that either:
    ///  - There is a clashing alias already registered.
    ///  - The aliases aren't prefixed with '-' or '--'.
    ///
    /// @param[in, out] option A reference to the option to register.
    /// @return This instance of CLIParser.
    CLIParser& register_option(FlagOption& option)
    {
        _named_options.register_flag(option);
        return *this;
    }

    /// @brief Registers a key-value option to be parsed from the command-line.
    ///
    /// @warning This function will raise an assertion in the event that either:
    /// - There is a clashing alias already registered
    /// - The aliases aren't prefixed with '-' or '--'.
    ///
    /// @tparam Type The expected value-type type of the key-value argument.
    /// @param[in, out] option A reference to the option to register.
    /// @return This instance of CLIParser.
    template<class Type>
    CLIParser& register_option(KeyValueOption<Type>& option)
    {
        const _named_id id = _named_options.register_key_value(option);
        if constexpr (!is_optional_v<Type>)
            _required_options.push_back(id);

        return *this;
    }

    /// @brief Registers a positional option to be parsed from the command-line.
    ///
    /// @tparam Type The expected type of the positional argument.
    /// @param[in, out] option A reference to the option to register.
    /// @return This instance of CLIParser.
    template<class Type>
    CLIParser& register_option(PositionalOption<Type>& option)
    {
        _positional_options.register_option(option);
        return *this;
    }

private:
    void _print_help_message()
    {
        // TODO(Caetano): add usage

        for (const _positional_description& desc : _positional_options.descriptions()) {
            std::cout << *desc.name_ptr << ":\n";
            std::cout << "  " << *desc.description_ptr << "\n\n";
        }

        std::unordered_map<_named_id, std::string> alias_lists;
        for (const auto& [name, id] : _named_options.alias_to_id()) {
            auto [loc, succeeded] = alias_lists.emplace(id, name);
            if (!succeeded)
                loc->second += " | " + name;
        }

        for (const auto& [id, alias_list] : alias_lists) {
            std::cout << alias_list << ":\n";
            std::cout << "  " << _named_options.description_of(id) << "\n\n";
        }
    }

public:
    /// @brief Parses the command-line arguments based on the options registered.
    ///
    /// @param argc The number of arguments
    /// @param argv The argument list
    void parse(const int argc, const char* const* argv)
    {
        if (argc == 1) {
            _print_help_message();
            return;
        }

        std::unordered_set not_visited(std::begin(_required_options), std::end(_required_options));

        size_t position = 0;
        for (int i = 1; i < argc; ++i) {
            const auto& arg = argv[i];
            if (std::strcmp(arg, "-h") == 0 || std::strcmp(arg, "--help") == 0) {
                _print_help_message();
                return;
            }
            // Named option
            if (const auto id = _named_options.id_of(argv[i]); id != _invalid_id) {
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
