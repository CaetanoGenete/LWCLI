#include "gtest/gtest.h"

#include <array>
#include <ranges>
#include <sstream>
#include <string>

#include "LWCLI/LWCLI.hpp"

[[nodiscard]] std::vector<std::string> split_args(const std::string& command_line) noexcept
{
    std::stringstream ss(command_line);
    std::vector<std::string> result;

    std::string arg;
    while (std::getline(ss, arg, ' '))
        result.push_back(arg);

    return result;
}

/* Happy tests ------------------------------------------------------------------------------------------------------ */

[[nodiscard]] testing::AssertionResult parse_succeeds(lwcli::CLIParser& parser, int argc, const char* const* argv)
{
    try {
        parser.parse(argc, argv);
        return testing::AssertionSuccess();
    }
    catch (const lwcli::bad_parse& e) {
        return testing::AssertionFailure()
               << "'" << typeid(lwcli::bad_parse).name() << "' exception thrown with mesasage: " << e.what();
    }
}

template<std::ranges::contiguous_range RangeType>
requires std::is_same_v<const char*, std::ranges::range_value_t<RangeType>>
[[nodiscard]] testing::AssertionResult parse_succeeds(lwcli::CLIParser& parser, const RangeType& argv)
{
    return parse_succeeds(parser, static_cast<int>(std::size(argv)), std::data(argv));
}

[[nodiscard]] testing::AssertionResult parse_succeeds(lwcli::CLIParser& parser, const std::string& args)
{
    const auto args_list = split_args(args);
    const auto cstr_view = args_list | std::views::transform(&std::string::c_str);
    return parse_succeeds(parser, std::vector(std::begin(cstr_view), std::end(cstr_view)));
}

TEST(integration, all_options_types_happy)
{
    lwcli::FlagOption flag_option;
    flag_option.aliases = {"-v"};

    lwcli::KeyValueOption<int> key_value_option;
    key_value_option.aliases = {"--value"};

    lwcli::PositionalOption<double> positional_option;

    lwcli::CLIParser parser;
    parser.register_option(flag_option);
    parser.register_option(key_value_option);
    parser.register_option(positional_option);

    constexpr auto value = "10";
    constexpr auto positional = "0.31415926";
    EXPECT_TRUE(parse_succeeds(parser, std::array{"integration", "-v", "--value", value, positional}));

    ASSERT_EQ(1, flag_option.count);
    ASSERT_EQ(std::stoi(value), key_value_option.value);
    ASSERT_EQ(std::stod(positional), positional_option.value);
}

TEST(interation, duplicate_flag_options_happy)
{
    lwcli::FlagOption flag_option;
    flag_option.aliases = {"--value1", "--value2", "--value3"};

    lwcli::FlagOption other_flag_option;
    other_flag_option.aliases = {"--other-value"};

    lwcli::CLIParser parser;
    parser.register_option(flag_option);
    parser.register_option(other_flag_option);

    EXPECT_TRUE(parse_succeeds(
        parser,
        std::array{
            "integration",
            "--value1",
            "--value1",
            "--value2",
            "--value2",
            "--value2",
            "--value3",
            "--value3",
            "--other-value",
            "--value3",
            "--value3",
        }));
    ASSERT_EQ(9, flag_option.count);
    ASSERT_EQ(1, other_flag_option.count);
}

/* Unhappy tests ---------------------------------------------------------------------------------------------------- */

template<std::derived_from<lwcli::bad_parse> ExpectException>
[[nodiscard]] testing::AssertionResult parse_fails(lwcli::CLIParser& parser, const std::string& args)
{
    const std::vector<std::string> arg_list = split_args(args);
    const auto cstr_view = arg_list | std::views::transform(&std::string::c_str);
    const auto cstr_args = std::vector(std::begin(cstr_view), std::end(cstr_view));
    try {
        parser.parse(static_cast<int>(std::size(cstr_args)), std::data(cstr_args));
    }
    catch (const ExpectException&) {
        return testing::AssertionSuccess();
    }
    catch (const lwcli::bad_parse& e) {
        return testing::AssertionFailure()
               << typeid(lwcli::bad_parse).name() << " exception was thrown, but it was not the expected '"
               << typeid(ExpectException).name() << "', with message: " << e.what();
    }
    catch (...) {
        return testing::AssertionFailure()
               << "Exception was throw, but was not derived from '" << typeid(lwcli::bad_parse).name() << "'";
    }
    return testing::AssertionFailure() << "No exception was thrown";
}

