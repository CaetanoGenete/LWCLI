#ifndef LWCLI_INCLUDE_LWCLI_PARSER_HPP
#define LWCLI_INCLUDE_LWCLI_PARSER_HPP

#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

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

public:
    void parse(const int argc, const char* const* argv)
    {
        std::unordered_set not_visted(std::begin(_required_options), std::end(_required_options));

        const auto argend = argv + argc;
        for (size_t position = 0; ++argv != argend;) {
            // Named option
            if (const auto id = _named_options.id_of(*argv); id != _invalid_id) {
                not_visted.erase(id);
                switch (id.type()) {
                case _named_id::Type::FLAG:
                    _named_options.invoke_flag_option(id);
                    break;

                case _named_id::Type::KEY_VALUE:
                    if (++argv == argend)
                        throw bad_key_value_format(*std::prev(argv));

                    try {
                        _named_options.invoke_key_value_option(id, *argv);
                        break;
                    }
                    catch (const _bad_cast& e) {
                        throw bad_value_conversion(*std::prev(argv), e);
                    }

                default:
                    _unreachable();
                }
            }
            // Positional option
            else {
                try {
                    _positional_options.invoke_at(position++, *argv);
                }
                catch (const _bad_cast& e) {
                    throw bad_positional_conversion(e);
                }
            }
        }

        if (!not_visted.empty()) [[unlikely]] {
            std::unordered_map<_named_id, std::string> aliases;
            for (const auto& [alias, id] : _named_options.alias_to_id()) {
                if (not_visted.contains(id)) {
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
