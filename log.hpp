#ifndef _LOG_HPP
#define _LOG_HPP

// 封装日志库
// 1. 日志级别
// 2. 日志文件
// 3. 日志文件最大数量
// 4. 日志文件最大大小
// 5. 日志文件权限
// 6. 日志文件路径
// 7. 日志文件打开模式
// 8. 日志消息
// 9. 日志初始化
// 10. 日志关闭
// 11. 日志消息

// 考虑线程安全
// 1. 互斥锁
// 2. 互斥锁保护
// 3. 互斥锁保护的代码块

#include <iostream>
#include <string>
#include <mutex>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define NORMAL 0
#define WARNING 1
#define ERROR 2
#define FATAL 3
#define DEBUG 4
#define INFO 5

#define LOG_MAX_NUM 5
#define LOG_LEVEL int
#define LOG_FILE_PERMISSION 0666
#define LOG_MAX_SIZE 1024
#define LOG_FILE_PATH "./log.txt"
#define LOG_MESSAGE const char *
#define LOG_FILE_MODE O_CREAT | O_WRONLY | O_APPEND

namespace LOG_MSG
{
    class Log
    {
    private:
        static std::mutex mtx;
        static LOG_LEVEL currentLogLevel;
        static FILE *logFile;

    private:
        static std::string to_log(LOG_LEVEL level)
        {
            switch (level)
            {
            case NORMAL:
                return "NORMAL";
            case WARNING:
                return "WARNING";
            case ERROR:
                return "ERROR";
            case FATAL:
                return "FATAL";
            case DEBUG:
                return "DEBUG";
            case INFO:
                return "INFO";
            default:
                return "UNKNOWN";
            }
        }

    public:
        static void LogInit(const char *log_file_path, LOG_LEVEL level)
        {
            currentLogLevel = level;
            logFile = fopen(log_file_path, "a+");
            if (!logFile)
            {
                std::cerr << "OPEN ERROR" << std::endl;
                return;
            }
        }

        static void LogClose()
        {
            if (logFile)
            {
                fclose(logFile);
                logFile = nullptr;
            }
        }

        static void LogMessage(LOG_LEVEL level, LOG_MESSAGE message, ...)
        {
            if (level > currentLogLevel || !logFile)
                return; // 过滤日志级别和文件指针检查

            std::lock_guard<std::mutex> lock(mtx);

            va_list args;
            va_start(args, message);

            char buffer[1024]{};
            sprintf(buffer, "[%s][%ld][%ld] ", to_log(level).c_str(), time(nullptr), getpid());

            char response[1024]{};
            vsprintf(response, message, args);

            fprintf(logFile, "%s%s\n", buffer, response); // 写入日志文件

            va_end(args);
        }
    };
    // 静态成员初始化
    LOG_LEVEL Log::currentLogLevel = NORMAL;
    FILE *Log::logFile = nullptr;
    std::mutex Log::mtx;

}
#endif // _LOG_HPP_