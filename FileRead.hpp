#ifndef _FILE_READ_HPP_
#define _FILE_READ_HPP_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "Logger.hpp"

#define DEFAULT_OPEN_MODE std::ios::in | std::ios::binary


class FileRead
{
public:
    // 读取文件内容
    // file_path: 文件路径
    // content: 文件内容，输出参数
    static bool Read(const std::string &file_path, std::string &content, std::ios_base::openmode mode = DEFAULT_OPEN_MODE)
    {
        // 1. 打开文件
        std::ifstream file;
        file.open(file_path, mode);
        if(!file)
        {
            // Log::LogMessage(ERROR, "OPEN FILE ERROR");
            return false;
        }

        // 计算文件大小
        file.seekg(0, std::ios::end); // 将文件指针移动到文件末尾
        content.resize(file.tellg()); // 重设content大小
        file.seekg(0, std::ios::beg); // 将文件指针移动到文件开头

        // 2. 读取文件内容
        file.read(&content[0], content.size());

        if(file.bad())
        {
            // Log::LogMessage(ERROR, "READ FILE ERROR");
            return false;
        }
        file.close();
        return true;
    }
};




#endif // _FILE_READ_HPP_