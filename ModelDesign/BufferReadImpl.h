#ifndef BUFFERREADIMPL_H
#define BUFFERREADIMPL_H

#include "Buffer.h"
#include "CircularBuffer.h"
#include "EnvelopeSignal.h"
#include "DataTypesAndParsers.h"
#include <vector>
#include <memory>
#include <iostream>

namespace SystemVueModelBuilder {
class BufferReadImpl
{
private:
    Buffer* m_buffer;  // 指向所属的 Buffer 对象

public:
    //Buffer的读取实现类
    explicit BufferReadImpl(Buffer* buffer) : m_buffer(buffer) {}

    //输出端buffer的读取方法，由读指针读取给bufferReader输入端
    // 基础类型读取
    bool ReadDataForReader(int& outputData, const std::string& readerName);
    bool ReadDataForReader(double& outputData, const std::string& readerName);
    bool ReadDataForReader(float& outputData, const std::string& readerName);
    bool ReadDataForReader(bool& outputData, const std::string& readerName);
    bool ReadDataForReader(std::complex<float>& outputData, const std::string& readerName);
    bool ReadDataForReader(std::complex<double>& outputData, const std::string& readerName);
    // CircularBuffer基础类型读取
    bool ReadDataForReader(size_t readSize, std::vector<int>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<double>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<float>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<bool>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<std::complex<float>>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<std::complex<double>>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<int*>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<double*>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<std::complex<double>*>& outputData, const std::string& readerName);

    // CircularBuffer 相关读取
    bool ReadDataForReader(size_t readSize, SystemVueModelBuilder::CircularBufferBase& outputData, const std::string& readerName);

    // EnvelopeSignal 相关读取
    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::EnvelopeSignal>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, SystemVueModelBuilder::EnvelopeCircularBuffer& outputData, const std::string& readerName);

    // 矩阵类型读取
    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::IntMatrix>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::DoubleMatrix>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::FloatMatrix>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::BoolMatrix>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::FComplexMatrix>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::DComplexMatrix>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::EnvelopeMatrix>& outputData, const std::string& readerName);
private:
    template<typename BufferType, typename OutputType>
    bool ReadDataForReaderImpl(size_t readSize, std::vector<OutputType>& outputData,
                               const std::string& readerName,CircularBuffer<BufferType>* circularbuffer)
    {
        if (m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
            qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not found!";
            return false;
        }

        size_t& readerPosition  = m_buffer->m_readerPositions[readerName];
        // 检查reader位置有效性
        if (readerPosition == SIZE_MAX) {
            qDebug() << "ERROR: Invalid reader position for '" << QString::fromStdString(readerName) << "'";
            readerPosition = 0;
            return false;
        }
        //获取可用数据量
        size_t available = m_buffer->m_totalWritten - readerPosition;

        //时间驱动与数据流驱动 区别
        //1.时间驱动每次读取1个数据
        //2.数据流驱动每次读取读指针数据量

        //1.时间驱动
        if(m_buffer->IsVariableMode()) {
            //读取读指针数据量
            outputData.resize(available);
            for (size_t i = 0; i < available; i++) {
                size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
                outputData[i] = (*circularbuffer)[readIndex];
            }

            //更新这个读指针位置和当前数据量
            readerPosition += available;
            m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
            return true;
        }
        //2.数据流驱动
        else {
            //读取读指针数据量
            outputData.resize(readSize);
            for (size_t i = 0; i < readSize; i++) {
                size_t readIndex = (readerPosition + i) % m_buffer->m_bufferSize;
                outputData[i] = (*circularbuffer)[readIndex];
            }

            //更新这个读指针位置和当前数据量
            readerPosition += readSize;
            m_buffer->m_dataCount = m_buffer->m_totalWritten - readerPosition;
            return true;
        }
    }
    // 具体类型的读取实现
    //兼容:
    //bool -> int
    bool ReadIntDataForReaderImpl(size_t readSize, std::vector<int>& outputData, const std::string& readerName);

    //兼容:
    //int -> double
    bool ReadDoubleDataForReaderImpl(size_t readSize, std::vector<double>& outputData, const std::string& readerName);

    //兼容:
    //int -> float
    bool ReadFloatDataForReaderImpl(size_t readSize, std::vector<float>& outputData, const std::string& readerName);

    //兼容:
    //int -> bool
    bool ReadBoolDataForReaderImpl(size_t readSize, std::vector<bool>& outputData, const std::string& readerName);

    //兼容:
    //int -> fcomplex
    //double -> fcomplex
    //float -> fcomplex
    bool ReadFComplexDataForReaderImpl(size_t readSize, std::vector<std::complex<float>>& outputData, const std::string& readerName);

    //兼容:
    //int -> dcomplex
    //double -> dcomplex
    //float -> fcomplex
    bool ReadDComplexDataForReaderImpl(size_t readSize, std::vector<std::complex<double>>& outputData, const std::string& readerName);

    //兼容:
    //int -> envelope
    //double -> envelope
    //float -> envelope
    bool ReadEnvelopeSignalDataForReaderImpl(size_t readSize, std::vector<SystemVueModelBuilder::EnvelopeSignal>& outputData,
                                             const std::string& readerName);
    bool ReadEnvelopeCircularBufferDataForReaderImpl(size_t readSize, SystemVueModelBuilder::EnvelopeCircularBuffer& outputData,
                                             const std::string& readerName);

    //兼容:
    //bool_M -> int_M
    bool ReadIntMatrixDataForReaderImpl(size_t readSize, std::vector<SystemVueModelBuilder::IntMatrix>& outputData, const std::string& readerName);

    //兼容:
    //int_M -> double_M
    bool ReadDoubleMatrixDataForReaderImpl(size_t readSize, std::vector<SystemVueModelBuilder::DoubleMatrix>& outputData, const std::string& readerName);

    //兼容:
    //int_M -> float_M
    bool ReadFloatMatrixDataForReaderImpl(size_t readSize, std::vector<SystemVueModelBuilder::FloatMatrix>& outputData, const std::string& readerName);

    //兼容:
    //int_M -> bool_M
    bool ReadBoolMatrixDataForReaderImpl(size_t readSize, std::vector<SystemVueModelBuilder::BoolMatrix>& outputData, const std::string& readerName);
    bool ReadFComplexMatrixDataForReaderImpl(size_t readSize, std::vector<SystemVueModelBuilder::FComplexMatrix>& outputData, const std::string& readerName);
    bool ReadDComplexMatrixDataForReaderImpl(size_t readSize, std::vector<SystemVueModelBuilder::DComplexMatrix>& outputData, const std::string& readerName);
    bool ReadEnvelopeMatrixDataForReaderImpl(size_t readSize, std::vector<SystemVueModelBuilder::EnvelopeMatrix>& outputData, const std::string& readerName);

    // 总线读取模板函数
    template<typename T>
    bool ReadTypedBusDataForReader(size_t readSize, SystemVueModelBuilder::CircularBufferBus& outputData, const std::string& readerName);

    // 辅助方法
    bool SmartExpandIfNeeded(size_t writeSize, size_t readSize);
    void AutoRestoreIfPossible();
    bool CheckAllBusReaderHaveData(const std::string& readerName, size_t readSize);
};
}
#endif // BUFFERREADIMPL_H
