#ifndef BUFFER_H
#define BUFFER_H

#include "CircularBuffer.h"
#include "EnvelopeSignal.h"
#include "DataTypesAndParsers.h"
#include "BusConnection.h"
#include "../Common/LogExport.h"

#include <vector>
#include <variant>
#include <memory>
#include <string>
#include <iostream>
#include <functional>
#include <QDebug>
#include <any>

namespace SystemVueModelBuilder {

class DFInterface;
class BufferReader;
class BufferBusDataImpl;
class BufferExpansionImpl;
class BufferMemoryImpl;
class BufferWriteImpl;
class BufferReadImpl;

class Buffer
{
public:
    //用于判断是否是bus类型的桥接写指针
    enum WriterType {
        STANDARD,     // 标准单连接读取器
        BUS_MASTER,   // 总线主读取器（管理多个连接）
        BUS_BRIDGE    // 总线桥接读取器（单连接）
    };

    using DataType = DataTypes::Type;

    Buffer(const std::string& name, size_t writeSize = 1, DataType type = DataType::INT);
    ~Buffer();
    //设置输出端口的数据类型
    DataType GetDataType() const;
    void SetDataType(DataType type);


    //--------------------------------------------------------------
    bool WriteData(int data);
    bool WriteData(double data);
    bool WriteData(float data);
    bool WriteData(bool data);
    bool WriteData(std::complex<float> data);
    bool WriteData(std::complex<double> data);
    //写入输出端buffer的方法
    bool WriteData(const std::vector<int>& data);
    bool WriteData(const std::vector<double>& data);
    bool WriteData(const std::vector<float>& data);
    bool WriteData(const std::vector<bool>& data);
    bool WriteData(const std::vector<std::complex<float>>& data);
    bool WriteData(const std::vector<std::complex<double>>& data);
    bool WriteData(const std::vector<int*>& data);
    bool WriteData(const std::vector<double*>& data);
    bool WriteData(const std::vector<std::complex<double>*>& data);
    bool WriteData(const SystemVueModelBuilder::CircularBufferBase& data);

    //bus类型写入
    template<typename T>
    bool WriteDataToChannel(int channelIndex, const std::vector<T>& data)
    {
        // 检查 writer 类型
        if (m_writerType != BUS_MASTER) {
            LOG_ERROR("Only master bus writers should call WriteDataToChannel");
            return false;
        }

        // 检查通道索引有效性
        if (channelIndex < 0 || channelIndex >= static_cast<int>(m_busConnections.size())) {
            qDebug() << "ERROR: Invalid channel index" << channelIndex
                     << ", available channels:" << m_busConnections.size();
            return false;
        }

        const auto& connection = m_busConnections[channelIndex];

        // 检查桥接写入器是否存在
        if (!connection.bridgeWriter) {
            LOG_ERROR("ERROR: Bridge writer is null for channel", channelIndex);
            return false;
        }

        // 检查该通道是否允许写入
        if (!connection.PermitWrite) {
            return false;
        }

        // 调用桥接写入器的 WriteData 方法
        connection.bridgeWriter->WriteData(data);
    }
    bool WriteEnvelopeDataToChannel(int channelIndex, const std::vector<EnvelopeSignal>& data, double fc);
    bool WriteData(const SystemVueModelBuilder::CircularBufferBus& data);

    bool WriteData(const SystemVueModelBuilder::EnvelopeSignal& data);
    bool WriteData(const std::vector<SystemVueModelBuilder::EnvelopeSignal>& data);

    bool WriteData(const std::vector<SystemVueModelBuilder::IntMatrix>& data);
    bool WriteData(const std::vector<SystemVueModelBuilder::DoubleMatrix>& data);
    bool WriteData(const std::vector<SystemVueModelBuilder::FloatMatrix>& data);
    bool WriteData(const std::vector<SystemVueModelBuilder::BoolMatrix>& data);
    bool WriteData(const std::vector<SystemVueModelBuilder::FComplexMatrix>& data);
    bool WriteData(const std::vector<SystemVueModelBuilder::DComplexMatrix>& data);
    bool WriteData(const std::vector<SystemVueModelBuilder::EnvelopeMatrix>& data);
    //输出端buffer的读取方法，由读指针读取给bufferReader输入端
    bool ReadDataForReader(int& outputData, const std::string& readerName);
    bool ReadDataForReader(double& outputData, const std::string& readerName);
    bool ReadDataForReader(float& outputData, const std::string& readerName);
    bool ReadDataForReader(bool& outputData, const std::string& readerName);
    bool ReadDataForReader(std::complex<float>& outputData, const std::string& readerName);
    bool ReadDataForReader(std::complex<double>& outputData, const std::string& readerName);

