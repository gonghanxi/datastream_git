#ifndef BLOCK_H
#define BLOCK_H

// #include "DFInterface.h"
//#include "ILogWriter.h"

#include "../Common/ILogWriter.h"
#include "../Common/LogExport.h"
//仿真器参数 — 定义在 Common/SimuParameter.h 中，通过以下 include 引入
#include "../Common/SimuParameter.h"

#include "Buffer.h"
#include "BufferReader.h"
#include "BusConnection.h"
#include "DataStreamVerification.h"
#include "DataTypesAndParsers.h"
#include "SystemVue.h"

#include <string>
#include <map>
#include <iostream>
#include <set>
#include <variant>
#include <mutex>


namespace SystemVueModelBuilder {
struct PortMsg
{
    enum PortDataType {
        INT,
        COMPLEX,
        ANYTYPE,
        ENVELOPE,
        REAL,
        FIXEDPOINT,
        VARIANT,
        MULTIPLE_INT,
        MULTIPLE_COMPLEX,
        MULTIPLE_ANYTYPE,
        MULTIPLE_ENVELOPE,
        MULTIPLE_REAL,
        MULTIPLE_FIXEDPOINT,
        MULTIPLE_VARIANT,
        INT_MATRIX,
        COMPLEX_MATRIX,
        ANYTYPE_MATRIX,
        ENVELOPE_MATRIX,
        REAL_MATRIX,
        FIXEDPOINT_MATRIX,
        VARIANT_MATRIX,
        MULTIPLE_INT_MATRIX,
        MULTIPLE_COMPLEX_MATRIX,
        MULTIPLE_ANYTYPE_MATRIX,
        MULTIPLE_ENVELOPE_MATRIX,
        MULTIPLE_REAL_MATRIX,
        MULTIPLE_FIXEDPOINT_MATRIX,
        MULTIPLE_VARIANT_MATRIX
    };
    int id;
    QString putType;
    QString name;
    PortDataType dataType;
    int topProtId;
    bool isOptional;
    unsigned int portRate;
};
//接收外部参数类
class Parameter
{
public:
    std::string Value;
    std::string Name;
};


class BlockPortImpl;
class BlockSinkImpl;
class Block
{
public:
    using DataType = Buffer::DataType;
    enum class BlockType{
        SOURCE,     // 信号源：无输入，有输出
        PROCESSOR,  // 处理器：有输入，有输出
        SINK        // 终端块：有输入，无输出
    };
    enum class TimeStampMode{
        ONTIME,     //正常传递时间戳
        NOTIME      //不传递时间戳
    };
    enum class BlockState{
        IDLE,       //空闲状态
        STARTED,    //已启动
        STOPPED,    //已停止
        DONE        //已完成
    };
    enum class TerminalMode {
        AUTO,       // 自动模式，根据时间范围
        SAMPLES,    // 采样模式，根据采样索引
        TIME        // 时间模式，根据时间戳
    };
public:

    //DDS - 事件节拍
    void setEventSize(int eventSize)
    {
        EventSize = eventSize;
    }

    int getEventSize() const
    {
        return EventSize;
    }

    //DDS服务 - 周期: 时间节拍
    void setCurStep(int curStep)
    {
        m_CurentStep = curStep;
    }
    int getCurStep() const
    {
        return m_CurentStep;
    }
    //DDS服务 - 周期: 输出路径
    void setSinkOutPutPath(const std::string &outPutPath)
    {
        m_sinkOutPutPath = outPutPath;
    }
    std::string getSinkOutPutPath()
    {
        return m_sinkOutPutPath;
    }


    // 引擎设置用户ID
    void setUserId(const std::string& id)
    {
        m_UserId = id;
    }
    std::string getUserId()
    {
        return m_UserId;
    }
    // 引擎设置模型参数接口
    void setParameter(const Parameter &param)
    {
        parameters[param.Name]=param;
    }

    // 引擎设置仿真软件初始化参数
    void setSimuParams(const SimuParameter& simu)
    {
        m_simuPara=simu;
    }

    // 引擎设置输出服务器文件路径
    void setOutPutPath(const std::string &outPutPath)
    {
        m_outPutPath=outPutPath;
    }
    // 引擎获取子系统名称
    std::string getSubsystemName() const {
        return m_subsystemName;
    }
    // 引擎设置子系统名称
    void setSubsystemName(const std::string& subsystemName) {
        m_subsystemName = subsystemName;
    }


