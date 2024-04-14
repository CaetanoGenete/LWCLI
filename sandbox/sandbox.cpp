#include <iostream>

#include "LWCLI/LWCLI.hpp"

int main(int argc, const char** argv)
{
    lwcli::FlagOption _option1{
        .aliases = {"-v", "--verbose"},
        .description = "This does something actually usedful",
    };

    lwcli::KeyValueOption<int> _option2{
        .aliases = {"--value"},
        .description = "This does something useful lol.",
    };

    lwcli::CLIParser parser;
    parser.register_option(_option1);
    parser.register_option(_option2);

    try {
        parser.parse(argc, argv);
    }
    catch (const lwcli::bad_parse& e) {
        std::cerr << e.what() << std::endl;
        std::exit(1);
    }
}
