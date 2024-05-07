#ifndef LWCLI_INCLUDE_LWCLI_OPTIONS_STORES_HPP
#define LWCLI_INCLUDE_LWCLI_OPTIONS_STORES_HPP

#include <cassert>       // For access to assert
#include <cstdint>       // For access to size_t
#include <functional>    // For access to std::invoke
#include <string>        // For access to std::string
#include <unordered_map> // For access to std::unordered_map
#include <vector>        // For access to std::vector

#include "LWCLI/cast.hpp"
#include "LWCLI/exceptions.hpp"
#include "LWCLI/options.hpp"
#include "LWCLI/type_utility.hpp"
#include "LWCLI/unreachable.hpp"

namespace lwcli
{

class _named_option_store;

struct _named_id
{
public:
    friend struct ::std::hash<_named_id>;
    friend class _named_option_store;

    enum class Type : uint8_t {
        FLAG,
        KEY_VALUE,
    };

    using value_t = unsigned int;

public:
    constexpr _named_id() noexcept:
        _type(Type::FLAG),
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

template<class Type>
void _on_invoke_valued_option(const char* const value, void* const result_ptr)
{
    using naked_type = unwrapped_t<Type>;
    try {
        *static_cast<Type*>(result_ptr) = cast<naked_type>::from_string(value);
    }
    catch (...) {
        // TODO(Caetano): perhaps add method of displaying pretty names
        throw _bad_cast(value, typeid(naked_type).name());
    }
}

struct _erased_valued_option
{
    void* result;
    void (*callback)(const char*, void*);
};

// Helper class to store and retrieve named options (i.e. flag and key-value options) in O(1) time. Interfacing with
// this class involves first registering an option using register_flag(...), returning an id object which may then be
// used to:
// - parse the option using invoke_flag(...) and invoke_key_value(...).
// - retrieving the description of the option with description_of(...).
//
// Id's can also be retrieved by alias, via the id_of(...) member.
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
    void register_flag(FlagOption& option)
    {
        // To keep with CLI best practices, flag options must always have a description
        assert(!option.description.empty());

        _register_aliases(
            _named_id(_named_id::Type::FLAG, static_cast<_named_id::value_t>(_flag_count_ptrs.size())),
            option.aliases);

        _flag_count_ptrs.push_back(&option.count);
        _flag_descriptions.push_back(&option.description);
        assert(_flag_count_ptrs.size() == _flag_descriptions.size());
    }

    template<class Type>
    [[nodiscard]] _named_id register_key_value(KeyValueOption<Type>& option)
    {
        // To keep with CLI best practices, key-value options must always have a description.
        assert(!option.description.empty());

        const _named_id id(_named_id::Type::KEY_VALUE, static_cast<_named_id::value_t>(_key_value_options.size()));
        _register_aliases(id, option.aliases);

        _key_value_options.emplace_back(&option.value, _on_invoke_valued_option<Type>);
        _key_value_descriptions.push_back(&option.description);
        assert(_key_value_options.size() == _key_value_descriptions.size());

        return id;
    }

public:
    [[nodiscard]] _named_id id_of(const std::string& alias) const
    {
        const auto loc = _alias_to_id.find(alias);
        return loc != _alias_to_id.end() ? loc->second : _invalid_id;
    }

    [[nodiscard]] const std::string& description_of(const _named_id id) const noexcept
    {
        switch (id.type()) {
        case _named_id::Type::FLAG:
            return *_flag_descriptions[id._index];
        case _named_id::Type::KEY_VALUE:
            return *_key_value_descriptions[id._index];
        }
        // Note: needed to stop clang-tidy from complaining (Even enums are exhausted)...
        _unreachable();
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
    [[nodiscard]] const std::unordered_map<std::string, _named_id>& alias_to_id() const noexcept
    {
        return _alias_to_id;
    }

private:
    std::unordered_map<std::string, _named_id> _alias_to_id;

    std::vector<void*> _flag_count_ptrs;
    std::vector<const std::string*> _flag_descriptions;

    std::vector<_erased_valued_option> _key_value_options;
    std::vector<const std::string*> _key_value_descriptions;
};

struct _positional_description
{
    std::string* name_ptr;
    std::string* description_ptr;
};

class _positional_options_store
{
public:
    template<class Type>
    void register_option(PositionalOption<Type>& option)
    {
        // To keep with CLI best practices, names and descriptions must be provided for all positional
        // options.
        assert(!option.name.empty());
        assert(!option.description.empty());

        _options.emplace_back(&option.value, _on_invoke_valued_option<Type>);
        _descriptions.emplace_back(&option.name, &option.description);
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

public:
    [[nodiscard]] const std::vector<_positional_description>& descriptions() const noexcept
    {
        return _descriptions;
    }

private:
    std::vector<_erased_valued_option> _options;
    std::vector<_positional_description> _descriptions;
};

} // namespace lwcli

template<>
struct std::hash<lwcli::_named_id>
{
    [[nodiscard]] std::size_t operator()(const lwcli::_named_id& id) const noexcept
    {
        const auto index = static_cast<size_t>(id._index);
        assert(index < std::numeric_limits<size_t>::max() / 2 && "id too large, uniqueness of hash not guaranteed.");

        switch (id.type()) {
        case lwcli::_named_id::Type::FLAG:
            return 2 * index;
        case lwcli::_named_id::Type::KEY_VALUE:
            return 2 * index + 1;
        }
        lwcli::_unreachable();
    }
};

#endif // LWCLI_INCLUDE_LWCLI_OPTIONS_STORES_HPP
