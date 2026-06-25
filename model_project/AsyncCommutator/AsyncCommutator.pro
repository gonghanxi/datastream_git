QT = core
CONFIG += c++17 cmdline
TEMPLATE = lib

DEFINES += SYSTEMVUEMODELBUILDER_EXPORTS
DESTDIR = $$PWD/../../bin/models
INCLUDEPATH += $$PWD/../../ModelDesign/
include($$PWD/../../ModelDesign/openBlas/openBlas.pri)

win32 {
    CONFIG += dll
    LIBS += -L$$PWD/../../ModelDesign/lib -lDataStream
}

linux {
    LIBS += -L$$PWD/../../ModelDesign/lib -lDataStream
    DEFINES += QT_NO_DEBUG_OUTPUT
    LIBS += -lm
    CONFIG += shared
    CONFIG += unversioned_libname
    TARGET = $$basename(PWD)
    QMAKE_EXTENSION_SHLIB = so
    QMAKE_CXXFLAGS = -std=c++17 -Wno-unused-variable -fPIC
    QMAKE_CFLAGS = -std=c11
    CONFIG += c++17
    QMAKE_LFLAGS_SONAME =
    VERSION =
}

DEFINES += QT_DEPRECATED_WARNINGS

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    AsyncCommutator.h \
    AsyncCommutator_Block.h

SOURCES += \
    AsyncCommutator.cpp \
    AsyncCommutator_Block.cpp
