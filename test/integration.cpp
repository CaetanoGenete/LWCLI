#include "gtest/gtest.h"

#include <ranges>
#include <string>
#include <sstream>

#include "LWCLI/LWCLI.hpp"

TEST(integration, all_options_types_happy)
{
    lwcli::FlagOption flag_option;
    flag_option.aliases = { "-v" };

    lwcli::KeyValueOption<int> key_value_option;
    key_value_option.aliases = { "--value" };

    lwcli::PositionalOption<double> positional_option;

    lwcli::CLIParser parser;
    parser.register_option(flag_option);
    parser.register_option(key_value_option);
    parser.register_option(positional_option);

    constexpr const char* value = "10";
    constexpr const char* positional = "0.31415926";
    constexpr const char* argv[] = { "integration", "-v", "--value", value, positional };

    ASSERT_NO_THROW(parser.parse(static_cast<int>(std::size(argv)), argv));
    ASSERT_EQ(1, flag_option.count);
    ASSERT_EQ(std::stoi(value), key_value_option.value);
    ASSERT_EQ(std::stod(positional), positional_option.value);
}

struct key_value_unhappy_tests : public testing::TestWithParam<std::string> {};

INSTANTIATE_TEST_SUITE_P(
    unhappy_key_value_args,
    key_value_unhappy_tests,
    testing::Values(
        "integration --value non-int-value",
        "integration --value non-int-value",
        "integration --value --other-value 10",
        "integration --undefined-key 10"));

TEST_P(key_value_unhappy_tests, key_value_unhappy)
{
    lwcli::KeyValueOption<int> key_value;
    key_value.aliases = { "--value" };

    lwcli::CLIParser parser;
    parser.register_option(key_value);

    // Control case (non-failing)

    constexpr const char* control_argv[] = { "control", "--value", "10" };
    constexpr const auto control_argc = std::size(control_argv);
    EXPECT_NO_THROW(parser.parse(control_argc, control_argv));

    // Failing case:

    std::stringstream data(GetParam());

    std::vector<std::string> args;
    std::string line;
    while (std::getline(data, line, ' '))
        args.push_back(line);

    std::vector<const char*> cstr_args;
    cstr_args.reserve(std::size(args));
    for (const auto& arg : args)
        cstr_args.emplace_back(arg.c_str());

    const auto argc = static_cast<int>(std::size(args));
    const auto argv = std::data(cstr_args);
    ASSERT_THROW(parser.parse(argc, argv), lwcli::bad_parse);
}

TEST(interation, duplicate_flag_options_happy)
{
    lwcli::FlagOption flag_option;
    flag_option.aliases = { "--value1", "--value2", "--value3" };

    lwcli::FlagOption other_flag_option;
    other_flag_option.aliases = { "--other-value" };

    lwcli::CLIParser parser;
    parser.register_option(flag_option);
    parser.register_option(other_flag_option);

    constexpr const char* argv[] {
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
        "--value3"
    };
    constexpr auto argc = static_cast<int>(std::size(argv));

    ASSERT_NO_THROW(parser.parse(argc, argv));
    ASSERT_EQ(9, flag_option.count);
}
