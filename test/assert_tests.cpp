#include "gtest/gtest.h" // cppcheck-suppress [missingInclude]

#include "LWCLI/options.hpp"
#include "LWCLI/parser.hpp"

TEST(AssertTests, MultiOptionsSameKey)
{
    lwcli::FlagOption option1;
    option1.aliases = {"--value1", "--value2"};

    lwcli::FlagOption option2;
    option2.aliases = {"--value2", "--value3"};

    lwcli::CLIParser parser;
    parser.register_option(option1);
    EXPECT_DEATH(parser.register_option(option2), "Duplicate option alias detected.");
}

TEST(AssertTests, MultiOptionsSameKeyDifferentOptionTypes)
{
    lwcli::FlagOption option1;
    option1.aliases = {"--value1", "--value2"};

    lwcli::KeyValueOption<int> option2;
    option2.aliases = {"--value2", "--value3"};

    lwcli::CLIParser parser;
    parser.register_option(option1);
    EXPECT_DEATH(parser.register_option(option2), "Duplicate option alias detected.");
}

TEST(AssertTests, FailOnEmptyAlias)
{
    lwcli::FlagOption err_option;
    err_option.aliases = {"--value1", "", "--value2"};

    lwcli::CLIParser parser;
    // Note: Not checking for error message here because code is sufficiently clear.
    EXPECT_DEATH(parser.register_option(err_option), ".*");
}

TEST(AssertTests, FailOnAliasWithSpace)
{
    lwcli::FlagOption err_option;
    err_option.aliases = {"--value1", "--val ue2"};

    lwcli::CLIParser parser;
    EXPECT_DEATH(parser.register_option(err_option), "Aliases should contain no spaces.");
}
