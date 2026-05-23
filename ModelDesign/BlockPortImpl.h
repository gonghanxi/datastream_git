#ifndef BLOCKPORTIMPL_H
#define BLOCKPORTIMPL_H

#include "Block.h"
#include "DataTypesAndParsers.h"

namespace SystemVueModelBuilder {

class BlockPortImpl
{
private:
    //Block指针，用于访问Block的私有成员变量
    Block* m_block;
public:
    using DataType = Buffer::DataType;

    //Block添加端口的实现类
    explicit BlockPortImpl(Block* block) :m_block(block) {}

    //将模型输入端口添加到数据流输入端口
    //Bus类型
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::CircularBufferBus& externalPort,
                                      size_t readSize, DataType dataType);
    //时域类型
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<double>& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<int>& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<float>& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<bool>& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<std::complex<float>>& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>>& externalPort,
                                      size_t readSize, DataType dataType);
    //普通类型
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::IntCircularBuffer& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::FloatCircularBuffer& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::FComplexCircularBuffer& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::DComplexCircularBuffer& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::CircularBuffer<double>& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::BoolCircularBuffer& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::EnvelopeCircularBuffer& externalPort,
                                      size_t readSize, DataType dataType);
    //基础类型
    BufferReader* AddInputPort(const std::string& portName, int& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, double& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, float& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, bool& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, std::complex<float>& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, std::complex<double>& externalPort,
                                      size_t readSize, DataType dataType);
    //矩阵类型
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::IntMatrixCircularBuffer& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::DoubleMatrixCircularBuffer& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::FloatMatrixCircularBuffer& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::BoolMatrixCircularBuffer& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::FComplexMatrixCircularBuffer& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::DComplexMatrixCircularBuffer& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::EnvelopeMatrixCircularBuffer& externalPort,
                                      size_t readSize, DataType dataType);
    //时域矩阵类型
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<IntMatrix>& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<DoubleMatrix>& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<FloatMatrix>& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<BoolMatrix>& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<FComplexMatrix>& externalPort,
                                      size_t readSize, DataType dataType);
    BufferReader* AddInputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<DComplexMatrix>& externalPort,
                                      size_t readSize, DataType dataType);


    //将模型输出端口添加到数据流输出端口
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::CircularBufferBus& externalPort,
                                      size_t writeSize, DataType dataType);
    //普通类型
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::IntCircularBuffer& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::FloatCircularBuffer& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::BoolCircularBuffer& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::FComplexCircularBuffer& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::DComplexCircularBuffer& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::DoubleCircularBuffer& externalPort,
                          size_t writeSize, DataType dataType);

    //时域类型
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<int>& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<float>& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<double>& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<bool>& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<std::complex<float>>& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>>& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::EnvelopeCircularBuffer& externalPort,
                          size_t writeSize, DataType dataType);

    //基础类型
    Buffer* AddOutputPort(const std::string& portName, int& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, float& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, bool& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, double& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, std::complex<float>& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, std::complex<double>& externalPort,
                          size_t writeSize, DataType dataType);
    //矩阵类型
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::IntMatrixCircularBuffer& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::FloatMatrixCircularBuffer& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::BoolMatrixCircularBuffer& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::DoubleMatrixCircularBuffer& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::FComplexMatrixCircularBuffer& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::DComplexMatrixCircularBuffer& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::EnvelopeMatrixCircularBuffer& externalPort,
                          size_t writeSize, DataType dataType);

    //时域矩阵类型
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<IntMatrix>& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<DoubleMatrix>& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<FloatMatrix>& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<BoolMatrix>& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<FComplexMatrix>& externalPort,
                          size_t writeSize, DataType dataType);
    Buffer* AddOutputPort(const std::string& portName, SystemVueModelBuilder::TimedCircularBuffer<DComplexMatrix>& externalPort,
                          size_t writeSize, DataType dataType);
};
}
#endif // BLOCKPORTIMPL_H
