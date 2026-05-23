#include "BufferReader.h"
#include "Block.h"
#include "BufferReaderDataReadImpl.h"

using namespace SystemVueModelBuilder;

BufferReader::BufferReader(const std::string& name, size_t readSize, DataType type)
    : m_name(name),m_readSize(readSize), m_dataType(type), m_connectedBuffer(nullptr)
{
    //实现类指针初始化
    m_datareader = std::make_unique<BufferReaderDataReadImpl>(this);
    m_readerType = STANDARD;
    m_dfinterface.SetModelName(name.c_str());
    //数据类型容器初始化
    createDataVariant(type);
    //临时缓冲区初始化
    InitializeTempBuffer(readSize, m_dataType);

}

BufferReader::~BufferReader()
{
    m_connectedBuffer = nullptr;
    qDebug() << "=== BufferReader '" << QString::fromStdString(m_name) << "' destructor ===" ;
}

BufferReader::DataType BufferReader::GetDataType() const
{
    return m_dataType;
}

void BufferReader::SetDataType(DataType type)
{
    m_dataType = type;
}

void BufferReader::connectToBuffer(Buffer *buffer)
{
    if (!buffer) {
        qDebug() << "ERROR: Cannot connect to NULL buffer";
        return;
    }

    m_connectedBuffer = buffer;
    qDebug() << "BufferReader '" << QString::fromStdString(m_name) << "' connected to buffer '"
              << QString::fromStdString(buffer->GetName()) << "'";
    qDebug() << "Reader type: " << QString::fromStdString(ReaderTypeToString(m_readerType));

    // 根据读取器类型决定如何注册
    if (m_readerType == BUS_BRIDGE || m_readerType == STANDARD) {
//        qDebug() << "Registering reader to buffer...";

        // 方法1：使用读取器名称
        size_t index = buffer->AddReader(m_readSize, m_name);

        if (index != SIZE_MAX) {
            // 关键：注册读取器对象
            buffer->RegisterReader(m_name, this);
//            qDebug() << "Reader object registered successfully";
        } else {
            qDebug() << "WARNING: Failed to add reader to buffer";
        }
    } else if (m_readerType == BUS_MASTER) {
        qDebug() << "Bus master reader - not registering directly to buffer";
    }
}

void BufferReader::Disconnect()
{
    //断开连接
    if (m_connectedBuffer) {
        m_connectedBuffer->UnRegisterReader(m_name);
        qDebug() << "BufferReader '" << QString::fromStdString(m_name) << "' disconnected from buffer";
        m_connectedBuffer = nullptr;
    }
}

