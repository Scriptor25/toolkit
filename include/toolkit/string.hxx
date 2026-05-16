#pragma once

#include <string>
#include <vector>

namespace toolkit
{
    template<typename S, typename D>
    void split(std::vector<S> &vec, const S &str, D delim)
    {
        vec.clear();

        size_t b{}, e{};
        for (; (e = str.find(delim, b)) != S::npos; b = e)
            if (b != e)
                vec.push_back(str.substr(b, e - b));

        if (b != e)
            vec.push_back(str.substr(b, e - b));
    }
}
