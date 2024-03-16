#include "LWCLI/options.hpp"


struct test
{
    std::string value;
    void* blahh;
    void* something_else;
};

int main(int argc, const char** argv)
{
    lwcli::FlagOption _option1;
    _option1.aliases = {"-v", "--verbose"};

    lwcli::KeyValueOption<std::optional<int>> _option2;
    _option2.aliases = {"--value"};

    // lwcli::PositionalOption<double> _option3;

    lwcli::CLIParser parser;
    parser.register_option(_option1);
    parser.register_option(_option2);
    // parser.register_option(_option3);
    parser.parse(argc, argv);

    std::cout << _option1.count << std::endl;
    if(_option2.value.has_value()) {
        std::cout << _option2.value.value() << std::endl;
    }
    else {
        std::puts("No --value defined");
    }
}