    // 引擎设置日志参数
    void setLogWriter(ILogWriter *writer)
    {
        mWriter=writer;
    }

    // 引擎设置instanceName
    void setInstanceName(const std::string &instanceName)
    {
        m_name=instanceName;
    }
    // 引擎获取instanceName
    const std::string& getInstanceName() const {
        return m_name;
    }

    // 引擎设置Block id
    void setId(int id)
    {
        this->id=id;
    }
    // 引擎获取Block id
    int getId() const
    {
        return this->id;
    }

    QMap<int, PortMsg> getPortsMsg() const;
    void setPortsMsg(const QMap<int, PortMsg> &value);
    // 获取引擎设置的模型参数
    Parameter getParameter(const std::string& name)
    {
        return parameters[name];
    }
    // 获取引擎设置的所有模型参数
    const std::map<std::string,Parameter> getAllParameter()
    {
        return parameters;
    }

    // 获取引擎设置的仿真器参数
    SimuParameter getSimu()
    {
        return m_simuPara;
    }

    // 获取引擎设置的输出路径
    std::string getOutPutPath()
    {
        return m_outPutPath;
    }
    int getId()
    {
        return id;
    }

    //写日志接口
    void writeLog(const std::string &msg)
    {
        qDebug()<<QString(msg.c_str());
        if(mWriter)
        {
            mWriter->write(msg);
        }
    }
    //--------------------------------------------------------------
    // 引擎时间调度器
    // 引擎时间调度器设置当前时间
    virtual void SetCurrentTime(double time) { m_currentTime = time; }
    // 获取当前仿真时间
    double GetCurrentTime() const { return m_currentTime; }
    // 变步长
    // 引擎时间调度器 - 变步长获取模型的时间步长（保留）
    // 获取模型期望的时间步长
    // 返回0表示跟随全局步长（每步都执行）
    // 返回>0表示模型期望的步长（如64倍基础步长）
    virtual double GetDesiredTimeStep() const { return 0.0; }
    // 获取模型能接受的最小步长
    // 返回0表示无限制
    virtual double GetMinimumTimeStep() const { return 0.0; }
    // 判断在指定时间是否需要执行
    // 默认每步都执行
    virtual bool ShouldExecuteAt(double time) const { return true; }
    // 获取本次执行产出的数据点数
    // 对于64:64模型返回64，对于1:1模型返回1
    virtual int GetOutputDataCount() const { return 1; }

    // 获取模型需要累积的输入数据点数
    // 对于64:64模型返回64，对于5:1模型返回5，对于1:5模型返回1
    virtual int GetInputAccumulateCount() const { return 1; }

    // 设置下次执行时间
    virtual void SetNextExecutionTime(double time) { m_nextExecTime = time; }

    // 获取下次执行时间
    double GetNextExecutionTime() const { return m_nextExecTime; }

    // 获取模型采样周期（基础步长的倍数）
    double GetSamplePeriod() const { return m_samplePeriod; }

    // 设置模型采样周期
    void SetSamplePeriod(double period) { m_samplePeriod = period; }

    // 设置抽取/插值因子
    void SetDecimationFactor(int factor) { m_decimationFactor = factor; }
    int GetDecimationFactor() const { return m_decimationFactor; }

    // 引擎调用：设置为变步长模式
    void SetVariableStepMode(bool enabled)
    {
        m_isVariableStepMode = enabled;
    }
    // 查询是否为变步长模式
    bool IsVariableStepMode() const { return m_isVariableStepMode; }

    // ========== 事件驱动模式（ZeroCross） ==========
    void SetEventDrivenMode(bool enabled) { m_isEventDrivenMode = enabled; }
    bool IsEventDrivenMode() const { return m_isEventDrivenMode; }

    void SetCurrentIteration(unsigned long long iteration) { m_currentIteration = iteration; }
    unsigned long long GetCurrentIteration() const { return m_currentIteration; }

    // 调度器设置：本次迭代是否跳过数据输出（ZeroCross触发时）
    void SetSkipDataOutput(bool skip) { m_skipDataOutput = skip; }
    bool ShouldSkipDataOutput() const { return m_skipDataOutput; }
    //--------------------------------------------------------------
    Block();
    Block(const std::string& name);
    virtual ~Block();

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
    //Bus类型
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

