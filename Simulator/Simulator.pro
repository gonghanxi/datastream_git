TARGET = SignalFlowSimulator
TEMPLATE = app
CONFIG += c++17 cmdline

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
DESTDIR = "$$PWD/../bin"
QT += core gui opengl svg printsupport network websockets concurrent

# 解决QDebug中文乱码/不显示核心配置
CONFIG += utf8_source


# 操作系统检测
#QMAKE_CXXFLAGS += /MDd
win32 {
    DEFINES += WINDOWS_PLATFORM
    # MSVC 编码配置
    QMAKE_CXXFLAGS += /utf-8
    QMAKE_CFLAGS += /utf-8
    # 使用动态运行时库（已正确）
    QMAKE_CXXFLAGS += /MD
    # Debug版本使用/MDd
    CONFIG(debug, debug|release) {
        QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CXXFLAGS_DEBUG
        QMAKE_CXXFLAGS -= /MD
        QMAKE_CXXFLAGS += /MDd
        DEFINES += _DEBUG
    }
    QMAKE_CXXFLAGS += /wd4005
    QMAKE_CFLAGS += /wd4005
}

linux {
#    DEFINES += QT_NO_DEBUG_OUTPUT
#    DEFINES += LINUX_PLATFORM
    # Linux 下不需要 /utf-8 参数
    # 添加必要的链接选项
    QMAKE_LFLAGS += -Wl,-rpath,\'\$$ORIGIN\'
    DEFINES += QT_NO_DEBUG_OUTPUT
    DEFINES += LINUX_PLATFORM
    # 强制使用 C++17，覆盖所有默认设置
    QMAKE_CXXFLAGS += -std=c++17 -Wno-unused-variable -fPIC
    QMAKE_CFLAGS = -std=c11
    CONFIG += c++17
    CONFIG += c++1z
    # 修复 moc 在 GCC 7.x 上解析 stl_relops.h 失败的问题
    # 定义 _GLIBCXX_VISIBILITY 为空宏，防止 moc 解析失败
    QMAKE_MOC = /usr/lib64/qt5/bin/moc -D_GLIBCXX_VISIBILITY=default

#    # 如果是 ARM64 架构（如麒麟）
#    contains(QMAKE_HOST.arch, "aarch64") {
#        DEFINES += ARM64_PLATFORM
#        message("Building for ARM64 architecture")
#    }
}

INCLUDEPATH += $$PWD/../ModelDesign
INCLUDEPATH += $$PWD/../SignalFlowSimulatorLib
INCLUDEPATH += $$PWD/../Common
INCLUDEPATH += $$PWD/DDS



SOURCES += \
    DDS/CommandHandler.cpp \
    DDS/WebSocketServer.cpp \
    StdinListener.cpp \
    main.cpp


FORMS += \


HEADERS += \ \
    DDS/CommandHandler.h \
    DDS/WebSocketServer.h \
    StdinListener.h


RESOURCES += \

