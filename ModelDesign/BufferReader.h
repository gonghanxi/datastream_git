#ifndef BUFFERREADER_H
#define BUFFERREADER_H


#include "Buffer.h"
#include "DFInterface.h"
#include "CircularBuffer.h"
#include "EnvelopeSignal.h"
#include "BusConnection.h"
#include "DataTypesAndParsers.h"

#include <string>
#include <memory>
#include <iostream>
#include <variant>

namespace SystemVueModelBuilder {

class Block;
class BufferReaderDataReadImpl;
class BufferReader
{
public:
    //用于判断是否是bus类型的桥接读指针
    enum ReaderType {
        STANDARD,     // 标准单连接读取器
        BUS_MASTER,   // 总线主读取器（管理多个连接）
        BUS_BRIDGE    // 总线桥接读取器（单连接）
    };

    using DataType = DataTypes::Type;



    BufferReader(const std::string& name, size_t readSize = 1, DataType type = DataType::INT);
    ~BufferReader();
    //输入端端口的数据类型
    DataType GetDataType() const;
    void SetDataType(DataType type);
    //--------------------------------------------------------------
    //connect方法
    void connectToBuffer(Buffer* buffer);
    void Disconnect();

    //--------------------------------------------------------------
    bool ReadData(int& outputData);
    bool ReadData(double& outputData);
    bool ReadData(float& outputData);
    bool ReadData(bool& outputData);
    bool ReadData(std::complex<float>& outputData);
    bool ReadData(std::complex<double>& outputData);
    //--------------------------------------------------------------
    //从输出端读取数据方法
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
    //--------------------------------------------------------------
    //设置输入端的名称/读指针速率大小
    void SetName(const std::string& name);
    void SetReadSize(size_t readSize);
    //--------------------------------------------------------------
    //表征频率，用于包络信号
    void setCharacterizationFrequency(double fc);
    double getCharacterizationFrequency() const;
    bool hasCharacterizationFrequency() const;
    // 从连接的Buffer传播表征频率
    bool propagateCharacterizationFrequencyFromBuffer();

    //--------------------------------------------------------------
    //get方法
    const std::string& GetName() const;
    size_t GetReadSize() const;
    size_t GetRate() const;
    Buffer* GetConnectedBuffer() const;
    //获取可读取的数量
    size_t GetAvailableDataCount() const;
    //判断是否有数据可读
    bool HasDataAvailable();
    bool IsConnected() const;
    // 检查是否有有效连接（总线类型和非总线类型都适用）
    bool HasValidConnection() const;
    bool IsConnectedToBuffer(Buffer* buffer) const;
    //上下游判断
    bool IsUpstreamDone() const;
    bool IsDownstreamDone() const;
    void SetDownstreamDone(bool done);

    SystemVueModelBuilder::DFInterface* GetDFInterface();

    //--------------------------------------------------------------
    // 添加总线数据可用性检查
    size_t GetBusAvailableDataCount() const;
    static bool IsBusType(DataType type);
    bool IsBusReader() const;
    bool IsBusUpstreamDone() const;
    bool IsBusDownstreamDone() const;
    // 总线连接管理函数
    const std::vector<BusConnection>& GetBusConnections() const; //获取bus链接容器
    size_t GetBusConnectionCount() const; //获取bus链接的数量
    void AddBusConnection(const BusConnection& connection);
    void ClearBusConnections();
    //判断读指针类型
    ReaderType GetReaderType() const;
    void SetReaderType(ReaderType type);
    std::string ReaderTypeToString(ReaderType type);
    //--------------------------------------------------------------
    //外部端口buffer更新时读指针的更新方法
    void OnBufferReallocated();
    void ReconnectToBuffer(Buffer* buffer);
    void EnsureProperRegistration(Buffer* buffer);
    //--------------------------------------------------------------
    //背压回调
    void NotifySpaceAvailable();// 空间释放通知（当读取数据后调用）
    //--------------------------------------------------------------
    // 设置临时缓冲区
    void SetTempBuffer(size_t size, DataType dataType);

    // 从连接缓冲区读取数据到临时缓冲区
    bool ReadToTempBuffer();

    // 获取临时缓冲区中的数据量
    size_t GetTempBufferDataCount() const;