    template<typename BufferType, typename MemoryType>
    Buffer* AddOutputPort(const std::string& portName, BufferType& externalPort,
                          size_t writeSize, DataType dataType, MemoryType* bufferMemory, size_t memorySize = 1024)
    {
        //检查是否添加过相同端口
        if(m_outputPorts.find(portName) != m_outputPorts.end()) {
            qDebug() << QString::fromStdString(m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
            return m_outputPorts[portName];
        }
        bufferMemory = new MemoryType[memorySize];
        externalPort.SetBuffer(bufferMemory, memorySize, 1);
        externalPort.Initialize();
        Buffer* buffer = new Buffer(portName, writeSize, dataType);

        bool success = buffer->SetExternalCircularBuffer(&externalPort);
        if (!success) {
            qDebug() << "ERROR: Failed to set external circular buffer";
            delete[] bufferMemory;
            delete buffer;
            return nullptr;
        }
        buffer->EnsureCircularBuffer();

        m_outputPorts[portName] = buffer;
        m_outputPortDataTypes[portName] = dataType;
        m_outputPortNames.push_back(portName);
        m_outputPortNameToIndex[portName] = m_outputPortNames.size() - 1;

        return buffer;
    }
    //--------------------------------------------------------------
    //设置输出文件
    void SetOutputFile(const std::string& filename);
    std::string GetOutputFile() const;

    //上下游连接方法
    static void Connect(Block* upstreamBlock, const std::string& upstreamOutputPort,
                        Block* downstreamBlock, const std::string& downstreamInputPort);

    //矩阵校验
    // 设置和获取约束系统
    static void SetVerificationSystem(std::shared_ptr<DataStreamVerification> system);
    static std::shared_ptr<DataStreamVerification> GetVerificationSystem();

    // 获取块的系数权重（可被子类重写）
    virtual double getUpCoefficient() const { return 1.0; }
    virtual double getDownCoefficient() const { return -1.0; }

    // 获取总线通道数（用于 bus-to-bus 连接检测，-1 表示未定义）
    virtual int GetBusChannelCount() const { return -1; }

    // 延迟 bus-to-bus 连接记录（两端均无通道数参数时暂存）
    struct DeferredBusConnection {
        Block* upstreamBlock;
        std::string upstreamOutputPort;
        Block* downstreamBlock;
        std::string downstreamInputPort;
        DataType outputDataType;
    };

    // 解析所有延迟的 bus-to-bus 连接（在所有 Connect() 完成后调用）
    static void ResolveAllDeferredBusConnections();
    //--------------------------------------------------------------
    //设置Block的名称
    void SetName(const std::string& portName);
    std::string GetName() const;

    //设置Block的类型/端口的数据类型
    BlockType GetBlockType() const;
    void SetBlockType(BlockType type);
    DataType GetDataType() const;
    void SetDataType(DataType type);
    //--------------------------------------------------------------
    //判断是否可以执行处理
    bool CanProcess();

    //Block的调度方法
    virtual bool Run();
    virtual bool Initialize();
    virtual bool Setup();
    virtual bool Stop();
    virtual bool Done();
    virtual bool Flush();
    virtual bool IsCollectionComplete();
    bool IsDone() const;
    void SetDone(bool done);
    //判断所有输出端口是否准备好接收数据
    bool AreOutputPortsReady();
    //--------------------------------------------------------------
    //终端块处理
    bool ProcessAsTerminalBlock(const std::string& inputPortName);
    bool IsTerminalBlock() const;
    //--------------------------------------------------------------
    //获取单个端口名称
    const std::string& GetInputPortName(size_t index = 0) const;
    const std::string& GetOutputPortName(size_t index = 0) const;
    //获取单个端口
    BufferReader* GetInputPort(const std::string& portName);
    Buffer* GetOutputPort(const std::string& portName);
    //获取单个端口索引
    size_t GetInputPortIndex(const std::string& portName) const;
    size_t GetOutputPortIndex(const std::string& portName) const;
    //获取端口名称容器
    const std::vector<std::string>& GetAllInputPortNames() const;
    const std::vector<std::string>& GetAllOutputPortNames() const;
    //获取端口数量
    size_t GetInputPortCount() const;
    size_t GetOutputPortCount() const;
    //获取端口容器
    std::map<std::string, BufferReader*> GetInputPorts() const;
    std::map<std::string, Buffer*> GetOutputPorts() const;
    //获取Block状态
    BlockState GetState() const;
    //--------------------------------------------------------------
    //---------------------获取总线连接信息---------------------------
    //获取输入端口总线连接数量
    size_t GetBusConnectionCount(const std::string& inputPortName) const;
    size_t GetBusConnectionCountForReader(const std::string& portName) const;
    //为输入端口添加单个总线连接
    void AddBusConnection(const std::string& inputPortName, const BusConnection& connection);
    //获取输入端口总线连接的容器
    const std::vector<BusConnection>& GetBusConnections(const std::string& inputPortName) const;
    const std::vector<BusConnection>& GetBusConnectionsForReader(const std::string& readerName) const;

    //--------------------------------------------------------------
    //判断下游是否读取完成
    bool IsDownstreamDone();
    //---------------------终端处理----------------
    void SetTerminalMode(TerminalMode mode);
    void SetTimeRange(double start, double stop);
    void SetSampleRange(size_t start, size_t stop);
    //--------------------------------------------------------------
    //模板函数：从输入端口读取数据
    template<typename T>
    std::vector<T> ReadInputData(const std::string& inputPortName)
    {
        BufferReader* reader = GetInputPort(inputPortName);
        if (!reader) {
            qDebug() << "ERROR: Reader not found for port: " << QString::fromStdString(inputPortName);
            return std::vector<T>();
        }

//        qDebug() << "=== Block::ReadInputData ===";
//        qDebug() << "Port: " << QString::fromStdString(inputPortName);
//        qDebug() << "Reader: " << QString::fromStdString(reader->GetName())
//                  << ", Type: " << reader->GetReaderType();

        std::vector<T> data;

        // 直接读取，让 BufferReader 处理不同类型
        bool success = reader->ReadData(data);

        if (success) {
//            qDebug() << "Successfully read " << data.size() << " samples";
        } else {
            //LOG_INFO("Failed to read data");
        }

        return data;
    }
    //模板函数：从临时缓冲区读取数据
    template<typename T>
    std::vector<T> ReadFromTempBuffer(BufferReader* reader, const std::string& portName)
    {
        qDebug() << "=== ReadFromTempBuffer (Block) ===";
        qDebug() << "Port: " << QString::fromStdString(portName);

        // 首先读取数据到临时缓冲区
        if (!reader->ReadToTempBuffer()) {
            qDebug() << "Failed to read data to temp buffer";
            return std::vector<T>();
        }

        // 然后从临时缓冲区读取
        std::vector<T> data;
        if (!reader->ReadFromTempBuffer(data, reader->GetReadSize())) {
            qDebug() << "Failed to read from temp buffer";
            return std::vector<T>();
        }

        qDebug() << "Successfully read " << data.size() << " samples from temp buffer";

        return data;
    }

    // 数据写入输出端口
    template<typename T>
    bool WriteOutputData(const std::string& outputPortName, const std::vector<T>& data)
    {
        if (data.empty()) {
            return false;
        }

        Buffer* outputBuffer = GetOutputPort(outputPortName);
        if (!outputBuffer) {
            qDebug() << "WARNING: Output port '" << QString::fromStdString(outputPortName) << "' not found";
            return false;
        }
        if(std::is_same_v<T, SystemVueModelBuilder::EnvelopeSignal>) {
            for(auto Reader : m_inputPorts) {
                if (Reader.second->hasCharacterizationFrequency()) {
                    double fc = Reader.second->getCharacterizationFrequency();
//                    qDebug() << "Got characterization frequency " << fc
//                             << " from reader '" << QString::fromStdString(Reader.first) << "'";
                    outputBuffer->setCharacterizationFrequency(fc);
                    }
                }

        }
        // 设置写入器指针
        outputBuffer->SetWriter(this);

        return outputBuffer->WriteData(data);
    }

    // 判断输出端口是否为 bus-to-bus 连接（输出BUS且至少一个下游输入也是BUS类型）
    bool IsOutputBusToBus(const std::string& outputPortName)
    {
        Buffer* outputBuffer = GetOutputPort(outputPortName);
        if (!outputBuffer) return false;
        for (const auto& conn : outputBuffer->GetBusConnections()) {
            if (conn.downstreamBlock) {
                BufferReader* downstreamInput = conn.downstreamBlock->GetInputPort(conn.downstreamPortName);
                if (downstreamInput && BufferReader::IsBusType(downstreamInput->GetDataType()))
                    return true;
            }
        }
        return false;
    }

    //变速率模型
    /**
     * @brief 获取指定输入端口需要的数据量才能进行一次处理
     * @param portName 端口名称
     * @return 需要的数据量，默认返回1
     *
     * 子类重写场景：
     * - Reverse模型：如果设置倒序长度N，则需要N个数据才能输出
     * - 抽取器：需要M个数据输出1个
     * - 插值器：需要1个数据输出L个
     */
    virtual int GetRequiredInputCount(const std::string& portName) const;

    /**
     * @brief 获取所有输入端口中最大的需求量
     * @return 最大需求量
     *
     * 用于快速判断：如果所有端口都满足最大需求量，则所有端口都满足
     */
    virtual int GetMaxRequiredInputCount() const;

    /**
     * @brief 获取建议的批量处理大小
     * @return 批量大小，默认返回1
     *
     * 子类可重写以支持批量处理，提高效率
     */
    virtual int GetBatchSize() const;

    /**
     * @brief 批量执行处理
     * @param maxCount 最大处理次数
     * @return 实际处理次数
     *
     * 子类可重写以实现高效的批量处理
     */
    virtual int RunBatch(int maxCount);

    // ========== 背压 ==========
    bool IsBackpressured() const;
    void SetBackpressured(bool backpressured);

    // ========== 过零检测标志 ==========
    bool IsZeroCrossTriggered() const;
    void SetZeroCrossTriggered(bool triggered);

    // ========== 过零检测类型标识（非虚函数，Initialize时设置） ==========
    void SetIsZeroCrossType(bool isZC) { m_isZeroCrossType = isZC; }
    bool IsZeroCrossType() const { return m_isZeroCrossType; }

    // 获取下游Buffer使用率（用于动态批量调整）
    float GetDownstreamBufferUsage() const;

    // 获取建议的批量大小（根据下游背压动态调整）
    int GetRecommendedBatchSize() const;

    // ========== 模型 ==========
    void SetIsBitShiftRegister(bool IsBitShiftRegister);
    bool IsBitShiftRegister() const;
    void SetBitShiftRegisterNumBits(int NumBits);
    int GetBitShiftRegisterNumBits() const;
private:
    //基础设置
    std::string m_name;
    BlockType m_blockType;
    DataType m_dataType;
    SystemVueModelBuilder::DFInterface m_dfinterface;
    std::string m_sinkOutPutPath;//收集器输出完整路径
    //DDS服务 - 周期: 时间节拍
    int m_CurentStep = 0;

    //Bus类型连接计数
    static int m_OutPutbusConnectionCount;
    static int m_busConnectionCount;

    //功能实现类
    std::unique_ptr<BlockPortImpl> m_porter;
    std::unique_ptr<BlockSinkImpl> m_sinker;
    friend class SystemVueModelBuilder::BlockPortImpl;
    friend class SystemVueModelBuilder::BlockSinkImpl;


    //所有端口数据类型
    std::map<std::string, DataType> m_inputPortDataTypes;
    std::map<std::string, DataType> m_outputPortDataTypes;

    //所有端口
    std::map<std::string, BufferReader*> m_inputPorts;
    std::map<std::string, Buffer*> m_outputPorts;

    //所有输出端口写指针大小
    std::unordered_map<std::string, size_t> m_outputSizes;

    // 端口名称容器
    std::vector<std::string> m_inputPortNames;
    std::vector<std::string> m_outputPortNames;

    // 端口名称到索引的映射
    std::unordered_map<std::string, size_t> m_inputPortNameToIndex;
    std::unordered_map<std::string, size_t> m_outputPortNameToIndex;

    //端口id-name
    std::map<int, std::string> m_portName;

    //端口id-putType
    std::map<int, std::string> m_portPutType;

    //矩阵链路校验
    static std::shared_ptr<DataStreamVerification> s_VerificationSystem;

    // 运行
    bool m_isDone;
    BlockState m_state;

    //--------------------------------------------------------------
    // 终端写入json运行
    TerminalMode m_terminalMode = TerminalMode::AUTO;
    double m_timeStart = 0.0;
    double m_timeStop = std::numeric_limits<double>::max();
    size_t m_sampleStart = 0;
    size_t m_sampleStop = std::numeric_limits<size_t>::max();
    double m_samplingRate = 0.0;
    double m_samplingRateIncrement = 0.0;
    size_t m_processedCount = 0;  // 已处理的数据计数

    //写入文件
    std::ofstream m_outputFile;
    std::string m_outputFilename;
    std::mutex m_fileMutex;  // 文件写入互斥锁

    QMap<int,PortMsg> m_PortMessage;
    //--------------------------------------------------------------
    //总线映射
    std::unordered_map<std::string, std::vector<BusConnection>> m_busConnections;
    std::unordered_map<std::string, std::vector<std::pair<Block*, std::string>>> m_connectedBusUpstreamBlocks;
    //总线转普通类型的校验
    static DataType BusToCircularBuffer(DataType type);

    // 延迟 bus-to-bus 连接列表（两端 GetBusChannelCount 均返回 -1 时暂存）
    static std::vector<DeferredBusConnection> s_deferredBusConnections;
    //--------------------------------------------------------------



    //背压
    bool m_isBackpressured = false;
    //过零检测触发标志
    bool m_isZeroCrossTriggered = false;
    //过零检测类型标识（Initialize时由ZeroCross_Block设置）
    bool m_isZeroCrossType = false;
    // ========== 模型 ==========
    bool m_IsBitShiftRegister;
    int m_NumBits;
private:
    //引擎接收参数
    int id;
    std::map<std::string,Parameter> parameters;
    ILogWriter* mWriter=NULL;
    SimuParameter m_simuPara;     //仿真器参数
    std::string m_outPutPath;     //输出文件路径
    std::string m_subsystemName;  //子系统名称
    std::string m_UserId;         //用户ID
    //引擎调度器
    double m_currentTime = 0.0;           // 当前仿真时间
    double m_nextExecTime = 0.0;          // 下次执行时间
    double m_samplePeriod = 0.0;          // 模型采样周期
    int m_decimationFactor = 1;           // 抽取/插值因子
    bool m_isVariableStepMode = false;  // 默认不是变步长模式
    bool m_isEventDrivenMode = false;   // 默认不是事件驱动模式
    unsigned long long m_currentIteration = 0; // 当前迭代计数（事件驱动模式）
    bool m_skipDataOutput = false;      // 本次迭代是否跳过数据输出（ZeroCross触发）

    //事件节拍 - 仿真次数
    int EventSize = 0;
public:
    //---------------------解析矩阵参数----------------
    template<typename T>
    SystemVueModelBuilder::Matrix<T> ParseStringToMatrix(const std::string& str) {
        // 通过 DataTypeParser 调用具体类型的解析函数
        if constexpr (std::is_same_v<T, double>) {
            return DataTypesAndParsers::ParseStringToMatrixDouble(str);
        } else if constexpr (std::is_same_v<T, int>) {
            return DataTypesAndParsers::ParseStringToMatrixInt(str);
        } else if constexpr (std::is_same_v<T, bool>) {
            return DataTypesAndParsers::ParseStringToMatrixBool(str);
        } else if constexpr (std::is_same_v<T, float>) {
            return DataTypesAndParsers::ParseStringToMatrixFloat(str);
        } else if constexpr (std::is_same_v<T, std::complex<float>>) {
            return DataTypesAndParsers::ParseStringToMatrixFComplex(str);
        } else if constexpr (std::is_same_v<T, std::complex<double>>) {
            return DataTypesAndParsers::ParseStringToMatrixDComplex(str);
        } else if constexpr (std::is_same_v<T, char>) {
            return DataTypesAndParsers::ParseStringToMatrixChar(str);
        } else {
            throw std::runtime_error("Unsupported matrix type");
        }
    }
};
//导出函数
#define RegAlgo(classname) \
    extern "C" SYSTEMVUEMODELBUILDER_API Block* createAlgorithm() { \
        return new classname(#classname); \
    } \
    extern "C" SYSTEMVUEMODELBUILDER_API const char* getAlgorithmName() { \
        return #classname; \
    }

}
#endif // BLOCK_H
