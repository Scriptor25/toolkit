#pragma once

#include <string>
#include <vector>

namespace toolkit
{
    template<typename S, typename D>
    void split(std::vector<S> &vec, S &&str, D delim)
    {
        vec.clear();

        size_t b{}, e{};
        for (; (e = str.find(delim, b)) != S::npos; b = e)
            if (b != e)
                vec.push_back(str.substr(b, e - b));

        if (b != e)
            vec.push_back(str.substr(b, e - b));
    }

    template<typename S, typename D>
    std::vector<S> split(S &&str, D delim)
    {
        std::vector<S> vec;

        size_t b{}, e{};
        for (; (e = str.find(delim, b)) != S::npos; b = e)
            if (b != e)
                vec.push_back(str.substr(b, e - b));

        if (b != e)
            vec.push_back(str.substr(b, e - b));

        return vec;
    }

    template<typename S, typename D>
    void join(S &str, const std::vector<S> &vec, D delim)
    {
        str.clear();

        for (auto it = vec.begin(); it != vec.end(); ++it)
        {
            if (it != vec.begin())
                str += delim;

            str += *it;
        }
    }

    template<typename S, typename D>
    S join(const std::vector<S> &vec, D delim)
    {
        S str;

        for (auto it = vec.begin(); it != vec.end(); ++it)
        {
            if (it != vec.begin())
                str += delim;

            str += *it;
        }

        return str;
    }

    template<typename S>
    void trim(S &dst, S &&src)
    {
        typename S::iterator begin, end;

        for (auto it = src.begin(); it != src.end(); ++it)
            if (*it > 0x20)
            {
                begin = it;
                break;
            }

        for (auto it = src.rbegin(); it != src.rend(); ++it)
            if (*it > 0x20)
            {
                end = it.base();
                break;
            }

        dst = { begin, end };
    }

    template<typename S>
    S trim(S &&src)
    {
        typename S::iterator begin, end;

        for (auto it = src.begin(); it != src.end(); ++it)
            if (*it > 0x20)
            {
                begin = it;
                break;
            }

        for (auto it = src.rbegin(); it != src.rend(); ++it)
            if (*it > 0x20)
            {
                end = it.base();
                break;
            }

        return { begin, end };
    }

    template<typename S>
    S lowercase(S str)
    {
        for (auto &c : str)
            c = std::tolower(c);
        return str;
    }

    template<typename S>
    S uppercase(S str)
    {
        for (auto &c : str)
            c = std::toupper(c);
        return str;
    }
}
