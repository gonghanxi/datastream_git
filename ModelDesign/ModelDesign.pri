QT += core
QT += network serialport

INCLUDEPATH += \
    $$PWD
DEFINES += ADD_
DEFINES += HAVE_LAPACK_CONFIG_H
DEFINES += LAPACK_COMPLEX_STRUCTURE


include($$PWD/../Common/Common.pri)
SOURCES += \
        $$PWD/Block.cpp \
        $$PWD/BlockPortImpl.cpp \
        $$PWD/BlockSinkImpl.cpp \
        $$PWD/Buffer.cpp \
        $$PWD/BufferBusDataImpl.cpp \
        $$PWD/BufferExpansionImpl.cpp \
        $$PWD/BufferMemoryImpl.cpp \
        $$PWD/BufferReader.cpp \
        $$PWD/BufferReaderDataReadImpl.cpp \
        $$PWD/BufferReadImpl.cpp \
        $$PWD/BufferWriteImpl.cpp \
        $$PWD/BusConnection.cpp \
        $$PWD/CDFInterfaceImplementation.cpp \
        $$PWD/CDFParamImplementation.cpp \
        $$PWD/CDFPortImplementation.cpp \
        $$PWD/configmanager.cpp \
        $$PWD/DataStreamVerification.cpp \
        $$PWD/DataTypesAndParsers.cpp \
        $$PWD/DFErrorHandler.cpp \
        $$PWD/DFInterface.cpp \
        $$PWD/DFModel.cpp \
        $$PWD/DFParam.cpp \
        $$PWD/DFPort.cpp \
        $$PWD/Dll_templates.cpp \
        $$PWD/DynamicControlImplementation.cpp \
        $$PWD/LapackMatWrapper.cpp \
        $$PWD/RegisterModel.cpp \
        $$PWD/SimulationControl.cpp \
        $$PWD/SinkControlImplementation.cpp \
        $$PWD/openBlas/LapackMat.cpp \
        $$PWD/openBlas/LapackMatBase.cpp \

HEADERS += \
    $$PWD/Block.h \
    $$PWD/BlockPortImpl.h \
    $$PWD/BlockSinkImpl.h \
    $$PWD/Buffer.h \
    $$PWD/BufferBusDataImpl.h \
    $$PWD/BufferExpansionImpl.h \
    $$PWD/BufferMemoryImpl.h \
    $$PWD/BufferReader.h \
    $$PWD/BufferReaderDataReadImpl.h \
    $$PWD/BufferReadImpl.h \
    $$PWD/BufferWriteImpl.h \
    $$PWD/BusConnection.h \
    $$PWD/CDFInterfaceImplementation.h \
    $$PWD/CDFParamImplementation.h \
    $$PWD/CDFPortImplementation.h \
    $$PWD/CircularBuffer.h \
    $$PWD/configmanager.h \
    $$PWD/DataStreamVerification.h \
    $$PWD/DataTypesAndParsers.h \
    $$PWD/DFEnumerations.h \
    $$PWD/DFErrorHandler.h \
    $$PWD/DFInterface.h \
    $$PWD/DFModel.h \
    $$PWD/DFParam.h \
    $$PWD/DFPort.h \
    $$PWD/DynamicControlImplementation.h \
    $$PWD/LapackMatWrapper.h \
    $$PWD/EnumTypeConverter.h \
    $$PWD/EnvelopeSignal.h \
    $$PWD/eresult.h \
    $$PWD/FixedPointEnums.h \
    $$PWD/Matrix.h \
    $$PWD/MatrixCircularBuffer.h \
    $$PWD/MatrixMathFunction.h \
    $$PWD/ModelBuilder.h \
    $$PWD/RegisterModel.h \
    $$PWD/SimulationControl.h \
    $$PWD/SinkControlImplementation.h \
    $$PWD/SystemVue.h \
    $$PWD/SystemVueModels.h \
    $$PWD/TimedCircularBuffer.h \
    $$PWD/TimedDFModel.h \
    $$PWD/configmanager.h \
    $$PWD/eresult.h \
    $$PWD/iir/Iir.h \
    $$PWD/openBlas/LapackMat.h \
    $$PWD/openBlas/LapackMatBase.h

DEFINES += SYSTEMVUEMODELBUILDER_LIB

