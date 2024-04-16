#include "gtest/gtest.h"

#include "LWCLI/LWCLI.hpp"

TEST(assert_tests, multi_options_same_key)
{
    lwcli::FlagOption option1{
        .aliases = {"--value1", "--value2"},
    };
    lwcli::FlagOption option2{
        .aliases = {"--value2", "--value3"},
    };

    lwcli::CLIParser parser;
    parser.register_option(option1);
    EXPECT_DEATH(parser.register_option(option2), "Duplicate option alias detected.");
}

TEST(assert_tests, multi_options_same_key_different_option_types)
{
    lwcli::FlagOption option1;
    option1.aliases = {"--value1", "--value2"};

    lwcli::KeyValueOption<int> option2;
    option2.aliases = {"--value2", "--value3"};

    lwcli::CLIParser parser;
    parser.register_option(option1);
    EXPECT_DEATH(parser.register_option(option2), "Duplicate option alias detected.");
}

TEST(assert_tests, fail_on_empty_alias)
{
    lwcli::FlagOption err_option;
    err_option.aliases = {"--value1", "", "--value2"};

    lwcli::CLIParser parser;
    // Note: Not checking for error message here because code is sufficiently clear.
    EXPECT_DEATH(parser.register_option(err_option), ".*");
}

TEST(assert_tests, fail_on_alias_with_space)
{
    lwcli::FlagOption err_option;
    err_option.aliases = {"--value1", "--val ue2"};

    lwcli::CLIParser parser;
    EXPECT_DEATH(parser.register_option(err_option), "Aliases should contain no spaces.");
}
