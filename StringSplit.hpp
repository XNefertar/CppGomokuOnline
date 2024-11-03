#ifndef _STRING_SPLIT_HPP_
#define _STRING_SPLIT_HPP_

// 字符串分割接口封装
#include <iostream>
#include <vector>
#include <string>
#include "Logger.hpp"

class StringSplit
{
public:
    // input: 待分割的字符串
    // split_char: 分割字符
    // output: 分割结果
    static int Split(const std::string& input, const std::string& split_char, std::vector<std::string>& output)
    {
        size_t start = 0;
        size_t pos = 0;
        while(start < input.size())
        {
            pos = input.find(split_char, start);
            if(pos == std::string::npos)
            {
                output.push_back(input.substr(start));
                break;
            }
            if(pos == start)
            {
                start += split_char.size();
                continue;
            }
            output.push_back(input.substr(start, pos - start));
            start = pos + split_char.size();
        }
        return output.size();
    }
};




#endif // _STRING_SPLIT_HPP_