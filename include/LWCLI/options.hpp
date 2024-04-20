#ifndef LWCLI_INCLUDE_LWCLI_OPTIONS_HPP
#define LWCLI_INCLUDE_LWCLI_OPTIONS_HPP

#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "LWCLI/cast.hpp"
#include "LWCLI/exceptions.hpp"
#include "LWCLI/type_utility.hpp"
#include "LWCLI/unreachable.hpp"

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

class _named_option_store;

struct _named_id
{
public:
    friend struct ::std::hash<_named_id>;
    friend class _named_option_store;

    enum class Type {
        FLAG,
        KEY_VALUE,
    };

    using value_t = unsigned int;

public:
    constexpr _named_id() noexcept:
        _type(),
        _index(static_cast<value_t>(-1))
    {}

private:
    _named_id(const Type type, const value_t index) noexcept:
        _type(type),
        _index(index)
    {}

public:
    [[nodiscard]] Type type() const noexcept
    {
        return _type;
    }

    [[nodiscard]] bool operator!=(const _named_id&) const noexcept = default;
    [[nodiscard]] bool operator==(const _named_id&) const noexcept = default;

private:
    Type _type;
    value_t _index;
};

// Note: all default constructed `_named_id`s are marked as invalid.
constexpr _named_id _invalid_id{};

} // namespace lwcli

template<>
struct std::hash<lwcli::_named_id>
{
    [[nodiscard]] std::size_t operator()(const lwcli::_named_id& s) const noexcept
    {
        return std::hash<lwcli::_named_id::value_t>{}(s._index);
    }
};

namespace lwcli
{

template<class Type>
void _on_invoke_valued_option(const char* const value, void* const result_ptr)
{
    using naked_type = unwrapped_t<Type>;
    try {
        *static_cast<Type*>(result_ptr) = cast<naked_type>::from_string(value);
    }
    catch (...) {
        throw _bad_cast{value, typeid(naked_type).name()};
    }
}

struct _erased_valued_option
{
    void* result;
    void (*callback)(const char*, void*);
};

class _named_option_store
{
private:
    void _register_aliases(const _named_id id, const std::vector<std::string>& aliases)
    {
        assert(!aliases.empty() && "Named options must define atleast one identifier.");

        _alias_to_id.reserve(_alias_to_id.size() + aliases.size());
        for (const auto& alias : aliases) {
#ifndef LWCLI_DO_NOT_ENFORCE_PREFIXES
            assert(alias.starts_with("-") || alias.starts_with("--"));
#endif // LWCLI_DO_NOT_ENFORCE_PREFIXES

            assert(!_alias_to_id.contains(alias) && "Duplicate option alias detected.");
            assert(alias.find(' ') == std::string::npos && "Aliases should contain no spaces.");

            _alias_to_id.emplace(alias, id);
        }
    }

public:
    void register_flag(FlagOption& option) noexcept
    {
        _register_aliases(
            _named_id(_named_id::Type::FLAG, static_cast<_named_id::value_t>(_flag_count_ptrs.size())),
            option.aliases);

        _flag_count_ptrs.push_back(&option.count);
    }

    template<class Type>
    [[nodiscard]] _named_id register_key_value(KeyValueOption<Type>& option) noexcept
    {
        const _named_id id(_named_id::Type::KEY_VALUE, static_cast<_named_id::value_t>(_key_value_options.size()));
        _register_aliases(id, option.aliases);

        _key_value_options.emplace_back(&option.value, _on_invoke_valued_option<Type>);
        return id;
    }

public:
    [[nodiscard]] _named_id id_of(const std::string& alias) const
    {
        const auto loc = _alias_to_id.find(alias);
        return loc != _alias_to_id.end() ? loc->second : _invalid_id;
    }

    void invoke_flag_option(const _named_id id) const noexcept
    {
        assert(id.type() == _named_id::Type::FLAG);

        ++*static_cast<FlagOption::count_t*>(_flag_count_ptrs[id._index]);
    }

    void invoke_key_value_option(const _named_id id, const char* const value) const noexcept(false)
    {
        assert(id.type() == _named_id::Type::KEY_VALUE);

        const auto option = _key_value_options[id._index];
        // Note: this is expected to throw lwcli::_bad_cast
        std::invoke(option.callback, value, option.result);
    }

public:
    [[nodiscard]] std::unordered_map<std::string, _named_id> alias_to_id() const noexcept
    {
        return _alias_to_id;
    }

private:
    std::vector<void*> _flag_count_ptrs;
    std::vector<_erased_valued_option> _key_value_options;
    std::unordered_map<std::string, _named_id> _alias_to_id;
};

class _positional_options_store
{
public:
    template<class Type>
    void register_option(PositionalOption<Type>& option) noexcept
    {
        _options.emplace_back(&option.value, _on_invoke_valued_option<Type>);
    }

    void invoke_at(const size_t position, const char* const value) const noexcept(false)
    {
        const auto max_positional = _options.size();
        if (position < max_positional) {
            const auto option = _options[position];
            // Note: this is expected to throw lwcli::_bad_cast
            std::invoke(option.callback, value, option.result);
        }
        else
            throw bad_positional_count(value, max_positional);
    }

private:
    std::vector<_erased_valued_option> _options;
};

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

#endif // LWCLI_INCLUDE_LWCLI_OPTIONS_HPP
