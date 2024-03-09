#ifndef LWCLI_INCLUDE_LWCLI_OPTIONS_HPP
#define LWCLI_INCLUDE_LWCLI_OPTIONS_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

namespace lwcli
{

    struct FlagOption
    {
        using count_t = unsigned int;

        std::vector<std::string> aliases;
        count_t count = 0;
    };

    template<class Type>
    struct KeyValueOption
    {
        using value_t = Type;

        std::vector<std::string> aliases;
        value_t value;
    };

    template<class Type>
    struct PositionalOption
    {
        using value_t = Type;

        std::string name;
        unsigned int position;
        value_t value;
    };

    class Options
    {
    private:
        struct _AnyOption
        {
            using parse_func_t = unsigned int (*)(const char**, _AnyOption);

            parse_func_t parse;
            void* data;
        };

    private:
        _AnyOption::parse_func_t _option_parser(FlagOption& option)
        {
            return [](const char**, _AnyOption optionData) {
                ++(*(static_cast<FlagOption::count_t*>(optionData.data)));
                return 1u;
            };
        }

        template<class Type>
        _AnyOption::parse_func_t _option_parser(KeyValueOption<Type>& option)
        {
            return [](std::string, _AnyOption optionData) {
                std::cout << "parsing key value option" << std::endl;
                };
        }

    public:
        void add(FlagOption& option)
        {
            _options.reserve(_options.size() + option.aliases.size());
            for (const auto& alias : option.aliases) {
                _options.try_emplace(alias, _option_parser(option), &option.count);
            }
        }

        template<class Type>
        void add(KeyValueOption<Type>& option)
        {
            _options.reserve(_options.size() + option.aliases.size());
            for (const auto& alias : option.aliases) {
                _options.try_emplace(alias, _option_parser(option), &option.value);
            }
        }

        template<class Type>
        void add(PositionalOption<Type>& option)
        {
            _options.try_emplace(option.name, _option_parser(option), &option.value);
        }

    public:
        void parse(unsigned int argc, const char** argv)
        {
            const char** end = argv + argc;
            unsigned int skip = 0;
            do {
                argv += skip;
                const auto option = _options[*argv];
                std::invoke(option.parse, argv, option);
            }
            while(end - argv < skip);
        }

    private:
        std::unordered_map<std::string, _AnyOption> _options;
    };
}

#endif // LWCLI_INCLUDE_LWCLI_OPTIONS_HPP