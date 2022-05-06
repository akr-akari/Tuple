#define D_AKR_TEST
#include "akr_test.hh"

#include "../tuple.hh"

#include <iostream>

int main()
{
    {
        auto t1 = akr::Tuple();

        auto t2 = akr::Tuple<>::Create();

        auto t3 = akr::Tuple(true, 'A', 123, 3.14, "ABC", nullptr);

        auto t4 = akr::Tuple<>::Create(true, 'A', 123, 3.14, "ABC", nullptr);
    }
    {
        auto t1 = akr::Tuple(1, 2, 3);

        t1 = akr::Tuple(4, 5, 6);
    }
    {
        auto t1 = akr::Tuple(1) & 2 & 3;
    }
    {
        auto t1 = akr::Tuple(1) + akr::Tuple(2) + akr::Tuple(3);
    }
    {
        auto t1 = akr::Tuple<>::Concat(akr::Tuple(), true, 'A', 123, akr::Tuple(3.14, "ABC"), nullptr);
    }
    {
        auto t1 = akr::Tuple<>::Create();

        auto t2 = akr::Tuple<>::Create(true, 'A', 123, 3.14, "ABC", nullptr);
    }
    {
        auto t1 = akr::Tuple(true, 'A', 123, 3.14, "ABC", nullptr);

        t1.ForEach(   [](auto&& e) { std::cout << std::boolalpha << e << '\n'; });
    }
    {
        auto t1 = akr::Tuple(true, 'A', 123, 3.14, "ABC", nullptr);

        t1.IndexBy(0, [](auto&& e) { std::cout << std::boolalpha << e << '\n'; });

        try
        {
            t1.IndexBy(6, [](auto&&) {});
        }
        catch (const std::out_of_range& e)
        {
            std::cout << e.what() << '\n';
        }
    }
    {
        auto t1 = akr::Tuple(true, 'A', 123, 3.14, "ABC", nullptr);

        t1.IndexOf<0>() = false;
    }
}
