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
    QMAKE_CXXFLAGS = -std=c++17 -Wno-unused-variable -fPIC
    QMAKE_CFLAGS = -std=c11
    CONFIG += c++17
    CONFIG += c++1z
    # 清除版本相关设置
    QMAKE_LFLAGS_SONAME =
    VERSION =
}

DEFINES += QT_DEPRECATED_WARNINGS

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    MxDeCom_M.h \
    MxDeCom_M_Block.h

SOURCES += \
    MxDeCom_M.cpp \
    MxDeCom_M_Block.cpp
