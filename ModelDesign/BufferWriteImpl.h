#ifndef BUFFERWRITEIMPL_H
#define BUFFERWRITEIMPL_H

#include "Buffer.h"
#include "CircularBuffer.h"
#include "EnvelopeSignal.h"
#include "DataTypesAndParsers.h"

#include <vector>
#include <memory>
#include <iostream>
namespace SystemVueModelBuilder {
class BufferWriteImpl
{
private:
    Buffer* m_buffer;  // 指向所属的 Buffer 对象

public:
    //Buffer写入方法的实现类
    explicit BufferWriteImpl(Buffer* buffer) : m_buffer(buffer) {}
    //基础类型写入
    bool WriteData(int data);
    bool WriteData(double data);
    bool WriteData(float data);
    bool WriteData(bool data);
    bool WriteData(std::complex<float> data);
    bool WriteData(std::complex<double> data);
    // CircularBuffer基础类型写入
    bool WriteData(const std::vector<int>& data);
    bool WriteData(const std::vector<double>& data);
    bool WriteData(const std::vector<float>& data);
    bool WriteData(const std::vector<bool>& data);
    bool WriteData(const std::vector<std::complex<float>>& data);
    bool WriteData(const std::vector<std::complex<double>>& data);
    bool WriteData(const std::vector<int*>& data);
    bool WriteData(const std::vector<double*>& data);
    bool WriteData(const std::vector<std::complex<double>*>& data);

    // CircularBuffer 相关写入
    bool WriteData(const SystemVueModelBuilder::CircularBufferBase& data);

    // EnvelopeSignal 相关写入
    bool WriteData(const SystemVueModelBuilder::EnvelopeSignal& data);
    bool WriteData(const std::vector<SystemVueModelBuilder::EnvelopeSignal>& data);

    bool WriteData(const std::vector<SystemVueModelBuilder::IntMatrix>& data);
    bool WriteData(const std::vector<SystemVueModelBuilder::DoubleMatrix>& data);
    bool WriteData(const std::vector<SystemVueModelBuilder::FloatMatrix>& data);
    bool WriteData(const std::vector<SystemVueModelBuilder::BoolMatrix>& data);
    bool WriteData(const std::vector<SystemVueModelBuilder::FComplexMatrix>& data);
    bool WriteData(const std::vector<SystemVueModelBuilder::DComplexMatrix>& data);
    bool WriteData(const std::vector<SystemVueModelBuilder::EnvelopeMatrix>& data);
private:
    template<typename T>
    void WriteBusData(const std::vector<T>& data);

    // 具体类型的写入实现
    void WriteIntDataImpl(const std::vector<int>& data);
    void WriteDoubleDataImpl(const std::vector<double>& data);
    void WriteFloatDataImpl(const std::vector<float>& data);
    void WriteBoolDataImpl(const std::vector<bool>& data);
    void WriteFComplexDataImpl(const std::vector<std::complex<float>>& data);
    void WriteDComplexDataImpl(const std::vector<std::complex<double>>& data);
    void WriteEnvelopeSignalDataImpl(const SystemVueModelBuilder::EnvelopeSignal& data);
    void WriteEnvelopeSignalDataImpl(const std::vector<SystemVueModelBuilder::EnvelopeSignal>& data);

    void WriteIntMatrixDataImpl(const std::vector<SystemVueModelBuilder::IntMatrix> &data);
    void WriteDoubleMatrixDataImpl(const std::vector<SystemVueModelBuilder::DoubleMatrix> &data);
    void WriteFloatMatrixDataImpl(const std::vector<SystemVueModelBuilder::FloatMatrix> &data);
    void WriteBoolMatrixDataImpl(const std::vector<SystemVueModelBuilder::BoolMatrix> &data);
    void WriteFComplexMatrixDataImpl(const std::vector<SystemVueModelBuilder::FComplexMatrix> &data);
    void WriteDComplexMatrixDataImpl(const std::vector<SystemVueModelBuilder::DComplexMatrix> &data);
    void WriteEnvelopeMatrixDataImpl(const std::vector<SystemVueModelBuilder::EnvelopeMatrix> &data);


    // 辅助方法
    bool SmartExpandIfNeeded(size_t writeSize, size_t readSize);
    void AutoRestoreIfPossible();
};
}
#endif // BUFFERWRITEIMPL_H
