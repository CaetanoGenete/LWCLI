#include "gtest/gtest.h" // cppcheck-suppress [missingInclude]

#include <array>
#include <concepts>
#include <ranges>
#include <string>
#include <vector>

#include "LWCLI/exceptions.hpp"
#include "LWCLI/options.hpp"
#include "LWCLI/parser.hpp"

[[nodiscard]] std::vector<std::string> split_args(const std::string& command_line)
{
    std::vector<std::string> result;
    for (const auto& substr : command_line | std::views::split(' '))
        // cppcheck-suppress [useStlAlgorithm]
        result.emplace_back(std::begin(substr), std::end(substr));

    return result;
}

/* Happy tests ------------------------------------------------------------------------------------------------------ */

[[nodiscard]] testing::AssertionResult parse_succeeds(lwcli::CLIParser& parser, int argc, const char* const* argv)
{
    try {
        parser.parse(argc, argv);
    }
    catch (const lwcli::bad_parse& e) {
        return testing::AssertionFailure()
               << "'" << typeid(lwcli::bad_parse).name() << "' exception thrown with message: " << e.what();
    }
    return testing::AssertionSuccess();
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

TEST(integration, AllOptionTypesHappy)
{
    lwcli::FlagOption flag_option;
    flag_option.aliases = {"-v"};
    flag_option.description = "Description for option";

    lwcli::KeyValueOption<int> key_value_option;
    key_value_option.aliases = {"--value"};
    key_value_option.description = "Description for key-value option";

    lwcli::PositionalOption<double> positional_option;
    positional_option.name = "Some random name";
    positional_option.description = "Description for positional option";

    lwcli::CLIParser parser;
    parser.register_option(flag_option);
    parser.register_option(key_value_option);
    parser.register_option(positional_option);

    constexpr auto value = "10";
    constexpr auto positional = "0.31415926";
    EXPECT_TRUE(parse_succeeds(parser, std::array{"integration", "-v", "--value", value, positional}));

    EXPECT_EQ(1, flag_option.count);
    EXPECT_EQ(std::stoi(value), key_value_option.value);
    EXPECT_EQ(std::stod(positional), positional_option.value);
}

TEST(interation, DuplicateFlagOptionsHappy)
{
    lwcli::FlagOption flag_option;
    flag_option.aliases = {"--value1", "--value2", "--value3"};
    flag_option.description = "Description for option";

    lwcli::FlagOption other_flag_option;
    other_flag_option.aliases = {"--other-value"};
    other_flag_option.description = "Description for other option";

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
    EXPECT_EQ(9, flag_option.count);
    EXPECT_EQ(1, other_flag_option.count);
}

/* Unhappy tests ---------------------------------------------------------------------------------------------------- */

template<std::derived_from<lwcli::bad_parse> ExpectedException>
[[nodiscard]] testing::AssertionResult parse_fails(lwcli::CLIParser& parser, const std::string& args)
{
    const std::vector<std::string> arg_list = split_args(args);
    const auto cstr_view = arg_list | std::views::transform(&std::string::c_str);
    const auto cstr_args = std::vector(std::begin(cstr_view), std::end(cstr_view));

    try {
        parser.parse(static_cast<int>(std::size(cstr_args)), std::data(cstr_args));
    }
    catch (const ExpectedException&) {
        return testing::AssertionSuccess();
    }
    catch (const lwcli::bad_parse& e) {
        return testing::AssertionFailure()
               << typeid(lwcli::bad_parse).name() << " exception was thrown, but it was not the expected '"
               << typeid(ExpectedException).name() << "', with message: " << e.what();
    }
    catch (...) {
        return testing::AssertionFailure()
               << "Exception was throw, but was not derived from '" << typeid(lwcli::bad_parse).name() << "'";
    }
    return testing::AssertionFailure() << "No exception was thrown";
}

class KeyValueConversionUnhappyTests : public testing::TestWithParam<std::string>
{};

INSTANTIATE_TEST_SUITE_P(
    invalid_key_value_args,
    KeyValueConversionUnhappyTests,
    testing::Values(
        "integration --value non-int-value",
        "integration --other-value non-int-value",
        "integration --value true --other-value 10"));

TEST_P(KeyValueConversionUnhappyTests, BadConversion)
{
    lwcli::KeyValueOption<int> value;
    value.aliases = {"--value"};
    value.description = "Description for value";

    lwcli::KeyValueOption<double> other_value;
    other_value.aliases = {"--other-value"};
    other_value.description = "Description for other value";

    lwcli::CLIParser parser;
    parser.register_option(value);
    parser.register_option(other_value);

    // Control case (non-failing)
    EXPECT_TRUE(parse_succeeds(parser, std::array{"control", "--value", "10", "--other-value", "21.3"}));
    // Failing case:
    EXPECT_TRUE(parse_fails<lwcli::bad_value_conversion>(parser, GetParam()));
}

class BadPositionalUnhappyTests : public testing::TestWithParam<std::string>
{};

INSTANTIATE_TEST_SUITE_P(
    invalid_positional_count_tests,
    BadPositionalUnhappyTests,
    testing::Values("integration 10 20.123 30", "integration 10 20 30 40"));

TEST_P(BadPositionalUnhappyTests, BadConversion)
{
    lwcli::PositionalOption<int> value1;
    value1.name = "value1";
    value1.description = "Description for value 1";

    lwcli::PositionalOption<double> value2;
    value2.name = "value2";
    value2.description = "Description for value 2";

    lwcli::CLIParser parser;
    parser.register_option(value1);
    parser.register_option(value2);

    // Control case (non-failing)
    EXPECT_TRUE(parse_succeeds(parser, std::array{"control", "10", "20.123"}));
    // Failing case:
    EXPECT_TRUE(parse_fails<lwcli::bad_positional_count>(parser, GetParam()));
}

class BadKeyValueFormatUnhappyTests : public testing::TestWithParam<std::string>
{};

INSTANTIATE_TEST_SUITE_P(
    no_value_tests,
    BadKeyValueFormatUnhappyTests,
    testing::Values(
        "integration --value1 10 --value2-1",
        "integration --value1 10 --value2-2",
        "integration --value2-1 12.1 --value1"));

TEST_P(BadKeyValueFormatUnhappyTests, BadFormat)
{
    lwcli::KeyValueOption<int> value1;
    value1.aliases = {"--value1"};
    value1.description = "Description for value 1";

    lwcli::KeyValueOption<double> value2;
    value2.aliases = {"--value2-1", "--value2-2"};
    value2.description = "Description for value 2";

    lwcli::CLIParser parser;
    parser.register_option(value1);
    parser.register_option(value2);

    // Control case (non-failing)
    EXPECT_TRUE(parse_succeeds(parser, std::array{"control", "--value1", "10", "--value2-1", "12.1"}));
    // Failing case:
    EXPECT_TRUE(parse_fails<lwcli::bad_key_value_format>(parser, GetParam()));
}

class RequiredKeyValueTests : public testing::TestWithParam<std::string>
{};

INSTANTIATE_TEST_SUITE_P(
    invalid_required_key_value_inputs,
    RequiredKeyValueTests,
    testing::Values("integration --required1 10", "integration --required2 10"));

TEST_P(RequiredKeyValueTests, RequiredKeyValueOptionsUnhappy)
{
    lwcli::KeyValueOption<int> required_1;
    required_1.aliases = {"--required1"};
    required_1.description = "Description for required key-value option 1";

    lwcli::KeyValueOption<int> required_2;
    required_2.aliases = {"--required2"};
    required_2.description = "Description for required key-value option 2";

    lwcli::CLIParser parser;
    parser.register_option(required_1);
    parser.register_option(required_2);

    // Control case (non-failing)
    EXPECT_TRUE(parse_succeeds(parser, std::array{"control", "--required1", "10", "--required2", "20"}));
    // Failing case:
    EXPECT_TRUE(parse_fails<lwcli::bad_required_options>(parser, GetParam()));
}
