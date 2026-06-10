QT += core network serialport

CONFIG += c++17 cmdline
TEMPLATE = lib
CONFIG += staticlib
TARGET = DataStream
DESTDIR = $$PWD/lib



# 操作系统检测
win32 {
    DEFINES += WINDOWS_PLATFORM
    QMAKE_CXXFLAGS += /utf-8

    # 禁用 C4819 警告
    QMAKE_CXXFLAGS += /wd4819
}

linux {
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
}

# 包含矩阵计算库
include($$PWD/openBlas/openBlas.pri)
#LIBS += -L$$PWD/openBlas/lib/x64_release/openblas/lib -llibopenblas
##LIBS +=  -lLAPACK
#INCLUDEPATH +=  $$PWD/openBlas/lib/x64_release/openblas/include
DEFINES += ADD_
DEFINES += HAVE_LAPACK_CONFIG_H
DEFINES += LAPACK_COMPLEX_STRUCTURE

INCLUDEPATH += $$PWD/Fixpoint

SOURCES += \
        Block.cpp \
        BlockPortImpl.cpp \
        BlockSinkImpl.cpp \
        Buffer.cpp \
        BufferBusDataImpl.cpp \
        BufferExpansionImpl.cpp \
        BufferMemoryImpl.cpp \
        BufferReadImpl.cpp \
        BufferReader.cpp \
        BufferReaderDataReadImpl.cpp \
        BufferWriteImpl.cpp \
        BusConnection.cpp \
        CDFInterfaceImplementation.cpp \
        CDFParamImplementation.cpp \
        CDFPortImplementation.cpp \
        DFErrorHandler.cpp \
        DFInterface.cpp \
        DFModel.cpp \
        DFParam.cpp \
        DFPort.cpp \
        DataStreamVerification.cpp \
        DataTypesAndParsers.cpp \
        Dll_templates.cpp \
        DynamicControlImplementation.cpp \
        Fixpoint/FixedPointBitRef.cpp \
        LapackMatWrapper.cpp \
        RegisterModel.cpp \
        SimulationControl.cpp \
        SinkControlImplementation.cpp \
        configmanager.cpp \
        Fixpoint/FixedPoint.cpp \
        Fixpoint/FixedPointParameters.cpp \
        Fixpoint/FixedPointValue.cpp \
        Fixpoint/FixedPointObserver.cpp
#        main.cpp
#        openBlas/LapackMat.cpp \
#        openBlas/LapackMatBase.cpp
HEADERS += \
    Block.h \
    BlockPortImpl.h \
    BlockSinkImpl.h \
    Buffer.h \
    BufferBusDataImpl.h \
    BufferExpansionImpl.h \
    BufferMemoryImpl.h \
    BufferReadImpl.h \
    BufferReader.h \
    BufferReaderDataReadImpl.h \
    BufferWriteImpl.h \
    BusConnection.h \
    CDFInterfaceImplementation.h \
    CDFParamImplementation.h \
    CDFPortImplementation.h \
    CircularBuffer.h \
    DFEnumerations.h \
    DFErrorHandler.h \
    DFInterface.h \
    DFModel.h \
    DFParam.h \
    DFPort.h \
    DataStreamVerification.h \
    DataTypes.h \
    DataTypesAndParsers.h \
    DynamicControlImplementation.h \
    EnumTypeConverter.h \
    EnvelopeSignal.h \
    LapackMatWrapper.h \
    Matrix.h \
    MatrixCircularBuffer.h \
    MatrixMathFunction.h \
    ModelBuilder.h \
    RandomNumberGenerator.h \
    RegisterModel.h \
    SimulationControl.h \
    SinkControlImplementation.h \
    StringUtils.h \
    SystemVue.h \
    SystemVueModels.h \
    TimedCircularBuffer.h \
    TimedDFModel.h \
    configmanager.h \
    eresult.h \
    Fixpoint/FixedPoint.h \
    Fixpoint/FixedPointParameters.h \
    Fixpoint/FixedPointEnums.h \
    Fixpoint/FixedPointValue.h \
    Fixpoint/FixedPointBitRef.h \
    Fixpoint/FixedPointObserver.h \
    Fixpoint/FixedPointStruct.h \
    Fixpoint/FixedPointRep.h \
    Fixpoint/DFFixedPointInterface.h \
    Fixpoint/DLL_Export/SystemC-FixedPoint.h
#    openBlas/LapackMat.h \
#    openBlas/LapackMatBase.h

# 安装配置
unix:!android {
    target.path = /usr/local/lib
    INSTALLS += target
}


