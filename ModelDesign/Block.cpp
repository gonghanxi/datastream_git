#include "Block.h"
#include "BlockPortImpl.h"
#include "BlockSinkImpl.h"

#include <variant>
#include <algorithm>

using namespace SystemVueModelBuilder;

//初始化校验变量
std::shared_ptr<DataStreamVerification> Block::s_VerificationSystem = nullptr;
int Block::m_busConnectionCount = 0;
int Block::m_OutPutbusConnectionCount = 0;
std::vector<Block::DeferredBusConnection> Block::s_deferredBusConnections;

Block::Block()
    :m_blockType(BlockType::SOURCE)
    ,m_dataType(DataType::INT)
    ,m_isDone(false)
    ,m_state(BlockState::IDLE)
    ,m_isBackpressured(false)
    ,m_isZeroCrossTriggered(false)
    ,m_isZeroCrossType(false)
    ,m_IsBitShiftRegister(false)
    ,m_NumBits(1)
    ,id(0)
{
    //实现类指针初始化
    m_sinker = std::make_unique<BlockSinkImpl>(this);
    m_porter = std::make_unique<BlockPortImpl>(this);
    //仿真软件采样率/采样增量初始化
    m_samplingRate = m_simuPara.samplingRate;
    m_samplingRateIncrement = m_samplingRate;
}

Block::Block(const std::string &name)
    : m_name(name)
    ,m_blockType(BlockType::SOURCE)
    ,m_dataType(DataType::INT)
    ,m_isDone(false)
    ,m_state(BlockState::IDLE)
    ,m_isBackpressured(false)
    ,m_isZeroCrossTriggered(false)
    ,m_isZeroCrossType(false)
    ,m_IsBitShiftRegister(false)
    ,m_NumBits(1)
    ,id(0)
{
    //实现类指针初始化
    m_sinker = std::make_unique<BlockSinkImpl>(this);
    m_porter = std::make_unique<BlockPortImpl>(this);
    //模型名称初始化
    m_dfinterface.SetModelName(name.c_str());
    //仿真软件采样率/采样增量初始化
    m_samplingRate = m_simuPara.samplingRate;
    m_samplingRateIncrement = m_samplingRate;
}

Block::~Block()
{
    // 1. 先删除input ports (BufferReader)
    for (auto& inputPort : m_inputPorts) {
        delete inputPort.second;
    }
    m_inputPorts.clear();

    // 2. 然后删除output ports (Buffer)
    for (auto& outputPort : m_outputPorts) {
        delete outputPort.second;
    }
    m_outputPorts.clear();



    qDebug() << "=== Block '" << QString::fromStdString(m_name) << "' destructor ===";
}

