# FMUManager.pri

INCLUDEPATH += \
    $$PWD
	
DEFINES += ADD_
DEFINES += HAVE_LAPACK_CONFIG_H
DEFINES += LAPACK_COMPLEX_STRUCTURE

SOURCES += \
    $$PWD/FMUManager.cpp \
    $$PWD/fmu.cpp

# 头文件
HEADERS += \
    $$PWD/FMUManager.h \
    $$PWD/fmi2FunctionTypes.h \
    $$PWD/fmi2Functions.h \
    $$PWD/fmi2TypesPlatform.h \
    $$PWD/fmu.h