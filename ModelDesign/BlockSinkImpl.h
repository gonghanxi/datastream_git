#ifndef BLOCKSINKIMPL_H
#define BLOCKSINKIMPL_H

#include "Block.h"

namespace SystemVueModelBuilder {

class BlockSinkImpl
{
private:
    //Block指针，用于访问Block的私有成员变量
    Block* m_block;
public:
    using DataType = Buffer::DataType;
    using TerminalMode = Block::TerminalMode;
    //Block终端处理的实现类
    explicit BlockSinkImpl(Block* block) :m_block(block) {}


    //终端块处理
    bool ProcessAsTerminalBlock(const std::string& inputPortName);
    bool IsTerminalBlock() const;
    // 终端处理方法 不同数据类型
    bool ProcessAndWriteDoubles(BufferReader* reader, size_t availableData);
    bool ProcessAndWriteComplexDoubles(BufferReader* reader, size_t availableData);
    bool ProcessAndWriteEnvelopeSignals(BufferReader* reader, size_t availableData);
    bool ProcessAndWriteInts(BufferReader* reader, size_t availableData);
    bool ProcessAndWriteFloats(BufferReader* reader, size_t availableData);
    bool ProcessAndWriteBools(BufferReader* reader, size_t availableData);
    bool ProcessAndWriteComplexFloats(BufferReader* reader, size_t availableData);

    // 终端写入json方法
    void WriteDoubleToJson(size_t index, double value, double timestamp);
    void WriteDComplexToJson(size_t index, std::complex<double> value, double timestamp);
    void WriteFComplexToJson(size_t index, std::complex<float> value, double timestamp);
    void WriteEnvelopeSignalToJson(size_t index, const SystemVueModelBuilder::EnvelopeSignal& envelope, double timestamp);
    void WriteIntToJson(size_t index, int value, double timestamp);
    void WriteFloatToJson(size_t index, float value, double timestamp);
    void WriteBoolToJson(size_t index, bool value, double timestamp);
};
}
#endif // BLOCKSINKIMPL_H
