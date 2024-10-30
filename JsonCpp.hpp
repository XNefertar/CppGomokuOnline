#ifndef _JSON_CPP_HPP_
#define _JSON_CPP_HPP_

// 封装JsonCpp库
#include <jsoncpp/json/json.h>
#include <iostream>
#include <sstream>
#include <string>

class JsonCpp
{
public:
    // 序列化
    // root: 待序列化的Json对象
    // str: 序列化结果，输出参数
    static bool serialize(const Json::Value root, std::string& str)
    {
        Json::StreamWriterBuilder writerBuilder;
        std::unique_ptr<Json::StreamWriter> sw(writerBuilder.newStreamWriter());
        std::stringstream ss;
        // 序列化接口 -> write
        int ret = sw->write(root, &ss);
        // 序列化失败
        if(ret != 0)
        {
            std::cerr << "Serialize error!" << std::endl;
            return false;
        }
        str = ss.str();
        return true;
    }

    static bool deserialize(const std::string& str, Json::Value& root)
    {
        Json::CharReaderBuilder readerBuilder;
        std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
        // 执行反序列化
        bool res = reader->parse(str.data(), str.data() + str.size(), &root, nullptr);
        // 反序列化失败
        if(!res)
        {
            std::cerr << "Deserialize error!" << std::endl;
            return false;
        }
        return true;
    }

};



#endif // _JSON_CPP_HPP_