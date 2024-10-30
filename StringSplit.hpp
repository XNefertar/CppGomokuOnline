#ifndef _STRING_SPLIT_HPP_
#define _STRING_SPLIT_HPP_

// 字符串分割接口封装
#include <iostream>
#include <vector>
#include <string>

class StringSplit
{
public:
    // input: 待分割的字符串
    // split_char: 分割字符
    // output: 分割结果
    static int Split(const std::string& input, const std::string& split_char, std::vector<std::string>& output)
    {
        output.clear();
        size_t start = 0;
        size_t pos = 0;
        while((pos = input.find(split_char, start)) != std::string::npos)
        {
            output.push_back(input.substr(start, pos - start));
            start = pos + split_char.size();
        }
        return output.size();
    }
};




#endif // _STRING_SPLIT_HPP_