struct key_value_conversion_unhappy_tests : public testing::TestWithParam<std::string>
{};

INSTANTIATE_TEST_SUITE_P(
    invalid_key_value_args,
    key_value_conversion_unhappy_tests,
    testing::Values(
        "integration --value non-int-value",
        "integration --other-value non-int-value",
        "integration --value true --other-value 10"));

TEST_P(key_value_conversion_unhappy_tests, bad_conversion)
{
    lwcli::KeyValueOption<int> value;
    value.aliases = {"--value"};

    lwcli::KeyValueOption<double> other_value;
    other_value.aliases = {"--other-value"};

    lwcli::CLIParser parser;
    parser.register_option(value);
    parser.register_option(other_value);

    // Control case (non-failing)
    EXPECT_TRUE(parse_succeeds(parser, std::array{"control", "--value", "10", "--other-value", "21.3"}));
    // Failing case:
    EXPECT_TRUE(parse_fails<lwcli::bad_value_conversion>(parser, GetParam()));
}

struct bad_positional_unhappy_tests : public testing::TestWithParam<std::string>
{};

INSTANTIATE_TEST_SUITE_P(
    invalid_positional_count_tests,
    bad_positional_unhappy_tests,
    testing::Values("integration 10 20.123 30", "integration 10 20 30 40"));

TEST_P(bad_positional_unhappy_tests, bad_conversion)
{
    lwcli::PositionalOption<int> value1;
    lwcli::PositionalOption<double> value2;

    lwcli::CLIParser parser;
    parser.register_option(value1);
    parser.register_option(value2);

    // Control case (non-failing)
    EXPECT_TRUE(parse_succeeds(parser, std::array{"control", "10", "20.123"}));
    // Failing case:
    EXPECT_TRUE(parse_fails<lwcli::bad_positional_count>(parser, GetParam()));
}

struct bad_key_value_format_unhappy_tests : public testing::TestWithParam<std::string>
{};

INSTANTIATE_TEST_SUITE_P(
    no_value_tests,
    bad_key_value_format_unhappy_tests,
    testing::Values(
        "integration --value1 10 --value2-1",
        "integration --value1 10 --value2-2",
        "integration --value2-1 12.1 --value1"));

TEST_P(bad_key_value_format_unhappy_tests, bad_format)
{
    lwcli::KeyValueOption<int> value1;
    value1.aliases = {"--value1"};

    lwcli::KeyValueOption<double> value2;
    value2.aliases = {"--value2-1", "--value2-2"};

    lwcli::CLIParser parser;
    parser.register_option(value1);
    parser.register_option(value2);

    // Control case (non-failing)
    EXPECT_TRUE(parse_succeeds(parser, std::array{"control", "--value1", "10", "--value2-1", "12.1"}));
    // Failing case:
    EXPECT_TRUE(parse_fails<lwcli::bad_key_value_format>(parser, GetParam()));
}

struct required_key_value_tests : public testing::TestWithParam<std::string>
{};

INSTANTIATE_TEST_SUITE_P(
    invalid_required_key_value_inputs,
    required_key_value_tests,
    testing::Values("integration", "integration --required1 10", "integration --required2 10"));

TEST_P(required_key_value_tests, required_key_value_options_unhappy)
{
    lwcli::KeyValueOption<int> required_1;
    required_1.aliases = {"--required1"};

    lwcli::KeyValueOption<int> required_2;
    required_2.aliases = {"--required2"};

    lwcli::CLIParser parser;
    parser.register_option(required_1);
    parser.register_option(required_2);

    // Control case (non-failing)
    EXPECT_TRUE(parse_succeeds(parser, std::array{"control", "--required1", "10", "--required2", "20"}));
    // Failing case:
    EXPECT_TRUE(parse_fails<lwcli::bad_required_options>(parser, GetParam()));
}
