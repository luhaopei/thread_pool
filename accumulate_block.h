#pragma once
#include <numeric>

template<typename Iterator, typename T>
struct accumulate_block
{
    T operator()(Iterator first, Iterator last)
    {
        return std::accumulate(first, last, T());
    }
};
