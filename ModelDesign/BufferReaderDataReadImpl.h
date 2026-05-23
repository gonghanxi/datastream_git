#ifndef BUFFERREADERDATAREADIMPL_H
#define BUFFERREADERDATAREADIMPL_H
#include "BufferReader.h"
#include "BufferReadImpl.h"

namespace SystemVueModelBuilder {
class BufferReaderDataReadImpl
{
private:
    //BufferReader指针，用于访问BufferReader的私有成员变量
    BufferReader* m_reader;
public:
    //BufferReader读取方法的实现类
    explicit BufferReaderDataReadImpl(BufferReader* reader) : m_reader(reader) {}

    //从输出端读取数据方法
    bool ReadData(int& outputData);
    bool ReadData(double& outputData);
    bool ReadData(float& outputData);
    bool ReadData(bool& outputData);
    bool ReadData(std::complex<float>& outputData);
    bool ReadData(std::complex<double>& outputData);

    bool ReadData(std::vector<int>& outputData);
    bool ReadData(std::vector<double>& outputData);
    bool ReadData(std::vector<float>& outputData);
    bool ReadData(std::vector<bool>& outputData);
    bool ReadData(std::vector<std::complex<float>>& outputData);
    bool ReadData(std::vector<std::complex<double>>& outputData);
    bool ReadData(std::vector<int*>& outputData);
    bool ReadData(std::vector<double*>& outputData);
    bool ReadData(std::vector<std::complex<double>*>& outputData);
    bool ReadData(SystemVueModelBuilder::CircularBufferBase& outputData);

    bool ReadData(std::vector<SystemVueModelBuilder::EnvelopeSignal>& outputData);
    bool ReadData(SystemVueModelBuilder::EnvelopeCircularBuffer& outputData);

    //--------------------------------------------------------------
    //从输出端读取数据的矩阵类型方法
    bool ReadData(std::vector<SystemVueModelBuilder::IntMatrix> &outputData);
    bool ReadData(std::vector<SystemVueModelBuilder::DoubleMatrix> &outputData);
    bool ReadData(std::vector<SystemVueModelBuilder::FloatMatrix> &outputData);
    bool ReadData(std::vector<SystemVueModelBuilder::BoolMatrix> &outputData);
    bool ReadData(std::vector<SystemVueModelBuilder::FComplexMatrix> &outputData);
    bool ReadData(std::vector<SystemVueModelBuilder::DComplexMatrix> &outputData);
    bool ReadData(std::vector<SystemVueModelBuilder::EnvelopeMatrix> &outputData);
    //--------------------------------------------------------------
    //从输出端读取数据的bus类型方法
    bool ReadBusData(size_t readSize, std::vector<int>& outputData);
    bool ReadBusData(size_t readSize, std::vector<double>& outputData);
    bool ReadBusData(size_t readSize, std::vector<float>& outputData);
    bool ReadBusData(size_t readSize, std::vector<bool>& outputData);
    bool ReadBusData(size_t readSize, std::vector<std::complex<double>>& outputData);
    bool ReadBusData(size_t readSize, std::vector<std::complex<float>>& outputData);
    bool ReadBusData(size_t readSize, std::vector<SystemVueModelBuilder::EnvelopeSignal>& outputData);
    bool ReadBusData(size_t readSize, SystemVueModelBuilder::EnvelopeCircularBuffer& outputData);

    bool ReadBusData(size_t readSize, std::vector<SystemVueModelBuilder::IntMatrix>& outputData);
    bool ReadBusData(size_t readSize, std::vector<SystemVueModelBuilder::DoubleMatrix>& outputData);
    bool ReadBusData(size_t readSize, std::vector<SystemVueModelBuilder::FloatMatrix>& outputData);
    bool ReadBusData(size_t readSize, std::vector<SystemVueModelBuilder::BoolMatrix>& outputData);
    bool ReadBusData(size_t readSize, std::vector<SystemVueModelBuilder::FComplexMatrix>& outputData);
    bool ReadBusData(size_t readSize, std::vector<SystemVueModelBuilder::DComplexMatrix>& outputData);
    bool ReadBusData(size_t readSize, std::vector<SystemVueModelBuilder::EnvelopeMatrix>& outputData);
};
}
#endif // BUFFERREADERDATAREADIMPL_H