    bool ReadDataForReader(size_t readSize, std::vector<int>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<double>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<float>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<bool>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<std::complex<float>>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<std::complex<double>>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<int*>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<double*>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<std::complex<double>*>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, SystemVueModelBuilder::CircularBufferBase& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, SystemVueModelBuilder::CircularBufferBus& outputData, const std::string& readerName);

    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::EnvelopeSignal>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, SystemVueModelBuilder::EnvelopeCircularBuffer& outputData, const std::string& readerName);

    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::IntMatrix>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::DoubleMatrix>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::FloatMatrix>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::BoolMatrix>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::FComplexMatrix>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::DComplexMatrix>& outputData, const std::string& readerName);
    bool ReadDataForReader(size_t readSize, std::vector<SystemVueModelBuilder::EnvelopeMatrix>& outputData, const std::string& readerName);
    //--------------------------------------------------------------
    //添加/注册读指针方法
    size_t AddReader(size_t readSize);
    size_t AddReader(BufferReader* reader);
    size_t AddReader(size_t readSize, const std::string& readerName);
    void RegisterReader(const std::string& readerName, BufferReader* reader);
    void UnRegisterReader(const std::string& readerName);
    bool FindRegisterReader(const std::string& readerName);
    //--------------------------------------------------------------
    //重置方法
    void ResetBuffer();
    void ResetReaderPoint();
    //--------------------------------------------------------------
    //get方法
    size_t GetAvailableDataForReader(const std::string& readerName);//获取每个读指针可读的数据量
    std::string GetName() const;  //获取输出端口名称
    size_t GetBufferSize() const; //获取缓冲区的大小
    size_t GetTotalWritten() const; //获取总写入量
    size_t GetUsedSpace() const;  //获取当前已写入的数量
    size_t GetBufferFreeSpace() const; //获取空余空间量
    size_t GetReaderCount() const; //获取读指针的数量

    size_t GetReaderPosition(const std::string& readerName) const; //获取读指针的位置
    bool SetReaderPosition(const std::string &readerName, size_t newPosition);//设置读指针的位置（只用于写入失败回滚）


    size_t GetWriteSize() const; //获取写指针速率大小
    void SetWriteSize(size_t size);
    //--------------------------------------------------------------
    //获取外部端口的Buffer指针，用于访问外部端口的buffer

    //兼容
    //bool  time_bool
    SystemVueModelBuilder::IntCircularBuffer* getIntCircularBuffer();

    //兼容
    //int time_int
    SystemVueModelBuilder::DoubleCircularBuffer* getDoubleCircularBuffer();

    //兼容
    //int time_int
    SystemVueModelBuilder::FloatCircularBuffer* getFloatCircularBuffer();

    //兼容
    //int time_int
    SystemVueModelBuilder::BoolCircularBuffer* getBoolCircularBuffer();

    //兼容
    //int time_int
    //double  time_double
    //float time_float
    SystemVueModelBuilder::FComplexCircularBuffer* getFComplexCircularBuffer();

    //兼容
    //int time_int
    //double  time_double
    //float time_float
    SystemVueModelBuilder::DComplexCircularBuffer* getDComplexCircularBuffer();

    //兼容
    //int time_int
    //double time_double
    //float time_float
    SystemVueModelBuilder::EnvelopeCircularBuffer* getEnvelopeCircularBuffer();

    //兼容
    //bool_M  time_bool_M
    SystemVueModelBuilder::IntMatrixCircularBuffer* getIntMatrixCircularBuffer();

    //兼容
    //int_M time_int_M
    SystemVueModelBuilder::DoubleMatrixCircularBuffer* getDoubleMatrixCircularBuffer();

    //兼容
    //int_M time_int_M
    SystemVueModelBuilder::FloatMatrixCircularBuffer* getFloatMatrixCircularBuffer();

    //兼容
    //int_M time_int_M
    SystemVueModelBuilder::BoolMatrixCircularBuffer* getBoolMatrixCircularBuffer();
    SystemVueModelBuilder::FComplexMatrixCircularBuffer* getFComplexMatrixCircularBuffer();
    SystemVueModelBuilder::DComplexMatrixCircularBuffer* getDComplexMatrixCircularBuffer();
    SystemVueModelBuilder::EnvelopeMatrixCircularBuffer* getEnvelopeMatrixCircularBuffer();
    //--------------------------------------------------------------
    void UpdateBufferSize(); //更新端口的buffer大小，同样适用于外部端口的buffer
    void ReallocateBufferMemory(); //分配内部端口的buffer大小，即不是模型设置的端口
    void ReallocateExternalBuffer(); //分配外部端口的buffer大小
    //--------------------------------------------------------------
    void SetUpstreamDone(bool done); //设置上游是否处理完成，即输出端buffer的写指针是否写入过数据，也就是输入端读指针是否从上游输出端读取过数据
    bool IsUpstreamDone() const;  //判断上游是否处理完成
    bool IsDownstreamDone() const; //判断下游是否处理完成
    void SetDownstreamDone(bool done); //设置下游是否处理完成，即输出端buffer的下游输入端读指针是否读取过数据
    //--------------------------------------------------------------
    // 普通buffer端口内存管理
    // 使用外部缓冲区
    bool SetExternalCircularBuffer(SystemVueModelBuilder::CircularBufferBase* externalBuffer); //设置访问外部端口缓冲区的指针
    void EnsureCircularBuffer(); //确保内部缓冲区的指针能访问到外部缓冲区
    void EnsureTimedCircularBuffer();//确保内部缓冲区的指针能访问到外部缓冲区（时域类型）
    void CreateBufferVariantWithoutAllocation(); //将内部缓冲区的指针初始化
    void WireInternalBufferToExternalMemory(); //将内部缓冲区的指针连接到外部缓冲区的内存上


    // 输出bus端口内存管理
    bool SetExternalCircularBufferBus(SystemVueModelBuilder::CircularBufferBus* externalBus);
    CircularBufferBus* GetExternalCircularBufferBus() const;
    WriterType GetWriterType() const;
    void SetWriterType(WriterType type);
    std::string WriterTypeToString(WriterType type);
    // 辅助函数：根据bus的数据类型创建对应的 CircularBuffer 对象
    static SystemVueModelBuilder::CircularBufferBase* CreateCircularBufferByDataType(DataType dataType);

    // 总线连接管理函数
    const std::vector<OutPutBusConnection>& GetBusConnections() const; //获取bus链接容器
    size_t GetBusConnectionCount() const; //获取bus链接的数量
    void AddBusConnection(const OutPutBusConnection& connection);
    void ClearBusConnections();
    // 设置指定索引的总线连接的是否允许写入标志
    void SetBusConnectionPermitWrite(size_t connectionIndex, bool permit);
    // 获取指定索引的总线连接的是否允许写入标志
    bool GetBusConnectionPermitWrite(size_t connectionIndex) const;

    static bool IsBusType(DataType type);
    //--------------------------------------------------------------
    // 设置和获取表征频率，适用于包络信号
    void setCharacterizationFrequency(double fc);
    double getCharacterizationFrequency() const;
    bool hasCharacterizationFrequency() const;
    void PropagateCharacterizationFrequencyFromInput();
    //--------------------------------------------------------------
    template<typename T>
    void SetExternalBasicTypeOutputPorts(const std::string& portName, T& externalPorts)
    {
        m_externalBasicOutputPorts[portName] = std::ref(externalPorts);
    }
    //--------------------------------------------------------------
    //设置模型 基础类型端口（不含缓冲区）
    void SetExternalIntPort(const std::string& portName, int value);
    std::map<std::string, int> GetExternalIntPorts() const;
    std::map<std::string, int>& GetExternalIntPortsRef();

    void SetExternalDoublePort(const std::string& portName, double value);
    std::map<std::string, double> GetExternalDoublePorts() const;
    std::map<std::string, double>& GetExternalDoublePortsRef();

    void SetExternalFloatPort(const std::string& portName, float value);
    std::map<std::string, float> GetExternalFloatPorts() const;
    std::map<std::string, float>& GetExternalFloatPortsRef();

    void SetExternalBoolPort(const std::string& portName, bool value);
    std::map<std::string, bool> GetExternalBoolPorts() const;
    std::map<std::string, bool>& GetExternalBoolPortsRef();

    void SetExternalFComplexPort(const std::string& portName, std::complex<float> value);
    std::map<std::string, std::complex<float>> GetExternalFComplexPorts() const;
    std::map<std::string, std::complex<float>>& GetExternalFComplexPortsRef();

    void SetExternalDComplexPort(const std::string& portName, std::complex<double> value);
    std::map<std::string, std::complex<double>> GetExternalDComplexPorts() const;
    std::map<std::string, std::complex<double>>& GetExternalDComplexPortsRef();
    //--------------------------------------------------------------
    //容量控制接口
    std::vector<BufferReader*> GetReaders() const; //获取输出端口连接的所有读指针
    void SetMaxSize(size_t maxSize);
    size_t GetMaxSize() const;

    // 获取当前使用率 (0-100)
    float GetUsage() const;
    //--------------------------------------------------------------
    // 背压回调
    using BackpressureCallback = std::function<void(Buffer*, bool isBackpressured)>;
    void setBackpressureCallback(BackpressureCallback callback);
    void NotifySpaceAvailable();// 空间释放通知（当读取数据后调用）
    // 获取写入器指针（用于背压通知）
    void* GetWriter() const;
    void SetWriter(void* writer);

    //时间驱动
    // 引擎调用：设置为变步长模式
    void SetVariableMode(bool enabled) { m_isVariableMode = enabled; }
    // 查询是否为变步长模式
    bool IsVariableMode() const { return m_isVariableMode; }
