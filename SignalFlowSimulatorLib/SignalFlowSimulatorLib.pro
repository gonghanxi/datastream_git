TARGET = SignalFlowSimulatorLib
TEMPLATE = lib
CONFIG += c++17 cmdline dll
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += core
DESTDIR = "$$PWD/../bin"
# 解决QDebug中文乱码/不显示核心配置
CONFIG += utf8_source
# 操作系统检测
win32 {
    DEFINES += WINDOWS_PLATFORM
    # MSVC 编码配置
    QMAKE_CXXFLAGS += /utf-8
    QMAKE_CFLAGS += /utf-8
    # MSVC编译选项（使用MSVC风格的警告控制）
    QMAKE_CXXFLAGS += /wd4996  # 禁用deprecated declarations警告
    QMAKE_CXXFLAGS += /wd4100  # 禁用unused parameter警告
    QMAKE_CXXFLAGS += /wd4514  # 禁用unreferenced inline function警告
    QMAKE_CXXFLAGS += /wd4828      # 禁用无效字符警告
    QMAKE_CXXFLAGS += /wd4267
}

linux {
#    DEFINES += LINUX_PLATFORM
    CONFIG += unversioned_libname unversioned_soname
#    DEFINES += QT_NO_DEBUG_OUTPUT
    # Linux下不需要/utf-8参数
    DEFINES += QT_NO_DEBUG_OUTPUT
    DEFINES += LINUX_PLATFORM
    # 强制使用 C++17，覆盖所有默认设置
    QMAKE_CXXFLAGS = -std=c++17 -Wno-unused-variable -fPIC
    QMAKE_CFLAGS = -std=c11
    CONFIG += c++17
    CONFIG += c++1z
}

# 解决QDebug中文乱码/不显示核心配置
CONFIG += utf8_source

DEFINES +=SignalFlowSimulatorLib_EXPORTS


# 包含 ModelDesign 的头文件路径
INCLUDEPATH += $$PWD/../ModelDesign

# 链接 ModelDesign 静态库
win32 {
    # Windows下链接静态库
    LIBS += -L$$PWD/../ModelDesign/lib -lDataStream
}

linux {
    # Linux下链接静态库
    LIBS += -L$$PWD/../ModelDesign/lib -lDataStream
    # Linux下需要添加数学库
    LIBS += -lm
}

# 包含矩阵计算库
include($$PWD/../ModelDesign/openBlas/openBlas.pri)
include($$PWD/../FMUManager/FMUManager.pri)
#LIBS += -L$$PWD/../ModelDesign/openBlas/lib/x64_release/openblas/lib -llibopenblas
##LIBS +=  -lLAPACK
#INCLUDEPATH +=  $$PWD/../ModelDesign/openBlas/lib/x64_release/openblas/include
#include($$PWD/../ModelDesign/ModelDesign.pri)
#include($$PWD/../ModelDesign/openBlas/openBlas.pri)

INCLUDEPATH += $$PWD/fmu
INCLUDEPATH += $$PWD/VariableAnalysis
INCLUDEPATH += $$PWD/DataFlowScheduler
INCLUDEPATH += $$PWD/ConnectionCondition
INCLUDEPATH += $$PWD/DDS

SOURCES +=\
    ConnectionCondition/ShortOpenProcessor.cpp \
    ConnectionValidator.cpp \
    DDS/SimEngineController.cpp \
    DataFlowScheduler/ReadyQueueScheduler.cpp \
    DataFlowScheduler/SimpleScheduler.cpp \
    DataFlowScheduler/TimeDrivenScheduler.cpp \
    ModelCompatCheck.cpp \
    PortValidatorImpl.cpp \
    VarExpressionParse.cpp \
    VariableAnalysis/ExpressionResolver.cpp \
    VariableAnalysis/LinkParser.cpp \
    VariableAnalysis/MathExpressionCalculator.cpp \
    VariableAnalysis/SubsystemParameterMapper.cpp \
    VariableAnalysis/VariableScopeManager.cpp \
    algorithmmanager.cpp \
    dataflowcheck.cpp \
    fmu/FMUBlock.cpp \
    fmu/FMUModelInfo.cpp \
    libraryhelper.cpp \
    signalflowlinksort.cpp \
    simrunner.cpp \
    unitconvert.cpp

HEADERS  += \
    ConnectionCondition/ShortOpenProcessor.h \
    ConnectionValidator.h \
    DDS/SimEngineController.h \
    DataFlowScheduler/ReadyQueueScheduler.h \
    DataFlowScheduler/SimpleScheduler.h \
    DataFlowScheduler/TimeDrivenScheduler.h \
    JsonLinkDefine.h \
    ModelCompatCheck.h \
    PortValidatorImpl.h \
    VarExpressionParse.h \
    VariableAnalysis/ExpressionResolver.h \
    VariableAnalysis/LinkParser.h \
    VariableAnalysis/MathExpressionCalculator.h \
    VariableAnalysis/SubsystemParameterMapper.h \
    VariableAnalysis/Variable.h \
    VariableAnalysis/VariableScopeManager.h \
    algorithmmanager.h \
    connection.h \
    dataflowcheck.h \
    fmu/FMUBlock.h \
    fmu/FMUModelInfo.h \
    libraryhelper.h \
    signalflowlinksort.h \
    simrunner.h \
    unitconvert.h

INCLUDEPATH += \


FORMS += \



