// #include "LWCLI/options.hpp"
#include <fstream>
#include <ctime>
#include <vector>
#include <random>
#include <iostream>
#include <functional>
#include <chrono>

struct test
{
    std::string value;
    void* blahh;
    void* something_else;
};

int main(unsigned int argc, const char** argv)
{
    using namespace std::chrono;

    std::default_random_engine eng{0};
    std::uniform_int_distribution dist;
    auto rand = std::bind(dist, std::ref(eng));

    std::vector<std::string> strings;
    for (unsigned int n = 100; n <= 100; n += 100) {
        strings.clear();
        for(unsigned int i = 0; i < n; ++i) {
            unsigned int size = 10 + (rand() % 20);
            std::string str = "--";
            str.reserve(str.size() + size);
            for(unsigned int j = 0; j < size; ++j)
                str += (char) (('A' + rand() % ('Z' - 'A' + 1)) ^ ((rand() % 2) * 32));
        }
        std::vector<test> list;
        list.reserve(strings.size());
        for(auto& str : strings)
            list.emplace_back(str, nullptr, nullptr);

        auto prev = system_clock().now();
        std::cout << duration_cast<milliseconds>(system_clock().now() - prev) << "\n";

    }
}