    // 清空临时缓冲区
    void ClearTempBuffer();

    // 从临时缓冲区读取数据
    template<typename T>
    bool ReadFromTempBuffer(std::vector<T>& data, size_t count)
    {
        if (m_tempDataCount == 0) {
            qDebug() << "Temp buffer is empty";
            return false;
        }

        size_t readCount = std::min(count, m_tempDataCount);
        if (readCount == 0) {
            return false;
        }

        // 检查类型匹配
        if (!IsCompatibleType<T>(m_tempDataType)) {
            qDebug() << "ERROR: Type mismatch in temp buffer read";
            return false;
        }

        data.resize(readCount);
        size_t elementSize = sizeof(T);
        size_t offset = 0;  // 总是从临时缓冲区开头读取

        memcpy(data.data(), m_tempBuffer.data() + offset, readCount * elementSize);

        qDebug() << "Read " << readCount << " samples from temp buffer";

        // 注意：不从临时缓冲区移除数据，保持数据用于其他读取器
        return true;
    }

    //时间驱动
    // 引擎调用：设置为变步长模式
    void SetVariableMode(bool enabled) { m_isVariableMode = enabled; }
    // 查询是否为变步长模式
    bool IsVariableMode() const { return m_isVariableMode; }
private:
    std::string m_name;
    size_t m_readSize; //读指针大小
    DataType m_dataType;
    ReaderType m_readerType = STANDARD;

    // 默认不是变步长模式
    bool m_isVariableMode = false;

    //实现类BufferReaderDataReadImpl
    std::unique_ptr<BufferReaderDataReadImpl> m_datareader;
    friend class SystemVueModelBuilder::BufferReaderDataReadImpl;

    //包络信号fc
    double m_fc = 0.0;
    bool m_hasCharacterizationFrequency = false;

    bool m_downstreamDone = false;

    SystemVueModelBuilder::DFInterface m_dfinterface;
    std::variant<
    int,
    double,
    float,
    bool,
    std::complex<float>,
    std::complex<double>,

    int*,
    double*,
    std::complex<double>*,

    SystemVueModelBuilder::CircularBuffer<int>,
    SystemVueModelBuilder::CircularBuffer<double>,
    SystemVueModelBuilder::CircularBuffer<float>,
    SystemVueModelBuilder::CircularBuffer<bool>,
    SystemVueModelBuilder::CircularBuffer<std::complex<float>>,
    SystemVueModelBuilder::CircularBuffer<std::complex<double>>,

    SystemVueModelBuilder::TimedCircularBuffer<int>,
    SystemVueModelBuilder::TimedCircularBuffer<double>,
    SystemVueModelBuilder::TimedCircularBuffer<float>,
    SystemVueModelBuilder::TimedCircularBuffer<bool>,
    SystemVueModelBuilder::TimedCircularBuffer<std::complex<float>>,
    SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>>,

    std::unique_ptr<SystemVueModelBuilder::IntCircularBufferBus>,
    std::unique_ptr<SystemVueModelBuilder::DoubleCircularBufferBus>,
    std::unique_ptr<SystemVueModelBuilder::FloatCircularBufferBus>,
    std::unique_ptr<SystemVueModelBuilder::BoolCircularBufferBus>,
    std::unique_ptr<SystemVueModelBuilder::DComplexCircularBufferBus>,
    std::unique_ptr<SystemVueModelBuilder::FComplexCircularBufferBus>,

    SystemVueModelBuilder::EnvelopeCircularBuffer,

    SystemVueModelBuilder::IntMatrixCircularBuffer,
    SystemVueModelBuilder::DoubleMatrixCircularBuffer,
    SystemVueModelBuilder::FloatMatrixCircularBuffer,
    SystemVueModelBuilder::BoolMatrixCircularBuffer,
    SystemVueModelBuilder::FComplexMatrixCircularBuffer,
    SystemVueModelBuilder::DComplexMatrixCircularBuffer,
    SystemVueModelBuilder::EnvelopeMatrixCircularBuffer
        >m_dataVariant;//数据类型容器

