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

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    TransposeEnv.cpp \
    TransposeEnv_Block.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    TransposeEnv.h \
    TransposeEnv_Block.h