BufferReader *Block::AddInputPort(const std::string &portName, SystemVueModelBuilder::CircularBufferBus &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<double> &externalPort, size_t readSize, Block::DataType dataType)
{
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<int> &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<float> &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<bool> &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<std::complex<float> > &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<std::complex<double> > &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, IntCircularBuffer &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, FloatCircularBuffer &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, FComplexCircularBuffer &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, DComplexCircularBuffer &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, SystemVueModelBuilder::CircularBuffer<double> &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, BoolCircularBuffer &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, SystemVueModelBuilder::EnvelopeCircularBuffer &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, int &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, double &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, float &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, bool &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, std::complex<float> &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, std::complex<double> &externalPort, size_t readSize, Block::DataType dataType)
{
    //添加输入端口
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, IntMatrixCircularBuffer &externalPort, size_t readSize, Block::DataType dataType)
{
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, DoubleMatrixCircularBuffer &externalPort, size_t readSize, Block::DataType dataType)
{
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, FloatMatrixCircularBuffer &externalPort, size_t readSize, Block::DataType dataType)
{
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, BoolMatrixCircularBuffer &externalPort, size_t readSize, Block::DataType dataType)
{
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, FComplexMatrixCircularBuffer &externalPort, size_t readSize, Block::DataType dataType)
{
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, DComplexMatrixCircularBuffer &externalPort, size_t readSize, Block::DataType dataType)
{
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, EnvelopeMatrixCircularBuffer &externalPort, size_t readSize, Block::DataType dataType)
{
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<IntMatrix> &externalPort, size_t readSize, Block::DataType dataType)
{
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<DoubleMatrix> &externalPort, size_t readSize, Block::DataType dataType)
{
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<FloatMatrix> &externalPort, size_t readSize, Block::DataType dataType)
{
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<BoolMatrix> &externalPort, size_t readSize, Block::DataType dataType)
{
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<FComplexMatrix> &externalPort, size_t readSize, Block::DataType dataType)
{
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

BufferReader *Block::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<DComplexMatrix> &externalPort, size_t readSize, Block::DataType dataType)
{
    return m_porter->AddInputPort(portName, externalPort, readSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, CircularBufferBus &externalPort, size_t writeSize, Block::DataType dataType)
{
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, IntCircularBuffer &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, FloatCircularBuffer &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, BoolCircularBuffer &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, FComplexCircularBuffer &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}
Buffer *Block::AddOutputPort(const std::string &portName, SystemVueModelBuilder::DComplexCircularBuffer &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, SystemVueModelBuilder::DoubleCircularBuffer &externalPort, size_t writeSize, Block::DataType dataType) {
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<int> &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<float> &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<double> &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<bool> &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<std::complex<float> > &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<std::complex<double> > &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, double &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, std::complex<float> &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, std::complex<double> &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, IntMatrixCircularBuffer &externalPort, size_t writeSize, Block::DataType dataType)
{
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, FloatMatrixCircularBuffer &externalPort, size_t writeSize, Block::DataType dataType)
{
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, BoolMatrixCircularBuffer &externalPort, size_t writeSize, Block::DataType dataType)
{
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, DoubleMatrixCircularBuffer &externalPort, size_t writeSize, Block::DataType dataType)
{
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, FComplexMatrixCircularBuffer &externalPort, size_t writeSize, Block::DataType dataType)
{
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, DComplexMatrixCircularBuffer &externalPort, size_t writeSize, Block::DataType dataType)
{
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, EnvelopeMatrixCircularBuffer &externalPort, size_t writeSize, Block::DataType dataType)
{
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<IntMatrix> &externalPort, size_t writeSize, Block::DataType dataType)
{
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<DoubleMatrix> &externalPort, size_t writeSize, Block::DataType dataType)
{
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<FloatMatrix> &externalPort, size_t writeSize, Block::DataType dataType)
{
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<BoolMatrix> &externalPort, size_t writeSize, Block::DataType dataType)
{
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<FComplexMatrix> &externalPort, size_t writeSize, Block::DataType dataType)
{
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<DComplexMatrix> &externalPort, size_t writeSize, Block::DataType dataType)
{
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, SystemVueModelBuilder::EnvelopeCircularBuffer &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, int &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, float &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

Buffer *Block::AddOutputPort(const std::string &portName, bool &externalPort, size_t writeSize, Block::DataType dataType)
{
    //添加输出端口
    return m_porter->AddOutputPort(portName, externalPort, writeSize, dataType);
}

BufferReader* Block::GetInputPort(const std::string &portName)
{
    //遍历容器，根据端口名找到对应端口
    auto it = m_inputPorts.find(portName);
    if (it != m_inputPorts.end()) {
        BufferReader* reader = it->second;
        if (reader) {
            return reader;
        }
    }

    qDebug() << "ERROR: Block::GetInputPort - Reader not found for port: '"
              << QString::fromStdString(portName) << "' in block '" << QString::fromStdString(m_name) << "'";
    return nullptr;
}

Buffer* Block::GetOutputPort(const std::string &portName)
{
    //遍历容器，根据端口名找到对应端口
    auto it = m_outputPorts.find(portName);
    return (it != m_outputPorts.end()) ? it->second : nullptr;
}

std::string Block::GetName() const
{
    return m_name;
}

size_t Block::GetInputPortCount() const
{
    //返回输入端口数量
    return m_inputPorts.size();
}

size_t Block::GetOutputPortCount() const
{
    //返回输出端口数量
    return m_outputPorts.size();
}

std::map<std::string, BufferReader *> Block::GetInputPorts() const
{
    //返回输入端口容器
    return m_inputPorts;
}

std::map<std::string, Buffer *> Block::GetOutputPorts() const
{
    //返回输出端口容器
    return m_outputPorts;
}

void Block::SetBlockType(BlockType type)
{
    //设置Block的类型
    m_blockType = type;
}

void Block::SetDataType(DataType type)
{
    m_dataType = type;
}

Block::DataType Block::GetDataType() const
{
    return m_dataType;
}

Block::BlockType Block::GetBlockType() const
{
    return m_blockType;
}

void Block::Connect(Block* upstreamBlock, const std::string& upstreamOutputPort,
                    Block* downstreamBlock, const std::string& downstreamInputPort)
{
    //上下游链接方法
    if (!upstreamBlock || !downstreamBlock) {
        qDebug() << "model ptr error";
        return;
    }

    qDebug() << "UpBlock '" << QString::fromStdString(upstreamBlock->GetName()) << "' and DownBlock '" << QString::fromStdString(downstreamBlock->GetName());
    qDebug() << "': begin to connect!";
    qDebug() << "-----------";
    //获取上游输出端缓冲区和下游输入端读指针
    Buffer* outputBuffer = upstreamBlock->GetOutputPort(upstreamOutputPort);
    BufferReader* inputReader = downstreamBlock->GetInputPort(downstreamInputPort);

    if(!outputBuffer || !inputReader) {
        qDebug() << "port ptr error";
        return;
    }

    // 检查输入端是否为 Bus 类型
    DataType inputDataType = inputReader->GetDataType();
    bool isInputBusType = inputReader->IsBusType(inputDataType);
    // 检查输出端是否为 Bus 类型
    DataType outputDataType = outputBuffer->GetDataType();
    bool isOutputBusType = Buffer::IsBusType(outputDataType);
    // 1. 总线输入端：允许多个上游连接到同一个输入端
    // 2. 非总线输入端：保持单一连接限制
    // 对于非总线类型，检查是否已连接

    // 连接逻辑
    // 输出是BUS，输入也是BUS（bus-to-bus 直连，排除 Sink）
    if (isInputBusType && isOutputBusType && downstreamBlock->GetBlockType() != BlockType::SINK) {

        // 获取通道数
        int upCh = upstreamBlock->GetBusChannelCount();
        int downCh = downstreamBlock->GetBusChannelCount();

        // 场景3: 两端都没有通道数参数，延迟初始化
        if (upCh < 0 && downCh < 0) {
            DeferredBusConnection deferred;
            deferred.upstreamBlock = upstreamBlock;
            deferred.upstreamOutputPort = upstreamOutputPort;
            deferred.downstreamBlock = downstreamBlock;
            deferred.downstreamInputPort = downstreamInputPort;
            deferred.outputDataType = outputDataType;
            s_deferredBusConnections.push_back(deferred);

            // 记录连接关系（供后续查询）
            downstreamBlock->m_connectedBusUpstreamBlocks[downstreamInputPort].push_back(
                {upstreamBlock, upstreamOutputPort});

            qDebug() << "=== Bus-to-Bus 延迟连接 ==="
                     << "上游:" << QString::fromStdString(upstreamBlock->GetName())
                     << "下游:" << QString::fromStdString(downstreamBlock->GetName())
                     << "(两端均无通道数参数，等待 ResolveAllDeferredBusConnections)";
            return;
        }

        // 场景2: 一端有参数，另一端为 -1，使用有参数那一端的值
        // 场景1: 两端都有参数，校验一致性
        if (upCh > 0 && downCh > 0 && upCh != downCh) {
            LOG_ERROR("Bus-to-Bus 通道数不一致: ", upstreamBlock->GetName(),
                      "(", upCh, ") vs ", downstreamBlock->GetName(), "(", downCh, ")");
            return;
        }
        int channelCount = (upCh > 0) ? upCh : downCh;
        if (channelCount <= 0) channelCount = 1; // fallback

        qDebug() << "=== Bus-to-Bus 连接 ===";
        qDebug() << "上游:" << QString::fromStdString(upstreamBlock->GetName())
                 << "下游:" << QString::fromStdString(downstreamBlock->GetName())
                 << "通道数:" << channelCount;

        // 清理已有的 bus-to-bus 连接
        outputBuffer->ClearBusConnections();
        inputReader->ClearBusConnections();

        // 按通道数创建 N 组 bridge writer + channel buffer + bridge reader
        DataType bridgeDataType = BusToCircularBuffer(outputDataType);
        for (int ch = 0; ch < channelCount; ++ch) {
            // 1. 创建 bridge writer (BUS_BRIDGE)
            std::string wName = upstreamBlock->GetName() + "_bus2bus_w_" +
                                upstreamOutputPort + "_" + std::to_string(ch);
            Buffer* bridgeWriter = new Buffer(wName, outputBuffer->GetWriteSize(), bridgeDataType);
            bridgeWriter->SetWriterType(Buffer::BUS_BRIDGE);

            // 2. 创建 channel buffer
            CircularBufferBase* channelBuffer =
                    Buffer::CreateCircularBufferByDataType(bridgeDataType);
            if (!channelBuffer) {
                qDebug() << "ERROR: Failed to create CircularBuffer for bus2bus channel" << ch;
                delete bridgeWriter;
                continue;
            }
            bridgeWriter->SetExternalCircularBuffer(channelBuffer);

            // 3. 创建 bridge reader (BUS_BRIDGE)
            std::string rName = downstreamBlock->GetName() + "_bus2bus_r_" +
                                downstreamInputPort + "_" + std::to_string(ch);
            BufferReader* bridgeReader = new BufferReader(rName, inputReader->GetReadSize(), bridgeDataType);
            bridgeReader->SetReaderType(BufferReader::BUS_BRIDGE);
            bridgeReader->connectToBuffer(bridgeWriter);

            // 4. 注册到 outputBuffer 的 bus connections (输出侧分发)
            OutPutBusConnection outBusConn;
            outBusConn.downstreamBlock = downstreamBlock;
            outBusConn.downstreamPortName = downstreamInputPort;
            outBusConn.bridgeWriter = bridgeWriter;
            outBusConn.connectedReader = bridgeReader;
            outBusConn.isDownstreamDone = false;
            outBusConn.PermitWrite = true;
            outputBuffer->AddBusConnection(outBusConn);

            // 5. 注册到 inputReader 的 bus connections (输入侧收集)
            BusConnection inBusConn;
            inBusConn.upstreamBlock = upstreamBlock;
            inBusConn.upstreamPortName = upstreamOutputPort;
            inBusConn.bridgeReader = bridgeReader;
            inBusConn.connectedBuffer = bridgeWriter;
            inBusConn.isUpstreamDone = false;
            inputReader->AddBusConnection(inBusConn);

            qDebug() << "  通道" << ch << ": bridgeWriter=" << QString::fromStdString(wName)
                     << "bridgeReader=" << QString::fromStdString(rName);
        }

        // 设置主端口类型
        outputBuffer->SetWriterType(Buffer::BUS_MASTER);
        inputReader->SetReaderType(BufferReader::BUS_MASTER);

        // 记录连接关系
        downstreamBlock->m_connectedBusUpstreamBlocks[downstreamInputPort].push_back(
            {upstreamBlock, upstreamOutputPort});

        qDebug() << "Bus-to-Bus 连接完成，输出侧" << outputBuffer->GetBusConnectionCount()
                 << "个连接，输入侧" << inputReader->GetBusConnectionCount() << "个连接";
        return;
    }

    //输出是,输入不是
    if(!isInputBusType && isOutputBusType) {
        // 创建唯一的桥接读取器名称
        std::string bridgeName = upstreamBlock->GetName() + "_bus_bridge_" +
                upstreamOutputPort + "_" + std::to_string(m_OutPutbusConnectionCount);

        DataType bridgeDataType = DataType::CIRCULAR_BUFFER_INT;
        //根据主写入器的bus类型，添加对应CircularBuffer类型的桥接写入器
        bridgeDataType = BusToCircularBuffer(outputDataType);

        // 总线连接
        Buffer* bridgeWriter = new Buffer(bridgeName, outputBuffer->GetWriteSize(), bridgeDataType);
        bridgeWriter->SetWriterType(Buffer::BUS_BRIDGE);

        SystemVueModelBuilder::CircularBufferBase* channelBuffer =
                Buffer::CreateCircularBufferByDataType(bridgeDataType);

        if (!channelBuffer) {
            qDebug() << "ERROR: Failed to create CircularBuffer for data type: "
                     << static_cast<int>(bridgeDataType);
            delete bridgeWriter;
            return;
        }

        // 将 CircularBuffer 交给 bridgeWriter 管理
        bridgeWriter->SetExternalCircularBuffer(channelBuffer);

        qDebug() << "connect - bridgeWrite Datatype: " << static_cast<int>(bridgeWriter->GetDataType());
        if(bridgeWriter->GetDataType() == DataType::CIRCULAR_BUFFER_INT) {
            auto* buffer = bridgeWriter->getIntCircularBuffer();
            if(!buffer) {
                qDebug() << "external buffer is nullptr";
            }
            qDebug() << "external buffer is not nullptr";
        }


        // 设置主写入器类型
        outputBuffer->SetWriterType(Buffer::BUS_MASTER);

        // 创建总线连接对象
        OutPutBusConnection busConn;
        busConn.downstreamBlock = downstreamBlock;
        busConn.downstreamPortName = downstreamInputPort;
        busConn.bridgeWriter = bridgeWriter;
        busConn.connectedReader = inputReader;
        busConn.isDownstreamDone = false;
        busConn.PermitWrite = true;

        // 添加到主读取器
        outputBuffer->AddBusConnection(busConn);
        inputReader->connectToBuffer(bridgeWriter);
        m_OutPutbusConnectionCount++;
        qDebug() << "Created bridge writer: " << QString::fromStdString(bridgeName)
                 << ", connected to Reader: " << QString::fromStdString(inputReader->GetName())
                 << ", WriterType: " << QString::fromStdString(bridgeWriter->WriterTypeToString(bridgeWriter->GetWriterType()));

        qDebug() << "Main writer now has " << outputBuffer->GetBusConnectionCount()
                 << " bus connections";

    }
    //输出不是，输入也不是
    else if (!isInputBusType && !isOutputBusType) {
        // 非总线输入端：单一连接限制
        if (inputReader->IsConnected()) {
            qDebug() << "ERROR: Non-bus input port '" << QString::fromStdString(downstreamInputPort)
                     << "' already connected";
            return;
        }

        // 直接连接
        inputReader->connectToBuffer(outputBuffer);
    }
    //输出不是，输入是（非 bus-to-bus）
    else {
        // 创建唯一的桥接读取器名称
        std::string bridgeName = downstreamBlock->GetName() + "_bus_bridge_" +
                downstreamInputPort + "_" + std::to_string(m_busConnectionCount);

        // 总线连接

        BufferReader* bridgeReader = new BufferReader(bridgeName, inputReader->GetReadSize(), inputDataType);
        bridgeReader->SetReaderType(BufferReader::BUS_BRIDGE);

        // 建立连接 - 这会自动注册读取器
        bridgeReader->connectToBuffer(outputBuffer);

        qDebug() << "Created bridge reader: " << QString::fromStdString(bridgeName)
                 << ", connected to buffer: " << QString::fromStdString(outputBuffer->GetName())
                 << ", ReaderType: " << QString::fromStdString(bridgeReader->ReaderTypeToString(bridgeReader->GetReaderType()));

        // 设置主读取器类型
        inputReader->SetReaderType(BufferReader::BUS_MASTER);

        // 创建总线连接对象
        BusConnection busConn;
        busConn.upstreamBlock = upstreamBlock;
        busConn.upstreamPortName = upstreamOutputPort;
        busConn.bridgeReader = bridgeReader;
        busConn.connectedBuffer = outputBuffer;
        busConn.isUpstreamDone = false;

        // 添加到主读取器
        inputReader->AddBusConnection(busConn);
        m_busConnectionCount++;

        qDebug() << "Main reader now has " << inputReader->GetBusConnectionCount()
                 << " bus connections";
    }

    // 记录连接关系
    if (isInputBusType) {
        downstreamBlock->m_connectedBusUpstreamBlocks[downstreamInputPort].push_back(
        {upstreamBlock, upstreamOutputPort}
                    );
    }

    // 类型适配
    if (inputReader->GetDataType() == DataType::ANY) {
        qDebug() << "Downstream port uses ANY type, adapting to upstream type";
        inputReader->SetDataType(outputBuffer->GetDataType());
    }

    // 链路校验处理
    if(auto verificationSystem = GetVerificationSystem()) {
        //1.获取Block未知数变量
        auto upstreamVar = verificationSystem->registerBlock(upstreamBlock);
        auto downstreamVar = verificationSystem->registerBlock(downstreamBlock);

        //2.计算系数
        //上游系数 - 通常为正
        double upstreamCoeff = upstreamBlock->getUpCoefficient();
        //下游系数 - 通常为负
        double downstreamCoeff = downstreamBlock->getDownCoefficient();

        //3.从buffer获取实际参数
        size_t upstreamSize = 1;
        if (upstreamBlock->GetBlockType() != BlockType::SOURCE) {
            // 检查是否有输入端口
            if (upstreamBlock->GetInputPortCount() == 0) {
                qDebug() << "Block '" << QString::fromStdString(upstreamBlock->GetName())
                         << "' is not a SOURCE but has no input ports";
                return;
            }

            // 获取第一个端口的读取器
            BufferReader* firstReader = upstreamBlock->GetInputPort(upstreamBlock->GetInputPortName(0));
            if (!firstReader) {
                qDebug() << "Block '" << QString::fromStdString(upstreamBlock->GetName())
                         << "': first input reader is null";
                return;
            }

            upstreamSize = firstReader->GetReadSize();

            // 检查所有输入端口的读取大小是否一致
            for (size_t i = 0; i < upstreamBlock->GetInputPortCount(); i++) {
                std::string portName = upstreamBlock->GetInputPortName(i);
                BufferReader* reader = upstreamBlock->GetInputPort(portName);

                if (!reader) {
                    qDebug() << "Block '" << QString::fromStdString(upstreamBlock->GetName())
                             << "': input reader for port '" << QString::fromStdString(portName)
                             << "' is null";
                    return;
                }

                size_t upstreamSize_compare = reader->GetReadSize();

                // 当输入端的读指针大小不一致时，直接返回
                if (upstreamSize_compare != upstreamSize) {
                    qDebug() << "Block '" << QString::fromStdString(upstreamBlock->GetName())
                             << "': input readSize must be same (port "
                             << QString::fromStdString(portName) << " has size "
                             << upstreamSize_compare << ", expected " << upstreamSize << ")";
                    return;
                }
            }
        }
        size_t downstreamSize = inputReader->GetReadSize();

        if(upstreamSize > 0 && downstreamSize > 0) {
            //系数成比例
            upstreamCoeff *= static_cast<double>(upstreamSize);
            downstreamCoeff *= static_cast<double>(downstreamSize);
        }

        //4.添加校验方程: a*x_upstream + b*x_downstream = 0
        std::string VerificationDesc = "Connection: " + upstreamBlock->GetName() +
                "[" + upstreamOutputPort + "] -> " +
                downstreamBlock->GetName() +
                "[" + downstreamInputPort + "]";

        verificationSystem->addVerificationEquation(
                    upstreamBlock, downstreamBlock,
                    upstreamCoeff, downstreamCoeff,  // 注意这里下游系数取负！
                    0.0,
                    VerificationDesc);

        qDebug() << "Added constraint equation:";
        qDebug() << "  " << QString::fromStdString(VerificationDesc);
        std::stringstream eq_ss;
        eq_ss << std::fixed << std::setprecision(1)  // 设置小数点后1位
              << upstreamCoeff << "* x" << upstreamVar->variableId;

        if (downstreamCoeff >= 0) {
            eq_ss << " + " << downstreamCoeff;
        } else {
            eq_ss << " - " << std::abs(downstreamCoeff);
        }
        eq_ss << "* x" << downstreamVar->variableId << " = 0";

        qDebug() << "  Equation: " << QString::fromStdString(eq_ss.str());

    }

    if (outputBuffer && !isInputBusType && !isOutputBusType) {
        //链接完后再检查更新buffer大小，此前在connectToBuffer方法中已调用
//        outputBuffer->UpdateBufferSize();
        upstreamBlock->m_outputSizes[upstreamOutputPort] = outputBuffer->GetBufferSize();
    }

    qDebug() << "UpBlock '" << QString::fromStdString(upstreamBlock->GetName()) << "' and DownBlock '" << QString::fromStdString(downstreamBlock->GetName());
    qDebug() << "': " << ((isInputBusType || isOutputBusType) ? "Bus port" : "Standard port") << " connection completed!";
    qDebug() << "': end to connect!";
    qDebug() << "-----------";
}

void Block::SetVerificationSystem(std::shared_ptr<DataStreamVerification> system) {
    s_VerificationSystem = system;
}

std::shared_ptr<DataStreamVerification> Block::GetVerificationSystem() {
    return s_VerificationSystem;
}

void Block::SetName(const std::string &portName)
{
    m_name = portName;
}

bool Block::Setup()
{
    //检查Block是否是空闲状态
    if(m_state != BlockState::IDLE) {
        return false;
    }

    //设置Block的状态为开始状态
    m_state = BlockState::STARTED;
    return true;
}

bool Block::Stop()
{
    if(m_state == BlockState::STOPPED) {
        return false;
    }

    //禁用所有输出端口的自动触发
    for(const auto& outputPort : m_outputPorts) {
        Buffer* buffer = outputPort.second;
        if(buffer) {
            buffer->SetUpstreamDone(true);
            buffer->SetDownstreamDone(true);
        }
        if(buffer->IsBusType(buffer->GetDataType())) {
            for(auto& bus : buffer->GetBusConnections()) {
                bus.bridgeWriter->SetUpstreamDone(true);
                bus.bridgeWriter->SetDownstreamDone(true);
            }
        }
    }


    m_state = BlockState::STOPPED;
//    qDebug() << "Block '" << QString::fromStdString(m_name) << "': Stopped - exited data flow loop";
    return true;
}

bool Block::Done()
{
    if(m_state == BlockState::DONE) {
        return false;
    }

//    qDebug() << "Block '" << QString::fromStdString(m_name) << "': Starting Done() - processing remaining data...";

    // 保存当前的reader位置状态
    std::map<std::string, size_t> savedReaderPositions;
    for (const auto& inputPort : m_inputPorts) {
        BufferReader* reader = inputPort.second;
        if (reader && reader->GetConnectedBuffer()) {
            savedReaderPositions[inputPort.first] = reader->GetAvailableDataCount();
        }
    }


    //重置缓冲区
    for (const auto& outputPort : m_outputPorts) {
        Buffer* buffer = outputPort.second;
        if (buffer) {
            buffer->ResetBuffer();
            //buffer->ResetReaderPoint();
        }
    }

    // 关闭文件
    if(!m_outputFilename.empty() && m_outputFile.is_open()) {
        m_outputFile.close();
        qDebug() << "  - Closed output file: " << QString::fromStdString(m_outputFilename);
    }

    m_state = BlockState::DONE;
//    qDebug() << "Block '" << QString::fromStdString(m_name) << "': Marked as Done";
    return true;
}

bool Block::Flush()
{
    return true;
}

bool Block::IsCollectionComplete()
{
    return true;
}

bool Block::Run()
{
    qDebug() << "Block run enter";
    //Run函数为虚函数，信号源与处理器重写run函数完成内部数据处理功能
    if(m_state != BlockState::STARTED) {
        return false;
    }

    //终端块使用run函数将数据写入json文件
    if(m_blockType == BlockType::SINK) {
        for(const auto& inputPort : m_inputPorts) {
            const std::string& portName = inputPort.first;
            BufferReader* reader = inputPort.second;

            if (reader && reader->GetConnectedBuffer()) {
                size_t availableData = reader->GetAvailableDataCount();
                if (availableData > 0) {
                    ProcessAsTerminalBlock(portName);
                    return true;
                }
            }
        }
    }
    return true;
}

bool Block::Initialize(){ return true; }

bool Block::CanProcess()
{

    //检查Block的状态是否异常
    if(m_state == BlockState::STOPPED || m_state == BlockState::DONE) {
        qDebug() << "block state error";
		return false;
    }

    //检查是否连接成功
    bool upstreamDone = true;
    bool hasUpstreamData = false;

    for (const auto& inputPort : m_inputPorts) {
        BufferReader* reader = inputPort.second;
        if (reader && reader->IsConnected()) {
            // 检查上游是否完成
            if (!reader->IsUpstreamDone()) {
                upstreamDone = false;
            }

            // 检查是否有数据可读
            if (reader->HasDataAvailable()) {
                hasUpstreamData = true;
            }
        }
    }

    // 如果是源块，不需要上游数据
    if (m_blockType == BlockType::SOURCE) {
        // 源块：检查下游是否完成
        if (IsDownstreamDone()) {
            qDebug() << "Block '" << QString::fromStdString(m_name) << "': Downstream done, source cannot process";
            return false;
        }

        // 检查输出端口是否有空间
        return AreOutputPortsReady();
    }
    // 如果是终端块
    else if (m_blockType == BlockType::SINK) {
        // 终端块：检查上游是否完成或有数据
        return hasUpstreamData;
    }
    // 处理器块
    else {
        // 如果上游已完成且无数据，则不能处理
        if (upstreamDone && !hasUpstreamData) {
            LOG_ERROR("processor failed. need source");
            return false;
        }
//        qDebug() << "hasUpstreamData: " << hasUpstreamData;

        // 正常处理：需要上游数据且下游有空间
        return hasUpstreamData && AreOutputPortsReady();
    }
}

bool Block::AreOutputPortsReady()
{
    //检查所有输出端是否准备就绪
//    qDebug() << "===AreOutputPortsReady enter===";
    if(m_outputPorts.empty()) {
        return true;
    }
    for(const auto& outputPort : m_outputPorts) {
        Buffer* outputBuffer = outputPort.second;
        //outputBuffer不存在或者没有读指针，不进行检查
        if(!outputBuffer || outputBuffer->GetReaderCount() == 0) {
            continue;
        }
        //计算该输出端口期望的输出数据量
        // 捕获访问非法Buffer的异常
        try {
            size_t bufferSize = outputBuffer->GetBufferSize();
//            qDebug() << "bufferSize :" << bufferSize;

            //若为1024，则为默认大小
            if(bufferSize == 1024) {
                continue;
            }

//            qDebug() << "Buffer '" << QString::fromStdString(outputBuffer->GetName()) << "': ";
//            qDebug() << "expectedOutputSize :" << bufferSize;

            //检查输出端buffer剩余空间
            size_t freeSpace = outputBuffer->GetBufferFreeSpace();
//            qDebug() << "freeSpace: " << freeSpace;

            if(freeSpace <= 0) {
//                qDebug() << "current OutputPort: " << QString::fromStdString(outputBuffer->GetName())
//                         <<"expectedOutputSize :" << bufferSize<< ", freeSpace: " << freeSpace << "return false";
                return false;
            }
        } catch (...) {
//            LOG_WARN("Invalid Buffer for port: ",outputPort.first,", skip");
            continue;
        }
    }
    //qDebug() << "===AreOutputPortsReady end===";
    return true;

}

Block::DataType Block::BusToCircularBuffer(Block::DataType type)
{
    if(type == DataType::INT_BUS) return DataType::CIRCULAR_BUFFER_INT;
    else if(type == DataType::DOUBLE_BUS) return DataType::CIRCULAR_BUFFER_DOUBLE;
    else if(type == DataType::FLOAT_BUS) return DataType::CIRCULAR_BUFFER_FLOAT;
    else if(type == DataType::BOOL_BUS) return DataType::CIRCULAR_BUFFER_BOOL;
    else if(type == DataType::FCOMPLEX_BUS) return DataType::CIRCULAR_BUFFER_FCOMPLEX;
    else if(type == DataType::DCOMPLEX_BUS) return DataType::CIRCULAR_BUFFER_DCOMPLEX;
    else if(type == DataType::ENVELOPE_BUS) return DataType::ENVELOPE_SIGNAL;
    else if(type == DataType::MATRIX_INT_BUS) return DataType::MATRIX_INT;
    else if(type == DataType::MATRIX_DOUBLE_BUS) return DataType::MATRIX_DOUBLE;
    else if(type == DataType::MATRIX_FLOAT_BUS) return DataType::MATRIX_FLOAT;
    else if(type == DataType::MATRIX_BOOL_BUS) return DataType::MATRIX_BOOL;
    else if(type == DataType::MATRIX_FCOMPLEX_BUS) return DataType::MATRIX_FCOMPLEX;
    else if(type == DataType::MATRIX_DCOMPLEX_BUS) return DataType::MATRIX_DCOMPLEX;
    else if(type == DataType::MATRIX_ENVELOPE_BUS) return DataType::MATRIX_ENVELOPE;
    return DataType::CIRCULAR_BUFFER_INT;
}

void Block::ResolveAllDeferredBusConnections()
{
    if (s_deferredBusConnections.empty()) return;

    qDebug() << "=== 解析延迟 Bus-to-Bus 连接，共" << s_deferredBusConnections.size() << "个 ===";

    for (const auto& deferred : s_deferredBusConnections) {
        Block* upstreamBlock = deferred.upstreamBlock;
        Block* downstreamBlock = deferred.downstreamBlock;
        const std::string& upstreamOutputPort = deferred.upstreamOutputPort;
        const std::string& downstreamInputPort = deferred.downstreamInputPort;
        DataType outputDataType = deferred.outputDataType;

        Buffer* outputBuffer = upstreamBlock->GetOutputPort(upstreamOutputPort);
        BufferReader* inputReader = downstreamBlock->GetInputPort(downstreamInputPort);

        if (!outputBuffer || !inputReader) {
            LOG_ERROR("延迟连接解析失败: 端口为空");
            continue;
        }

        // 尝试从两端的已有 bus 连接数推导通道数
        size_t outConnCount = outputBuffer->GetBusConnectionCount();
        size_t inConnCount = inputReader->GetBusConnectionCount();
        int channelCount = static_cast<int>(std::max(outConnCount, inConnCount));

        // 再次尝试从模型的 GetBusChannelCount 获取（可能已被模型更新）
        int upCh = upstreamBlock->GetBusChannelCount();
        int downCh = downstreamBlock->GetBusChannelCount();
        if (upCh > 0) channelCount = upCh;
        else if (downCh > 0) channelCount = downCh;

        // 最终 fallback
        if (channelCount <= 0) channelCount = 1;

        qDebug() << "延迟连接解析:" << QString::fromStdString(upstreamBlock->GetName())
                 << "->" << QString::fromStdString(downstreamBlock->GetName())
                 << "通道数:" << channelCount
                 << "(输出侧已有连接:" << outConnCount
                 << "输入侧已有连接:" << inConnCount << ")";

        // 清理可能存在的旧连接
        outputBuffer->ClearBusConnections();
        inputReader->ClearBusConnections();

        // 创建 bridge
        DataType bridgeDataType = BusToCircularBuffer(outputDataType);
        for (int ch = 0; ch < channelCount; ++ch) {
            std::string wName = upstreamBlock->GetName() + "_bus2bus_w_" +
                                upstreamOutputPort + "_" + std::to_string(ch);
            Buffer* bridgeWriter = new Buffer(wName, outputBuffer->GetWriteSize(), bridgeDataType);
            bridgeWriter->SetWriterType(Buffer::BUS_BRIDGE);

            CircularBufferBase* channelBuffer =
                    Buffer::CreateCircularBufferByDataType(bridgeDataType);
            if (!channelBuffer) {
                qDebug() << "ERROR: Failed to create CircularBuffer for deferred bus2bus channel" << ch;
                delete bridgeWriter;
                continue;
            }
            bridgeWriter->SetExternalCircularBuffer(channelBuffer);

            std::string rName = downstreamBlock->GetName() + "_bus2bus_r_" +
                                downstreamInputPort + "_" + std::to_string(ch);
            BufferReader* bridgeReader = new BufferReader(rName, inputReader->GetReadSize(), bridgeDataType);
            bridgeReader->SetReaderType(BufferReader::BUS_BRIDGE);
            bridgeReader->connectToBuffer(bridgeWriter);

            OutPutBusConnection outBusConn;
            outBusConn.downstreamBlock = downstreamBlock;
            outBusConn.downstreamPortName = downstreamInputPort;
            outBusConn.bridgeWriter = bridgeWriter;
            outBusConn.connectedReader = bridgeReader;
            outBusConn.isDownstreamDone = false;
            outBusConn.PermitWrite = true;
            outputBuffer->AddBusConnection(outBusConn);

            BusConnection inBusConn;
            inBusConn.upstreamBlock = upstreamBlock;
            inBusConn.upstreamPortName = upstreamOutputPort;
            inBusConn.bridgeReader = bridgeReader;
            inBusConn.connectedBuffer = bridgeWriter;
            inBusConn.isUpstreamDone = false;
            inputReader->AddBusConnection(inBusConn);
        }

        outputBuffer->SetWriterType(Buffer::BUS_MASTER);
        inputReader->SetReaderType(BufferReader::BUS_MASTER);

        qDebug() << "延迟连接解析完成，输出侧" << outputBuffer->GetBusConnectionCount()
                 << "个连接，输入侧" << inputReader->GetBusConnectionCount() << "个连接";
    }

    s_deferredBusConnections.clear();
}

Block::BlockState Block::GetState() const
{
    return m_state;
}

void Block::SetOutputFile(const std::string &filename)
{
    //设置终端块输出文件名
    m_outputFilename = filename;
}

std::string Block::GetOutputFile() const
{
    return m_outputFilename;
}

bool Block::ProcessAsTerminalBlock(const std::string &inputPortName)
{
    //终端块处理数据方法
    return m_sinker->ProcessAsTerminalBlock(inputPortName);
}

bool Block::IsTerminalBlock() const
{
    //判断是否为终端块
    return m_sinker->IsTerminalBlock();
}

const std::string &Block::GetInputPortName(size_t index) const
{
    //获取第index + 1 个的输入端名称
    static const std::string emptyString = "";
    if (index < m_inputPortNames.size()) {
        return m_inputPortNames[index];
    }
    qDebug() << "WARNING: Input port index " << index << " out of range";
    return emptyString;
}

const std::string &Block::GetOutputPortName(size_t index) const
{
    //获取第index + 1 个的输出端名称
    return m_outputPortNames[index];;
}

size_t Block::GetInputPortIndex(const std::string &portName) const
{
    //获取对应名称的输入端索引
    auto it = m_inputPortNameToIndex.find(portName);
    if (it != m_inputPortNameToIndex.end()) {
        return it->second;
    }
    qDebug() << "WARNING: Input port '" << QString::fromStdString(portName) << "' not found";
    return SIZE_MAX;
}

size_t Block::GetOutputPortIndex(const std::string &portName) const
{
    //获取对应名称的输出端索引
    auto it = m_outputPortNameToIndex.find(portName);
    if (it != m_outputPortNameToIndex.end()) {
        return it->second;
    }
    qDebug() << "WARNING: Output port '" << QString::fromStdString(portName) << "' not found";
    return SIZE_MAX;
}

const std::vector<std::string> &Block::GetAllInputPortNames() const
{
    return m_inputPortNames;
}

const std::vector<std::string> &Block::GetAllOutputPortNames() const
{
    return m_outputPortNames;
}

bool Block::IsDownstreamDone() {
    for (const auto& outputport : m_outputPorts) {
        Buffer* buffer = outputport.second;
        if (buffer) {

            if (!buffer->IsDownstreamDone()) {
                return false;
            }
        }
    }
    // 如果没有输出端口，认为下游已完成
    if (m_outputPorts.empty()) {
        qDebug() << "  - No output ports, downstream considered done";
        return true;
    }

    qDebug() << "Block '" << QString::fromStdString(m_name) << "': All downstream paths are done";
    return true;
}

void Block::SetTerminalMode(Block::TerminalMode mode)
{
    //设置终端块的写入模式
    m_terminalMode = mode;
}

void Block::SetTimeRange(double start, double stop)
{
    //设置终端块的时间模式的起始时间与终止时间
    m_timeStart = start; m_timeStop = stop;
}

void Block::SetSampleRange(size_t start, size_t stop)
{
    //设置终端块的采样模式的起始采样点与终止采样点
    m_sampleStart = start; m_sampleStop = stop;
}

int Block::GetRequiredInputCount(const std::string &portName) const {
    (void)portName;  // 避免未使用参数警告
    return 1;
}

int Block::GetMaxRequiredInputCount() const {
    int maxRequired = 0;
    for (size_t i = 0; i < GetInputPortCount(); i++) {
        int required = GetRequiredInputCount(GetInputPortName(i));
        if (required > maxRequired) maxRequired = required;
    }
    return maxRequired == 0 ? 1 : maxRequired;
}

int Block::GetBatchSize() const {
    return 1;
}

int Block::RunBatch(int maxCount) {
    int processed = 0;
    while (processed < maxCount && Run()) {
        processed++;
    }
    return processed;
}

bool Block::IsBackpressured() const { return m_isBackpressured; }

void Block::SetBackpressured(bool backpressured) { m_isBackpressured = backpressured; }

bool Block::IsZeroCrossTriggered() const { return m_isZeroCrossTriggered; }

void Block::SetZeroCrossTriggered(bool triggered) { m_isZeroCrossTriggered = triggered; }

float Block::GetDownstreamBufferUsage() const
{
    float maxUsage = 0.0f;
    for (const auto& port : m_outputPorts) {
        Buffer* buffer = port.second;
        if (buffer && buffer->GetReaderCount() > 0) {
            float usage = buffer->GetUsage();
            if (usage > maxUsage) {
                maxUsage = usage;
            }
        }
    }
    return maxUsage;
}

int Block::GetRecommendedBatchSize() const
{
    float usage = GetDownstreamBufferUsage();
    int originalBatch = GetBatchSize();

    if (usage > 80.0f) {
        return 1;
    } else if (usage > 60.0f) {
        return std::max(1, originalBatch / 4);
    } else if (usage > 40.0f) {
        return std::max(1, originalBatch / 2);
    } else if (usage < 20.0f && originalBatch > 1) {
        return std::min(originalBatch, originalBatch * 2);
    }
    return originalBatch;
}

void Block::SetIsBitShiftRegister(bool IsBitShiftRegister)
{
    m_IsBitShiftRegister = IsBitShiftRegister;
}

bool Block::IsBitShiftRegister() const
{
    return m_IsBitShiftRegister;
}

void Block::SetBitShiftRegisterNumBits(int NumBits)
{
    m_NumBits = NumBits;
}

int Block::GetBitShiftRegisterNumBits() const
{
    return m_NumBits;
}

QMap<int, PortMsg> Block::getPortsMsg() const
{
    return m_PortMessage;
}

void Block::setPortsMsg(const QMap<int, PortMsg> &value)
{
    m_PortMessage = value;
}

bool Block::IsDone() const
{
    return m_isDone;
}

void Block::SetDone(bool done)
{
    m_isDone = done;
}

void Block::AddBusConnection(const std::string &inputPortName, const BusConnection &connection)
{
    //添加bus连接
    qDebug() << "=== Block::AddBusConnection ===";
    qDebug() << "Port: " << QString::fromStdString(inputPortName);
    qDebug() << "Connection: " << QString::fromStdString(connection.getInfo());

    m_busConnections[inputPortName].push_back(connection);

    qDebug() << "Bus connection added. Total connections for port '" << QString::fromStdString(inputPortName)
              << "': " << m_busConnections[inputPortName].size();
}

const std::vector<BusConnection> &Block::GetBusConnections(const std::string &inputPortName) const
{
    //获取bus连接容器
    static std::vector<BusConnection> emptyConnections;
    auto it = m_busConnections.find(inputPortName);
    return (it != m_busConnections.end()) ? it->second : emptyConnections;
}

const std::vector<BusConnection> &Block::GetBusConnectionsForReader(const std::string &readerName) const
{
    // 直接通过端口名查找，不通过reader对象
    auto it = m_busConnections.find(readerName);
    if (it != m_busConnections.end()) {
        return it->second;
    }

    // 如果找不到，尝试通过端口映射查找
    for (const auto& pair : m_busConnections) {
        const std::string& portName = pair.first;
        // 假设readerName就是端口名，或者有某种映射关系
        if (portName == readerName) {
            return pair.second;
        }
    }

    static std::vector<BusConnection> empty;
    qDebug() << "WARNING: No bus connections found for reader: " << QString::fromStdString(readerName);
    return empty;
}

size_t Block::GetBusConnectionCountForReader(const std::string &portName) const
{
    //获取具体输入端所连接的bus数量
    return GetBusConnectionsForReader(portName).size();
}

size_t Block::GetBusConnectionCount(const std::string &inputPortName) const
{
    //获取所有bus的连接数量
    auto it = m_busConnections.find(inputPortName);
    if (it == m_busConnections.end()) {
        return 0;
    }
    return it->second.size();
}
