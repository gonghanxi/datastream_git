#ifndef LIBRARYHELPER_H
#define LIBRARYHELPER_H

#include <QLibrary>
#include <Block.h>
#include <QDebug>

using namespace SystemVueModelBuilder;

// 跨平台函数指针类型定义
#ifdef _WIN32
    typedef Block* (*CreateFunction)();
    typedef const char* (*GetNameFunction)();  // Windows下返回const char*
#else
    typedef Block* (*CreateFunction)();
    typedef const char* (*GetNameFunction)();  // Linux下也是const char*
#endif

class LibraryHelper
{
public:
    LibraryHelper();
    explicit LibraryHelper(const QString& fileName);  // 使用explicit防止隐式转换
    ~LibraryHelper();

    // 获取错误信息
    QString getErrorString() const;

    // 创建算法实例
    Block* create();

    // 获取算法名称
    const char* getAlgorithmName();

private:
    void resolveFunctions();  // 解析函数

private:
    QLibrary *mLib = nullptr;
    CreateFunction createFunction = nullptr;
    GetNameFunction getNameFunction = nullptr;
    QString m_errorString;
};

#endif // LIBRARYHELPER_H