    Buffer* m_connectedBuffer = nullptr; //读指针对应连接的缓冲区
    //--------------------------------------------------------------
    std::vector<uint8_t> m_tempBuffer;  // 使用 uint8_t 存储原始字节
    size_t m_tempBufferSize = 0;         // 临时缓冲区大小
    size_t m_tempDataCount = 0;          // 临时缓冲区中的数据量
    DataType m_tempDataType;             // 临时缓冲区数据类型
    size_t m_tempReadSize = 0;          // 临时读取大小
    void InitializeTempBuffer(size_t bufferSize, DataType dataType); //初始化临时缓冲区
    bool ReadDataToTempBuffer(size_t readSize); //读取数据放到临时缓冲区

    //读取数据实施方法
    template<typename T>
    bool ReadTypedDataToTempBuffer(size_t readSize, size_t offset)
    {
        std::vector<T> tempData;
        if (!ReadData(tempData)) {
            return false;
        }

        // 复制到临时缓冲区
        memcpy(m_tempBuffer.data() + offset, tempData.data(), readSize * sizeof(T));
        m_tempDataCount += readSize;

        return true;
    }

    size_t GetDataTypeSize(DataType type) const;

    //数据类型判断
    template<typename T>
    bool IsCompatibleType(DataType dataType) const
    {
        return DataTypesAndParsers::IsCompatibleType<T>(dataType);
    }
    //--------------------------------------------------------------
    std::vector<BusConnection> m_busConnections;  // 总线连接列表
    bool m_isBusType = false;                     // 标记是否为总线类型
    //--------------------------------------------------------------
    void createDataVariant(DataType type);
    //读取数据实施方法
    template<typename T>
    bool ReadTypedData(T& outputData)
    {
        if (!m_connectedBuffer) {
            qDebug() << "ERROR: Buffer not connected for reader: " << QString::fromStdString(m_name);
            return false;
        }

        // 对于桥接读取器，不要检查总线连接
        if (m_readerType == BUS_BRIDGE) {
//            qDebug() << "Bridge reader - direct buffer access";
        }

        // 检查上游是否完成且无数据
        if (IsUpstreamDone() && GetAvailableDataCount() == 0) {
//            qDebug() << "BufferReader '" << QString::fromStdString(m_name) << "': Upstream done and no data available";
            SetDownstreamDone(true);  // 标记下游完成
            return false;
        }

        outputData.clear();

        //调用输出端的方法读取
        bool success = m_connectedBuffer->ReadDataForReader(m_readSize, outputData, m_name);

        if (success) {
//            qDebug() << "BufferReader '" << QString::fromStdString(m_name) << "': Successfully read "
//                      << outputData.size() << " data";
        } else {
            qDebug() << "BufferReader '" << QString::fromStdString(m_name) << "': Read failed";
        }

        return success;
    }
    //读取bus数据实施方法
    template<typename BusType>
    bool ReadTypedBusData(BusType& outputBus) {
        if (!m_connectedBuffer) {
            qDebug() << "BufferReader '" << QString::fromStdString(m_name) << "': Read failed - not connected to buffer";
            return false;
        }

        // 获取需要读取的大小
        size_t available = m_connectedBuffer->GetAvailableDataForReader(m_name);

        qDebug() << "Available bus data: " << available << " elements";

        // 注意：这里将 available 作为 readSize，因为总线数据的大小是动态的
        bool success = m_connectedBuffer->ReadDataForReader(available, outputBus, m_name);

        if (success) {
//            qDebug() << "BufferReader '" << QString::fromStdString(m_name) << "': Successfully read CircularBufferBus data";

            // 输出总线信息
            size_t portCount = outputBus.GetSize();
            qDebug() << "Bus has " << portCount << " ports";

            for (size_t i = 0; i < std::min(portCount, static_cast<size_t>(3)); i++) {
                // 使用 operator[] 访问端口
                auto& portBuffer = outputBus[i];
                qDebug() << "  Port " << i << ": size=" << portBuffer.GetSize()
                          << ", elementSize=" << portBuffer.GetSizeOf();
            }
        } else {
            qDebug() << "BufferReader '" << QString::fromStdString(m_name) << "': Read CircularBufferBus failed";
        }

        return success;
    }
};

}

#endif // BUFFERREADER_H
