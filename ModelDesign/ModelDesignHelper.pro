# 添加一个条件变量
CONFIG += helper_only_for_ide

# 如果是IDE代码分析模式，跳过构建
helper_only_for_ide {
    TEMPLATE = aux  # aux模板不产生构建目标
} else {
    TEMPLATE = lib
}

QT += core network serialport
CONFIG += c++17

include($$PWD/ModelDesign.pri)