bool BufferReader::ReadData(int &outputData)
{
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(double &outputData)
{
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(float &outputData)
{
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(bool &outputData)
{
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::complex<float> &outputData)
{
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::complex<double> &outputData)
{
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<int> &outputData)
{
    //读取方法
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<double> &outputData)
{
    //读取方法
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<float> &outputData)
{
    //读取方法
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<bool> &outputData)
{
    //读取方法
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<std::complex<float> > &outputData)
{
    //读取方法
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<std::complex<double> > &outputData)
{
    //读取方法
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<int *> &outputData)
{
    //读取方法
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<double *> &outputData)
{
    //读取方法
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<std::complex<double> *> &outputData)
{
    //读取方法
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(SystemVueModelBuilder::CircularBufferBase &outputData)
{
    //读取方法
    return m_datareader->ReadData(outputData);
}



bool BufferReader::ReadData(std::vector<EnvelopeSignal> &outputData)
{
    //读取方法
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(EnvelopeCircularBuffer &outputData)
{
    //读取方法
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<IntMatrix> &outputData)
{
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<DoubleMatrix> &outputData)
{
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<FloatMatrix> &outputData)
{
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<BoolMatrix> &outputData)
{
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<FComplexMatrix> &outputData)
{
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<DComplexMatrix> &outputData)
{
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadData(std::vector<EnvelopeMatrix> &outputData)
{
    return m_datareader->ReadData(outputData);
}

bool BufferReader::ReadBusData(size_t readSize, std::vector<int> &outputData)
{
    //读取方法
    return m_datareader->ReadBusData(readSize, outputData);
}

bool BufferReader::ReadBusData(size_t readSize, std::vector<double> &outputData)
{
    //读取方法
    return m_datareader->ReadBusData(readSize, outputData);
}

bool BufferReader::ReadBusData(size_t readSize, std::vector<float> &outputData)
{
    //读取方法
    return m_datareader->ReadBusData(readSize, outputData);
}

bool BufferReader::ReadBusData(size_t readSize, std::vector<bool> &outputData)
{
    //读取方法
    return m_datareader->ReadBusData(readSize, outputData);
}

bool BufferReader::ReadBusData(size_t readSize, std::vector<std::complex<double>>& outputData)
{
    //读取方法
    return m_datareader->ReadBusData(readSize, outputData);
}

bool BufferReader::ReadBusData(size_t readSize, std::vector<std::complex<float> > &outputData)
{
    //读取方法
    return m_datareader->ReadBusData(readSize, outputData);
}

bool BufferReader::ReadBusData(size_t readSize, std::vector<EnvelopeSignal> &outputData)
{
    //读取方法
    return m_datareader->ReadBusData(readSize, outputData);
}

bool BufferReader::ReadBusData(size_t readSize, EnvelopeCircularBuffer &outputData)
{
    //读取方法
    return m_datareader->ReadBusData(readSize, outputData);
}

bool BufferReader::ReadBusData(size_t readSize, std::vector<IntMatrix> &outputData)
{
    return m_datareader->ReadBusData(readSize, outputData);
}

bool BufferReader::ReadBusData(size_t readSize, std::vector<DoubleMatrix> &outputData)
{
    return m_datareader->ReadBusData(readSize, outputData);
}

bool BufferReader::ReadBusData(size_t readSize, std::vector<FloatMatrix> &outputData)
{
    return m_datareader->ReadBusData(readSize, outputData);
}

bool BufferReader::ReadBusData(size_t readSize, std::vector<BoolMatrix> &outputData)
{
    return m_datareader->ReadBusData(readSize, outputData);
}

bool BufferReader::ReadBusData(size_t readSize, std::vector<FComplexMatrix> &outputData)
{
    return m_datareader->ReadBusData(readSize, outputData);
}

bool BufferReader::ReadBusData(size_t readSize, std::vector<DComplexMatrix> &outputData)
{
    return m_datareader->ReadBusData(readSize, outputData);
}

bool BufferReader::ReadBusData(size_t readSize, std::vector<EnvelopeMatrix> &outputData)
{
    return m_datareader->ReadBusData(readSize, outputData);
}

void BufferReader::SetName(const std::string &name)
{
    m_name = name;
    m_dfinterface.SetModelName(name.c_str());
}

void BufferReader::SetReadSize(size_t readSize)
{
    //设置读指针大小
    if (m_readSize != readSize) {
        m_readSize = readSize;
    }
}

void BufferReader::setCharacterizationFrequency(double fc) {
    //设置表征频率
    m_fc = fc;
    double EPSILON = 1e-10;
    m_hasCharacterizationFrequency = (fabs(fc) > EPSILON);
//    qDebug() << "m_hasCharacterizationFrequency: " << m_hasCharacterizationFrequency;
//    qDebug() << "BufferReader '" << QString::fromStdString(m_name) << "': Set characterization frequency to " << m_fc;
}

double BufferReader::getCharacterizationFrequency() const {
    return m_fc;
}

bool BufferReader::hasCharacterizationFrequency() const {
    return m_hasCharacterizationFrequency;
}

bool BufferReader::propagateCharacterizationFrequencyFromBuffer() {
    //通过buffer传递表征频率
    if (m_connectedBuffer && m_connectedBuffer->hasCharacterizationFrequency()) {
        double fc = m_connectedBuffer->getCharacterizationFrequency();
        setCharacterizationFrequency(fc);
        return true;
    }
    return false;
}

const std::string &BufferReader::GetName() const
{
    return m_name;
}

size_t BufferReader::GetReadSize() const
{
    return m_readSize;
}

Buffer *BufferReader::GetConnectedBuffer() const
{
    return m_connectedBuffer;
}

size_t BufferReader::GetAvailableDataCount() const
{
    //获取读指针所连接的可读大小
    // 如果是总线类型，需要特殊处理
    if (BufferReader::IsBusType(m_dataType)) {
        return GetBusAvailableDataCount();
    }
    if (!m_connectedBuffer) {
        return 0;
    }
    size_t available = m_connectedBuffer->GetAvailableDataForReader(m_name);
    return available;
}

bool BufferReader::IsBusType(BufferReader::DataType type)
{
    //判断是否是bus类型
    return type == DataType::INT_BUS ||
            type == DataType::DOUBLE_BUS ||
            type == DataType::FLOAT_BUS ||
            type == DataType::BOOL_BUS ||
            type == DataType::DCOMPLEX_BUS ||
            type == DataType::FCOMPLEX_BUS ||
            type == DataType::ENVELOPE_BUS ||
            type == DataType::MATRIX_ENVELOPE_BUS ||
            type == DataType::MATRIX_INT_BUS ||
            type == DataType::MATRIX_DOUBLE_BUS ||
            type == DataType::MATRIX_FLOAT_BUS ||
            type == DataType::MATRIX_BOOL_BUS ||
            type == DataType::MATRIX_DCOMPLEX_BUS ||
            type == DataType::MATRIX_FCOMPLEX_BUS;
}

bool BufferReader::IsBusReader() const
{
    // 首先检查读取器类型
    if (m_readerType == BUS_MASTER || m_readerType == BUS_BRIDGE) {
        return true;
    }

    // 然后检查数据类型
    return IsBusType(m_dataType);
}

bool BufferReader::IsBusUpstreamDone() const
{
//    const auto& busConnections = GetBusConnections();
    if (m_busConnections.empty()) {
        return true; // 没有连接，认为上游已完成
    }

    // 检查所有总线连接的上游状态
    for (const auto& conn : m_busConnections) {
        if (conn.connectedBuffer && !conn.connectedBuffer->IsUpstreamDone()) {
            return false; // 有上游未完成
        }
    }

    return true; // 所有上游都完成
}

bool BufferReader::IsBusDownstreamDone() const
{
//    const auto& busConnections = GetBusConnections();
    if (m_busConnections.empty()) {
        qDebug() << QString::fromStdString(GetName()) << "busConnection is empty";
        return true;
    }

    // 检查所有总线连接的下游状态
    for (const auto& conn : m_busConnections) {
        if (conn.connectedBuffer && !conn.connectedBuffer->IsDownstreamDone()) {
            return false; // 有下游未完成
        }
    }

    return true; // 所有下游都完成
}

const std::vector<BusConnection> &BufferReader::GetBusConnections() const
{
    return m_busConnections;
}

size_t BufferReader::GetBusConnectionCount() const
{
    return m_busConnections.size();
}

void BufferReader::AddBusConnection(const BusConnection &connection)
{
    m_busConnections.push_back(connection);

    // 如果是第一次添加总线连接，标记为总线类型
    if (m_busConnections.size() == 1) {
        m_isBusType = true;
        qDebug() << "BufferReader '" << QString::fromStdString(m_name) << "': Marked as bus type";
    }
}

void BufferReader::ClearBusConnections()
{
    //清空bus
    m_busConnections.clear();
    m_isBusType = false;
}

BufferReader::ReaderType BufferReader::GetReaderType() const { return m_readerType; }

void BufferReader::SetReaderType(BufferReader::ReaderType type) { m_readerType = type; }

std::string BufferReader::ReaderTypeToString(BufferReader::ReaderType type)
{
    //读指针类型枚举类转string
    switch (type) {
    case STANDARD: return "STANDARD";
    case BUS_MASTER: return "BUS_MASTER";
    case BUS_BRIDGE: return "BUS_BRIDGE";
    default: return "UNKNOWN";
    }
}

void BufferReader::OnBufferReallocated()
{
    qDebug()<< "BufferReader '" << QString::fromStdString(m_name) << "': Buffer reallocated, updating internal references";

    // 清理临时缓冲区
    // 保留读取器位置
    ClearTempBuffer();
    m_tempDataCount = 0;
    m_tempReadSize = 0;

}

void BufferReader::ReconnectToBuffer(Buffer *buffer)
{
    qDebug() << "=== BufferReader::ReconnectToBuffer BEGIN ===";
    qDebug() << "Reader: '" << QString::fromStdString(m_name) << "' (" << this << ")";
    qDebug() << "Current buffer: " << m_connectedBuffer;

    //重新连接buffer，用于重新分配过内存后
    if (m_connectedBuffer) {
        qDebug() << " ('" << QString::fromStdString(m_connectedBuffer->GetName()) << "' at " << m_connectedBuffer << ")";
    }
    qDebug() << "New buffer: " << buffer;
    if (buffer) {
        qDebug() << " ('" << QString::fromStdString(buffer->GetName()) << "' at " << buffer << ")";
    }

    if (!buffer) {
        qDebug() << "ERROR: Cannot reconnect to null buffer";
        qDebug() << "=== ReconnectToBuffer END (FAILED) ===";
        return;
    }

    // 验证这是否是同一个Buffer对象（地址相同）
    if (m_connectedBuffer == buffer) {
        qDebug() << "WARNING: Already connected to this buffer";
        qDebug() << "=== ReconnectToBuffer END (NO CHANGE) ===";
        return;
    }

    // 保存旧的缓冲区引用
//    Buffer* oldBuffer = m_connectedBuffer;

    // 更新连接到新缓冲区
    m_connectedBuffer = buffer;

    // 清理临时缓冲区
    ClearTempBuffer();
    m_tempDataCount = 0;
    m_tempReadSize = 0;

    // 确保读取器在新缓冲区中正确注册
    EnsureProperRegistration(buffer);

    qDebug() << "Reconnection successful";
    qDebug() << "=== ReconnectToBuffer END (SUCCESS) ===";

}

void BufferReader::SetTempBuffer(size_t size, BufferReader::DataType dataType)
{
    //设置临时缓冲区
    m_tempBufferSize = size;
    m_tempDataType = dataType;
    size_t elementSize = GetDataTypeSize(dataType);
    m_tempBuffer.resize(size * elementSize);
    m_tempDataCount = 0;

    qDebug() << "Set temp buffer: size=" << size
              << ", type=" << static_cast<int>(dataType)
              << ", elementSize=" << elementSize;
}

bool BufferReader::ReadToTempBuffer()
{
    //读取数据存到临时缓冲区
    if (!m_connectedBuffer) {
        qDebug() << "ERROR: No connected buffer for temp buffer read";
        return false;
    }

    if (m_tempBuffer.empty()) {
        qDebug() << "ERROR: Temp buffer not initialized";
        return false;
    }

    size_t available = GetAvailableDataCount();
    if (available == 0) {
        qDebug() << "No data available for temp buffer";
        return false;
    }

    // 计算可以读取的数据量
    size_t readSize = std::min(available, m_tempBufferSize - m_tempDataCount);
    if (readSize == 0) {
        qDebug() << "Temp buffer is full";
        return false;
    }

    qDebug() << "Reading " << readSize << " samples to temp buffer";

    // 根据数据类型读取数据
    bool success = ReadDataToTempBuffer(readSize);
    if (success) {
//        qDebug() << "Successfully read " << readSize << " samples to temp buffer";
//        qDebug() << "Temp buffer now has " << m_tempDataCount << " samples";
    }

    return success;
}

size_t BufferReader::GetTempBufferDataCount() const { return m_tempDataCount; }

void BufferReader::ClearTempBuffer()
{
    m_tempDataCount = 0;
    qDebug() << "Temp buffer cleared";
}

void BufferReader::InitializeTempBuffer(size_t bufferSize, BufferReader::DataType dataType)
{
    //初始化临时缓冲区
    m_tempBufferSize = bufferSize;
    m_tempDataType = dataType;
    m_tempDataCount = 0;
    m_tempReadSize = 0;

    size_t elementSize = GetDataTypeSize(dataType);
    m_tempBuffer.resize(bufferSize * elementSize);

//    qDebug() << "BufferReader '" << QString::fromStdString(m_name) << "' temp buffer initialized: "
//              << "size=" << bufferSize << ", type=" << static_cast<int>(dataType)
//              << ", elementSize=" << elementSize;
}

bool BufferReader::ReadDataToTempBuffer(size_t readSize)
{
    size_t elementSize = GetDataTypeSize(m_tempDataType);
    size_t offset = m_tempDataCount * elementSize;

    // 从连接缓冲区读取数据到临时缓冲区
    switch (m_tempDataType) {
    case DataType::DOUBLE:
    case DataType::CIRCULAR_BUFFER_DOUBLE:
        return ReadTypedDataToTempBuffer<double>(readSize, offset);
    case DataType::FLOAT:
    case DataType::CIRCULAR_BUFFER_FLOAT:
        return ReadTypedDataToTempBuffer<float>(readSize, offset);
    case DataType::INT:
    case DataType::CIRCULAR_BUFFER_INT:
        return ReadTypedDataToTempBuffer<int>(readSize, offset);
    case DataType::COMPLEX_DOUBLE:
    case DataType::CIRCULAR_BUFFER_DCOMPLEX:
        return ReadTypedDataToTempBuffer<std::complex<double>>(readSize, offset);
    case DataType::COMPLEX_FLOAT:
    case DataType::CIRCULAR_BUFFER_FCOMPLEX:
        return ReadTypedDataToTempBuffer<std::complex<float>>(readSize, offset);
    case DataType::ENVELOPE_SIGNAL:
        return ReadTypedDataToTempBuffer<SystemVueModelBuilder::EnvelopeSignal>(readSize, offset);
    default:
        qDebug() << "ERROR: Unsupported data type for temp buffer";
        return false;
    }
}

size_t BufferReader::GetDataTypeSize(BufferReader::DataType type) const
{
    return DataTypesAndParsers::GetDataTypeSize(type);
}

bool BufferReader::HasDataAvailable()
{
//    qDebug() << "=== BufferReader::HasDataAvailable ===";
//    qDebug() << "Reader: " << QString::fromStdString(m_name);

    // 桥接读取器使用标准检查
    if (m_readerType == BUS_BRIDGE || m_readerType == STANDARD) {
        if (!m_connectedBuffer) {
            qDebug() << "Not connected to buffer";
            return false;
        }

        size_t available = m_connectedBuffer->GetAvailableDataForReader(m_name);
//        qDebug() << "Available data: " << available
//                  << ", required: " << m_readSize;

        return available >= m_readSize;
    }
    else if (m_readerType == BUS_MASTER) {
//        qDebug() << "Master bus reader checking all connections";

        if (m_busConnections.empty()) {
            LOG_ERROR("No bus connections found for reader: ", m_name);
            return false;
        }

        // 检查每个连接是否有数据
        for (size_t i = 0; i < m_busConnections.size(); ++i) {
            const auto& connection = m_busConnections[i];
//            qDebug() << "Checking connection " << i;

            if (!connection.bridgeReader || !connection.connectedBuffer) {
                qDebug() << "WARNING: Connection " << i << " is invalid";
                continue;
            }

            // 关键：桥接读取器应该用标准方式检查
            size_t available = connection.connectedBuffer->GetAvailableDataForReader(
                connection.bridgeReader->GetName());

//            qDebug() << "Available data: " << available
//                      << ", required: " << connection.bridgeReader->GetReadSize();

            if (available < connection.bridgeReader->GetReadSize()) {
//                qDebug() << "Connection " << i << " does not have enough data";
                return false;
            }
        }

//        qDebug() << "All bus connections have data available";
        return true;
    }

    return false;
}

bool BufferReader::IsConnected() const
{
    // 如果是总线类型，检查是否有任何总线连接
    if (m_isBusType) {
        return !m_busConnections.empty();
    }
    // 非总线类型检查普通连接
    return m_connectedBuffer != nullptr;
}

bool BufferReader::HasValidConnection() const {
//    if (IsBusType(m_dataType)) {
//        const auto& busConnections = GetBusConnections();
//        return !busConnections.empty();  // 总线类型：检查是否有总线连接
//    } else {
//        return IsConnected();      // 非总线类型：检查是否连接了缓冲区
//    }
    if (m_isBusType) {
        // 总线类型：检查所有连接是否有效
        for (const auto& conn : m_busConnections) {
            if (!conn.connectedBuffer || !conn.bridgeReader) {
                return false;
            }
        }
        return !m_busConnections.empty();
    } else {
        // 非总线类型：检查常规连接
        return m_connectedBuffer != nullptr;
    }
}

bool BufferReader::IsConnectedToBuffer(Buffer *buffer) const {
    return m_connectedBuffer == buffer;
}

bool BufferReader::IsUpstreamDone() const
{
    // 对于桥接读取器，检查连接的缓冲区
    if (m_readerType == BUS_BRIDGE && m_connectedBuffer) {
        return m_connectedBuffer->IsUpstreamDone();
    }

    // 对于主总线读取器，检查所有连接
    if (m_readerType == BUS_MASTER) {
        for (const auto& connection : m_busConnections) {
            if (connection.connectedBuffer && !connection.connectedBuffer->IsUpstreamDone()) {
                return false;
            }
        }
        return !m_busConnections.empty();
    }

    // 标准读取器
    return m_connectedBuffer ? m_connectedBuffer->IsUpstreamDone() : false;
}

bool BufferReader::IsDownstreamDone() const {
    if (m_isBusType) {
//        const auto& busConnections = GetBusConnections();
        if(!m_busConnections.empty()) {
            return IsBusDownstreamDone();
        }
    }
    return m_downstreamDone;
}

void BufferReader::SetDownstreamDone(bool done) {
    m_downstreamDone = done;
}

SystemVueModelBuilder::DFInterface *BufferReader::GetDFInterface()
{
    return &m_dfinterface;
}

size_t BufferReader::GetBusAvailableDataCount() const
{
    //获取bus类型的连接的可用数据
    if (m_busConnections.empty()) {
//        qDebug() << "ERROR: No bus connections found for reader: " << QString::fromStdString(m_name);
        return 0;
    }

    // 返回所有连接中的最小可用数据量
    size_t minAvailable = SIZE_MAX;
    for (const auto& conn : m_busConnections) {
        if (conn.connectedBuffer && conn.bridgeReader) {
            size_t available = conn.connectedBuffer->GetAvailableDataForReader(conn.bridgeReader->GetName());
            qDebug() << "Connection " << QString::fromStdString(conn.upstreamPortName) << " available data: " << available;
            minAvailable = std::min(minAvailable, available);
        }
    }

    return (minAvailable == SIZE_MAX) ? 0 : minAvailable;
}

void BufferReader::createDataVariant(DataType type)
{
    m_dataType = type;
    switch(type) {
    //基础类型，初始化为单个值
    case DataType::INT:
        m_dataVariant = int(0);
        break;
    case DataType::DOUBLE:
        m_dataVariant = double(0.0);
        break;
    case DataType::FLOAT:
        m_dataVariant = float(0.0f);
        break;
    case DataType::BOOL:
        m_dataVariant = bool(false);
        break;
    case DataType::COMPLEX_FLOAT:
        m_dataVariant = std::complex<float>(0.0f, 0.0f);
        break;
    case DataType::COMPLEX_DOUBLE:
        m_dataVariant = std::complex<double>(0.0, 0.0);
        break;
    //数组类型，初始化为空
    case DataType::INT_ARRAY:
        m_dataVariant = static_cast<int*>(nullptr);
        break;
    case DataType::DOUBLE_ARRAY:
        m_dataVariant = static_cast<double*>(nullptr);
        break;
    case DataType::COMPLEX_DOUBLE_ARRAY:
        m_dataVariant = static_cast<std::complex<double>*>(nullptr);
        break;
    //Circularbuffer类型初始化
    case DataType::CIRCULAR_BUFFER_INT:
        m_dataVariant = SystemVueModelBuilder::CircularBuffer<int>();
        break;
    case DataType::CIRCULAR_BUFFER_DOUBLE:
        m_dataVariant = SystemVueModelBuilder::CircularBuffer<double>();
        break;
    case DataType::CIRCULAR_BUFFER_FLOAT:
        m_dataVariant = SystemVueModelBuilder::CircularBuffer<float>();
        break;
    case DataType::CIRCULAR_BUFFER_BOOL:
        m_dataVariant = SystemVueModelBuilder::CircularBuffer<bool>();
        break;
    case DataType::CIRCULAR_BUFFER_FCOMPLEX:
        m_dataVariant = SystemVueModelBuilder::CircularBuffer<std::complex<float>>();
        break;
    case DataType::CIRCULAR_BUFFER_DCOMPLEX:
        m_dataVariant = SystemVueModelBuilder::CircularBuffer<std::complex<double>>();
        break;
    case DataType::TIMED_INT:
        m_dataVariant = SystemVueModelBuilder::TimedCircularBuffer<int>();
        break;
    case DataType::TIMED_DOUBLE:
        m_dataVariant = SystemVueModelBuilder::TimedCircularBuffer<double>();
        break;
    case DataType::TIMED_FLOAT:
        m_dataVariant = SystemVueModelBuilder::TimedCircularBuffer<float>();
        break;
    case DataType::TIMED_BOOL:
        m_dataVariant = SystemVueModelBuilder::TimedCircularBuffer<bool>();
        break;
    case DataType::TIMED_FCOMPLEX:
        m_dataVariant = SystemVueModelBuilder::TimedCircularBuffer<std::complex<float>>();
        break;
    case DataType::TIMED_DCOMPLEX:
        m_dataVariant = SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>>();
        break;
    //bus类型初始化
    case DataType::INT_BUS:
        m_dataVariant = std::make_unique<SystemVueModelBuilder::IntCircularBufferBus>();
        break;
    case DataType::DOUBLE_BUS:
        m_dataVariant = std::make_unique<SystemVueModelBuilder::DoubleCircularBufferBus>();
        break;
    case DataType::FLOAT_BUS:
        m_dataVariant = std::make_unique<SystemVueModelBuilder::FloatCircularBufferBus>();
        break;
    case DataType::BOOL_BUS:
        m_dataVariant = std::make_unique<SystemVueModelBuilder::BoolCircularBufferBus>();
        break;
    case DataType::FCOMPLEX_BUS:
        m_dataVariant = std::make_unique<SystemVueModelBuilder::FComplexCircularBufferBus>();
        break;
    case DataType::DCOMPLEX_BUS:
        m_dataVariant = std::make_unique<SystemVueModelBuilder::DComplexCircularBufferBus>();
        break;
    //包络
    case DataType::ENVELOPE_SIGNAL:
        m_dataVariant = SystemVueModelBuilder::EnvelopeCircularBuffer();
        break;
    //矩阵
    case DataType::MATRIX_INT:
        m_dataVariant = SystemVueModelBuilder::IntMatrixCircularBuffer();
        break;
    case DataType::MATRIX_DOUBLE:
        m_dataVariant = SystemVueModelBuilder::DoubleMatrixCircularBuffer();
        break;
    case DataType::MATRIX_FLOAT:
        m_dataVariant = SystemVueModelBuilder::FloatMatrixCircularBuffer();
        break;
    case DataType::MATRIX_BOOL:
        m_dataVariant = SystemVueModelBuilder::BoolMatrixCircularBuffer();
        break;
    case DataType::MATRIX_FCOMPLEX:
        m_dataVariant = SystemVueModelBuilder::FComplexMatrixCircularBuffer();
        break;
    case DataType::MATRIX_DCOMPLEX:
        m_dataVariant = SystemVueModelBuilder::DComplexMatrixCircularBuffer();
        break;
    case DataType::MATRIX_ENVELOPE:
        m_dataVariant = SystemVueModelBuilder::EnvelopeMatrixCircularBuffer();
        break;
    default:
        m_dataVariant = int(0);
        m_dataType = DataType::INT;
        break;
    }
}


void BufferReader::EnsureProperRegistration(Buffer *buffer)
{
//    qDebug() << "=== BufferReader::EnsureProperRegistration ===";
//    qDebug() << "Reader: '" << QString::fromStdString(m_name) << "'";

     // 检查读取器是否已经在缓冲区中注册
     bool isRegistered = buffer->FindRegisterReader(m_name);

     // 检查注册状态
     // 这里需要根据你的Buffer实现调整
     // 假设Buffer有方法来检查读取器注册状态

     // 如果没有注册，尝试注册
     if (!isRegistered) {
         qDebug() << "Reader not registered in buffer, attempting to register...";

         // 尝试直接添加读取器
         size_t readerId = buffer->AddReader(this);
         qDebug() << "AddReader returned ID: " << readerId;

         // 或者使用RegisterReader方法
         buffer->RegisterReader(m_name, this);
         qDebug() << "Registered reader in buffer";
     } else {
         qDebug() << "Reader already registered in buffer";
     }

     // 验证连接
     if (buffer->FindRegisterReader(m_name)) {
         qDebug() << "Reader registration confirmed";

         // 检查可用的数据
         size_t available = buffer->GetAvailableDataForReader(m_name);
         qDebug() << "Available data for reader: " << available;
     } else {
         qDebug() << "WARNING: Reader registration failed!";
     }

     qDebug() << "=== EnsureProperRegistration END ===";
}

void BufferReader::NotifySpaceAvailable()
{
    if (m_connectedBuffer) {
        m_connectedBuffer->NotifySpaceAvailable();
    }
}
