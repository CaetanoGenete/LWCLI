#ifndef LWCLI_INCLUDE_LWCLI_EXCEPTIONS_HPP
#define LWCLI_INCLUDE_LWCLI_EXCEPTIONS_HPP

#include <cstdint>
#include <ranges> // for access to std::ranges::input_range
#include <sstream>
#include <stdexcept>
#include <string>

namespace lwcli
{
/// Base class from which all LWCLI parsing errors inherit.
struct bad_parse : public std::runtime_error
{
protected:
    bad_parse(const std::string& failed_expression, const std::string& message):
        std::runtime_error("[FATAL] While parsing '" + failed_expression + "': " + message)
    {}

public:
    const std::string failed_expression;
};

struct _bad_cast
{
    const std::string value;
    const std::string type_name;
};

/// Exception thrown if: More than the expected number of positional arguments are provided.
///
/// > [!NOTE]
/// > Unrecognised key-value/flag options are parsed as positional arguments, hence, this exception may be thrown in
/// > the event that their identifiers are misspelled.
struct bad_positional_count : public bad_parse
{
public:
    bad_positional_count(const std::string& failed_expression, size_t n_max_positional):
        bad_parse(
            failed_expression,
            "Program expects at most " + std::to_string(n_max_positional) + " positional arguments, but at least "
                + std::to_string(n_max_positional + 1) + " were provided."),
        n_max_positional(n_max_positional)
    {}

    const size_t n_max_positional;
};

/// Exception thrown upon failure to convert from string to the expected type of a positional argument.
struct bad_positional_conversion : public bad_parse
{
    explicit bad_positional_conversion(const _bad_cast& error_data):
        bad_parse(error_data.value, "No suitable conversion found to " + error_data.type_name + " type."),
        value(error_data.value),
        type(error_data.type_name)
    {}

    const std::string value;
    const std::string type;
};

/// Exception thrown upon failure to convert from string to the expected type of a key-value option.
struct bad_value_conversion : public bad_parse
{
    bad_value_conversion(const std::string& key, const _bad_cast& error_data):
        bad_parse(
            key,
            "No suitable conversion found from '" + error_data.value + "' to " + error_data.type_name + " type."),
        value(error_data.value),
        type(error_data.type_name)
    {}

    const std::string value;
    const std::string type;
};

/// Exception thrown if no value is provided to a key-value option, this can only happen if the key is the last
/// argument passed.
struct bad_key_value_format : public bad_parse
{
    explicit bad_key_value_format(const std::string& key):
        bad_parse(key, "Expected a value, but none were provided")
    {}
};

/// Exception thrown if not **all** *required* arguments have been provided.
struct bad_required_options : public bad_parse
{
private:
    template<std::ranges::input_range Range>
    static std::string _build_error_messasge(const Range& missing_options)
    {
        std::stringstream stream;
        stream << "Arguments:\n";
        for (const std::string& arg_list : missing_options)
            stream << "\t> " << arg_list << "\n";
        stream << "were expected, but not provided.";
        return stream.str();
    }

public:
    template<std::ranges::input_range Range>
    requires std::is_convertible_v<std::ranges::range_value_t<Range>, std::string>
    explicit bad_required_options(const Range& missing_options):
        bad_parse("", _build_error_messasge(missing_options))
    {}
};
} // namespace lwcli

#endif // LWCLI_INCLUDE_LWCLI_EXCEPTIONS_HPP
