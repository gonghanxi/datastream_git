#ifndef COMPILER_H
#define COMPILER_H

#include <QString>

/**
 * @brief 跨平台编译器封装
 *
 * Windows: 通过 vswhere 自动查找 Visual Studio，使用 MSVC 编译
 * Linux:   使用 g++ 编译
 */
class Compiler {
public:
    /**
     * @brief 编译生成的源代码为动态链接库
     * @param sourceDir      源代码所在目录
     * @param outputPath     输出动态库完整路径 (如 /path/to/model.dll)
     * @param isWindows      平台标识，true 为 Windows
     * @param fmiHeadersDir  FMI 头文件所在目录
     * @return 编译成功返回 true，失败返回 false
     */
    static bool compile(const QString &sourceDir,
                        const QString &outputPath,
                        bool isWindows,
                        const QString &fmiHeadersDir);
};

#endif // COMPILER_H
