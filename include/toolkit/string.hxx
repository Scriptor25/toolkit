#pragma once

#include <string>
#include <vector>

namespace toolkit
{
    template<typename S, std::convertible_to<std::string_view> D>
    void split(std::vector<std::decay_t<S>> &vec, S &&str, const D &delim)
    {
        vec.clear();

        size_t b{}, e{};
        for (; (e = str.find(delim, b)) != std::decay_t<S>::npos; b = e + delim.size())
            if (b != e)
                vec.push_back(str.substr(b, e - b));

        if (b != e)
            vec.push_back(str.substr(b, e - b));
    }

    template<typename S, typename D>
    void split(std::vector<std::decay_t<S>> &vec, S &&str, D delim)
    {
        vec.clear();

        size_t b{}, e{};
        for (; (e = str.find(delim, b)) != std::decay_t<S>::npos; b = e + 1)
            if (b != e)
                vec.push_back(str.substr(b, e - b));

        if (b != e)
            vec.push_back(str.substr(b, e - b));
    }

    template<typename S, typename D>
    std::vector<std::decay_t<S>> split(S &&str, D &&delim)
    {
        std::vector<std::decay_t<S>> vec;
        split(vec, std::forward<S>(str), std::forward<D>(delim));
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
        using I = std::decay_t<S>::iterator;

        I begin, end;

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
        using I = std::decay_t<S>::iterator;

        I begin, end;

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