private:
    //基础设置
    std::string m_name;
    DataType m_dataType;

    // 默认不是变步长模式
    bool m_isVariableMode = false;

    // 实现类对象
    std::unique_ptr<BufferWriteImpl> m_writer;
    std::unique_ptr<BufferReadImpl> m_readerImpl;
    std::unique_ptr<BufferBusDataImpl> m_busProcessor;
    std::unique_ptr<BufferMemoryImpl> m_memoryImpl;
    std::unique_ptr<BufferExpansionImpl> m_expansionImpl;
    // 允许实现类访问私有成员
    friend class BufferWriteImpl;
    friend class BufferReadImpl;
    friend class BufferBusDataImpl;
    friend class BufferMemoryImpl;
    friend class BufferExpansionImpl;

    //缓冲区管理
    using CircularBufferVariant = std::variant<
    std::unique_ptr<SystemVueModelBuilder::IntCircularBuffer>,
    std::unique_ptr<SystemVueModelBuilder::DoubleCircularBuffer>,
    std::unique_ptr<SystemVueModelBuilder::FloatCircularBuffer>,
    std::unique_ptr<SystemVueModelBuilder::BoolCircularBuffer>,
    std::unique_ptr<SystemVueModelBuilder::DComplexCircularBuffer>,
    std::unique_ptr<SystemVueModelBuilder::FComplexCircularBuffer>,

    std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<int>>,
    std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<double>>,
    std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<float>>,
    std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<bool>>,
    std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>>>,
    std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<std::complex<float>>>,

    SystemVueModelBuilder::IntCircularBufferBus*,
    SystemVueModelBuilder::DoubleCircularBufferBus*,
    SystemVueModelBuilder::FloatCircularBufferBus*,
    SystemVueModelBuilder::BoolCircularBufferBus*,
    SystemVueModelBuilder::DComplexCircularBufferBus*,
    SystemVueModelBuilder::FComplexCircularBufferBus*,

    std::unique_ptr<SystemVueModelBuilder::EnvelopeCircularBuffer>,

    std::unique_ptr<SystemVueModelBuilder::IntMatrixCircularBuffer>,
    std::unique_ptr<SystemVueModelBuilder::DoubleMatrixCircularBuffer>,
    std::unique_ptr<SystemVueModelBuilder::FloatMatrixCircularBuffer>,
    std::unique_ptr<SystemVueModelBuilder::BoolMatrixCircularBuffer>,
    std::unique_ptr<SystemVueModelBuilder::DComplexMatrixCircularBuffer>,
    std::unique_ptr<SystemVueModelBuilder::FComplexMatrixCircularBuffer>,
    std::unique_ptr<SystemVueModelBuilder::EnvelopeMatrixCircularBuffer>,

    std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<IntMatrix>>,
    std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<DoubleMatrix>>,
    std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<FloatMatrix>>,
    std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<BoolMatrix>>,
    std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<FComplexMatrix>>,
    std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<DComplexMatrix>>
        >;
    CircularBufferVariant m_outputBuffer; //内部缓冲区的指针

    // 容量控制
    size_t m_maxSize = 1024;// 最大容量限制，默认1024 保留
    size_t m_maxBufferSize = 1024 * 1024 * 10;  // 缓冲区内存上限，默认10MB 用于扩容检查
    size_t m_trueOriginalBufferSize = 1024;  // 记录真实的原始大小，用于恢复判断
    // 背压回调
    BackpressureCallback m_backpressureCallback;
    void* m_writerPtr = nullptr;// 写入此Buffer的Block指针


    //读写控制
    size_t m_dataCount; //当前写入数量
    size_t m_bufferSize;//缓冲区大小
    size_t m_totalWritten = 0;   //总写入数量
    size_t m_writePosition = 0;  //写入指针位置
    size_t m_writeSize;          //写入指针大小
    // 每个reader的独立读取位置（绝对坐标）
    std::unordered_map<std::string, size_t> m_readerPositions;     //所有读指针位置
    std::unordered_map<std::string, BufferReader*> m_readerObjects;//所有读指针
    std::vector<size_t> m_readerReadSizes;                         //所有读指针大小
    std::vector<size_t> m_readerReadPositions;
    std::vector<std::string> m_readerNames;

    //检查下游是否完成
    bool m_upstreamDone = false;

    //表征频率存储 - 包络信号 Fc
    double m_envelopeFc;  // 表征频率
    bool m_hasEnvelopeFc; // 是否设置了表征频率

    //-------------------------------------------------------------
    //输出端 - 外部缓冲区的指针
    //普通类型
    SystemVueModelBuilder::CircularBufferBase* m_externalCircularBuffer = nullptr;
    bool m_usingExternalCircularBuffer = false;
    std::shared_ptr<void> m_allocatedMemory;
    //Bus类型
    SystemVueModelBuilder::CircularBufferBus* m_externalCircularBufferBus = nullptr;
    bool m_usingExternalCircularBufferBus = false;
    WriterType m_writerType;
    std::vector<OutPutBusConnection> m_busConnections;  // 总线连接列表
    bool m_isBusType = false;                     // 标记是否为总线类型
    //外部基础类型端口存储
    std::map<std::string, std::any> m_externalBasicOutputPorts;
    std::map<std::string, int> m_externalIntBasicPorts;
    std::map<std::string, double> m_externalDoubleBasicPorts;
    std::map<std::string, float> m_externalFloatBasicPorts;
    std::map<std::string, bool> m_externalBoolBasicPorts;
    std::map<std::string, std::complex<float>> m_externalFComplexBasicPorts;
    std::map<std::string, std::complex<double>> m_externalDComplexBasicPorts;
    //-------------------------------------------------------------
    // 用于存储总线数据的缓冲区
    std::vector<std::unique_ptr<SystemVueModelBuilder::CircularBufferBase>> m_busPortBuffers;

    size_t GetBusBufferFreeSpace() const;
    bool IsBusDownstreamDone() const;
    bool CheckAllBusReaderHaveData(const std::string& readerName, size_t readSize);
    //-------------------------------------------------------------
    //获取最慢读指针的位置
    size_t FindSlowestReaderPosition() const;

    size_t m_originalBufferSize;    //原始缓冲区大小
    bool m_isExpanded;              //是否处于扩容状态
    size_t m_expansionStartPoint;   //扩容起始点
    // 计算最小公倍数和最大公约数
    size_t CalculateLCM(size_t a, size_t b);
    size_t CalculateGCD(size_t a, size_t b);
    //扩容用于读取
    bool ExpandBufferForRead(size_t requiredSize, const std::string& readerName);
    //恢复buffer到扩容前的大小
    bool RestoreBufferSize(size_t newSize);
    //读取后检查
    void RearrangeBufferAfterRead(const std::string& readerName, size_t readSize);

    //动态扩容方法
    bool SmartExpandIfNeeded(size_t requiredWriteSize, size_t requiredReadSize);
    //自动恢复方法
    void AutoRestoreIfPossible();

    //-------------------------------------------------------------
public:
    // 辅助函数定义
    template<typename CircularBufferType>
    using circular_buffer_value_t = typename circular_buffer_value_type<CircularBufferType>::type;
};

}
#endif // BUFFER_H
