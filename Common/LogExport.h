#ifndef LOGEXPORT_H
#define LOGEXPORT_H
#include <iostream>
#include <string> // 打印std::string需要，纯基础类型可省略

#define ENABLE_DEBUG 1  // 0=关闭DEBUG日志  |  1=开启DEBUG日志

// 1. 递归终止函数：参数包解析完毕，接收流对象但无操作
inline void logWrite(std::ostream&) {}

// 2. 核心可变参数模板函数【带流对象参数】：完美适配 cout/cerr，无硬编码！
template<typename T, typename... Args>
inline void logWrite(std::ostream& os, const T& val, const Args&... args) {
    os << val;          // 使用传入的流对象输出，不再硬编码cout
    logWrite(os, args...); // 递归传递：流对象 + 剩余参数
}

#define LOG_INFO(...)  do{ std::cout << "[INFO]"; logWrite(std::cout, __VA_ARGS__); std::cout << std::endl; }while(0)
#define LOG_WARN(...)  do{ std::cout << "[WARN]"; logWrite(std::cout, __VA_ARGS__); std::cout << std::endl; }while(0)
#define LOG_ERROR(...) do{ std::cerr << "[ERROR]"; logWrite(std::cerr, __VA_ARGS__); std::cerr << std::endl; }while(0)
#if ENABLE_DEBUG
#define LOG_DEBUG(...) do{ std::cout << "[DEBUG]"; logWrite(std::cout, __VA_ARGS__); std::cout << std::endl; }while(0)
#else
#define LOG_DEBUG(...) do{}while(0)  // 关闭后：空宏，什么都不执行
#endif

//示例：
//LOG_INFO("程序启动成功，版本号：", 2.0, " 运行模式：正式版");

#endif // LOGEXPORT_H
