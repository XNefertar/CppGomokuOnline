#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <jsoncpp/json/json.h>


int main()
{
    // 序列化
    Json::Value student;
    student["name"] = "zhangsan";
    student["age"] = 18;
    // append 用于添加数组元素
    student["socre"].append(99);
    student["socre"].append(99.5);
    student["socre"].append(100);

    Json::StreamWriterBuilder writerBuilder;
    std::unique_ptr<Json::StreamWriter> sw(writerBuilder.newStreamWriter());
    std::stringstream ss;
    // 序列化接口 -> write
    int ret = sw->write(student, &ss);
    // 序列化失败
    if(ret != 0)
    {
        std::cerr << "Serialize error!" << std::endl;
        return -1;
    }
    // 序列化成功，打印序列化结果
    std::cout << "Serialize result: " << std::endl << std::endl;
    std::cout << ss.str() << std::endl;

    // 反序列化
    std::string str = ss.str();
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());

    std::cout << "Deserialize result: " << std::endl << std::endl;

    // 执行反序列化
    bool res = reader->parse(str.data(), str.data() + str.size(), &root, nullptr);
    // 反序列化失败
    if(!res)
    {
        std::cerr << "Deserialize error!" << std::endl;
        return -1;
    }
    // 打印反序列化结果
    std::cout << "name: " << root["name"].asString() << std::endl;
    std::cout << "age: " << root["age"].asInt() << std::endl;
    std::cout << "socre: " << std::endl;
    for(int i = 0; i < root["socre"].size(); ++i)
    {
        std::cout << root["socre"][i].asFloat() << " ";
    }
    std::cout << std::endl;
    return 0;
}