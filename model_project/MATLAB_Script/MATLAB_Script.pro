QT = core
CONFIG += c++17 cmdline
CONFIG += dll
TEMPLATE = lib

# 定义DLL导出宏
DEFINES += SYSTEMVUEMODELBUILDER_EXPORTS
DESTDIR = $$PWD/../../bin/models
INCLUDEPATH += $$PWD/../../ModelDesign/
include($$PWD/../../ModelDesign/openBlas/openBlas.pri)
win32 {
    LIBS += -L$$PWD/../../ModelDesign/lib -lDataStream
}

linux {
    LIBS += -L$$PWD/../../ModelDesign/lib -lDataStream
    DEFINES += QT_NO_DEBUG_OUTPUT
    LIBS += -lm
    CONFIG += unversioned_libname
    CONFIG -= plugin
    CONFIG -= create_prl
    CONFIG -= shared

    # 明确指定目标文件名
    TARGET = $$basename(PWD)
    QMAKE_EXTENSION_SHLIB = so

    # 清除版本相关设置
    QMAKE_LFLAGS_SONAME =
    VERSION =
    QMAKE_CXXFLAGS = -std=c++17 -Wno-unused-variable -fPIC
    QMAKE_CFLAGS = -std=c11
    CONFIG += c++17
    CONFIG += c++1z
    DEFINES += LINUX_PLATFORM
    CONFIG += unversioned_libname unversioned_soname
    QMAKE_CXXFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden

    # Octave 9.4.0 配置
    OCTAVE_ROOT = /usr/local/octave-9.4.0

    # 包含头文件
    INCLUDEPATH += $$OCTAVE_ROOT/include/octave-9.4.0

#    INCLUDEPATH +=  /usr/local/octave/include/octave-9.4.0

    # 链接库
    LIBS += -L$$OCTAVE_ROOT/lib/octave/9.4.0 -loctinterp -loctave

    # 设置运行时库路径
    QMAKE_LFLAGS += -Wl,-rpath,$$OCTAVE_ROOT/lib/octave/9.4.0
    QMAKE_LFLAGS += -Wl,-rpath,$$OCTAVE_ROOT/lib
}
win32{
    # Octave 9.4.0 配置
    OCTAVE_DIR = "D:\Program Files\GNU Octave\Octave-9.4.0"
   INCLUDEPATH += $$quote($$OCTAVE_DIR/mingw64/include/octave-9.4.0)
#   INCLUDEPATH += $$quote($$OCTAVE_DIR/mingw64/include/octave-9.4.0/octave)

# 如果需要，还要添加库路径
    LIBS += -L$$quote($$OCTAVE_DIR/mingw64/lib/octave/9.4.0/)

    LIBS += -loctave \
            -loctgui \
            -loctinterp
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        MATLAB_Script.cpp \
        MATLAB_Script_Block.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    MATLAB_Script.h \
    MATLAB_Script_Block.h

#include(D:/work/temPrj/GWDataFlowSimulator/ModelDesign/ModelDesign.pri)
#include($$PWD/../ModelDesign/ModelDesign.pri)
#include(D:\work_grxw\code\GWDataFlowSimulator\ModelDesign\ModelDesign.pri)
