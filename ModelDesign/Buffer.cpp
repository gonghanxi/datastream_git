#include "Buffer.h"
#include "BufferReader.h"

#include "BufferWriteImpl.h"
#include "BufferReadImpl.h"
#include "BufferBusDataImpl.h"
#include "BufferMemoryImpl.h"
#include "BufferExpansionImpl.h"

#include <algorithm>



using namespace SystemVueModelBuilder;

Buffer::Buffer(const std::string& name, size_t writeSize, DataType type)
    : m_name(name)
    ,m_dataType(type)
    ,m_maxBufferSize(1024 * 1024 * 100)  // 默认100MB上限
    ,m_trueOriginalBufferSize(1024)  // 初始化为默认值
    ,m_dataCount(0)
    ,m_writePosition(0)
    ,m_writeSize(writeSize)    
    ,m_readerPositions(0)    
    ,m_envelopeFc(0.0)    
    ,m_hasEnvelopeFc(false)    
    ,m_originalBufferSize(1024)
    , m_isExpanded(false)
    ,m_expansionStartPoint(SIZE_MAX)

{
    //CircularBuffer类型m_bufferSize都默认为1024
    m_bufferSize = 1024;
    // 新增：记录真实的原始大小
    m_trueOriginalBufferSize = m_bufferSize;
    //实现类指针初始化
    m_writer = std::make_unique<BufferWriteImpl>(this);
    m_readerImpl = std::make_unique<BufferReadImpl>(this);
    m_busProcessor = std::make_unique<BufferBusDataImpl>(this);
    m_memoryImpl = std::make_unique<BufferMemoryImpl>(this);
    m_expansionImpl = std::make_unique<BufferExpansionImpl>(this);

    //普通类型m_bufferSize默认大小为1
    if(m_dataType == DataType::INT ||
            m_dataType == DataType::DOUBLE ||
            m_dataType == DataType::FLOAT ||
            m_dataType == DataType::BOOL ||
            m_dataType == DataType::COMPLEX_FLOAT ||
            m_dataType == DataType::COMPLEX_DOUBLE)
    {
        m_bufferSize = 1;
    }

    //bus类型内部指针初始化
    if(IsBusType(m_dataType)) {
        switch(m_dataType) {
        case DataType::INT_BUS:
            m_outputBuffer = static_cast<SystemVueModelBuilder::IntCircularBufferBus*>(nullptr);
            break;
        case DataType::DOUBLE_BUS:
            m_outputBuffer = static_cast<SystemVueModelBuilder::DoubleCircularBufferBus*>(nullptr);
            break;
        case DataType::FLOAT_BUS:
            m_outputBuffer = static_cast<SystemVueModelBuilder::FloatCircularBufferBus*>(nullptr);
            break;
        case DataType::BOOL_BUS:
            m_outputBuffer = static_cast<SystemVueModelBuilder::BoolCircularBufferBus*>(nullptr);
            break;
        case DataType::FCOMPLEX_BUS:
            m_outputBuffer = static_cast<SystemVueModelBuilder::FComplexCircularBufferBus*>(nullptr);
            break;
        case DataType::DCOMPLEX_BUS:
            m_outputBuffer = static_cast<SystemVueModelBuilder::DComplexCircularBufferBus*>(nullptr);
            break;
        default:
            m_outputBuffer = std::make_unique<SystemVueModelBuilder::IntCircularBuffer>();
            break;
        }
    }
}

Buffer::~Buffer()
{
    // 清理读取器数据
    m_readerPositions.clear();
    m_readerReadSizes.clear();
    m_readerReadPositions.clear();
    m_readerObjects.clear();

    if (m_usingExternalCircularBuffer) {
//        qDebug() << "External buffer mode - special handling";

        if (m_externalCircularBuffer) {
            //获取外部端口的缓冲区
            void* memory = m_externalCircularBuffer->GetBufferMemory();
//            qDebug() << "External buffer memory: " << memory;

            if (memory) {
//                qDebug() << "Freeing external buffer memory...";
                try {
                    // 根据数据类型正确释放
                    switch (m_dataType) {
                    case DataType::CIRCULAR_BUFFER_INT:
                    case DataType::TIMED_INT:
                        delete[] static_cast<int*>(memory);
                        break;
                    case DataType::TIMED_DOUBLE:
                    case DataType::CIRCULAR_BUFFER_DOUBLE:
                        delete[] static_cast<double*>(memory);
                        break;
                    case DataType::TIMED_FLOAT:
                    case DataType::CIRCULAR_BUFFER_FLOAT:
                        delete[] static_cast<float*>(memory);
                        break;
                    case DataType::TIMED_BOOL:
                    case DataType::CIRCULAR_BUFFER_BOOL:
                        delete[] static_cast<bool*>(memory);
                        break;
                    case DataType::CIRCULAR_BUFFER_DCOMPLEX:
                    case DataType::TIMED_DCOMPLEX:
//                        qDebug() << "freeing dcomplex memory";
                        delete[] static_cast<std::complex<double>*>(memory);
                        break;
                    case DataType::CIRCULAR_BUFFER_FCOMPLEX:
                    case DataType::TIMED_FCOMPLEX:
                        delete[] static_cast<std::complex<float>*>(memory);
                        break;
                    case DataType::ENVELOPE_SIGNAL:
                        delete[] static_cast<SystemVueModelBuilder::EnvelopeSignal*>(memory);
                        break;
                    case DataType::MATRIX_INT:
                    case DataType::MATRIX_TIME_INT:
                        delete[] static_cast<SystemVueModelBuilder::IntMatrix*>(memory);
                        break;
                    case DataType::MATRIX_DOUBLE:
                    case DataType::MATRIX_TIME_DOUBLE:
                        delete[] static_cast<SystemVueModelBuilder::DoubleMatrix*>(memory);
                        break;
                    case DataType::MATRIX_FLOAT:
                    case DataType::MATRIX_TIME_FLOAT:
                        delete[] static_cast<SystemVueModelBuilder::FloatMatrix*>(memory);
                        break;
                    case DataType::MATRIX_BOOL:
                    case DataType::MATRIX_TIME_BOOL:
                        delete[] static_cast<SystemVueModelBuilder::BoolMatrix*>(memory);
                        break;
                    case DataType::MATRIX_FCOMPLEX:
                    case DataType::MATRIX_TIME_FCOMPLEX:
                        delete[] static_cast<SystemVueModelBuilder::FComplexMatrix*>(memory);
                        break;
                    case DataType::MATRIX_DCOMPLEX:
                    case DataType::MATRIX_TIME_DCOMPLEX:
                        delete[] static_cast<SystemVueModelBuilder::DComplexMatrix*>(memory);
                        break;
                    case DataType::MATRIX_ENVELOPE:
                        delete[] static_cast<SystemVueModelBuilder::EnvelopeMatrix*>(memory);
                        break;
                    default:
                        qDebug() << "freeing unkown memory";
                        delete[] static_cast<double*>(memory);
                        break;
                    }
                } catch (const std::exception& e) {
                    qDebug() << "ERROR freeing external memory: " << e.what();
                }
            }
        }

    } else if (IsBusType(m_dataType)) {
        //bus类型
        m_outputBuffer = {};
        m_allocatedMemory.reset();

    } else {
        //未知类型
        m_outputBuffer = {};
        m_allocatedMemory.reset();
    }

    m_externalCircularBuffer = nullptr;
    m_usingExternalCircularBuffer = false;

    qDebug() << "=== Buffer destructor completed ===";
}

size_t Buffer::AddReader(size_t readSize)
{
    //添加读指针
    size_t readerIndex = m_readerReadSizes.size();
    m_readerReadSizes.push_back(readSize);

    // 同步调整 m_readerNames
    if (readerIndex >= m_readerNames.size()) {
        m_readerNames.push_back("");  // 添加空字符串占位
    }

    // 更新缓冲区大小以适应新的reader
//    UpdateBufferSize();

    return readerIndex;
}

size_t Buffer::AddReader(BufferReader *reader)
{
    //添加读指针
    if (!reader) {
        qDebug() << "ERROR: Cannot add NULL reader";
        return SIZE_MAX;
    }

    std::string readerName = reader->GetName();
    size_t readSize = reader->GetReadSize();

    //
    return AddReader(readSize, readerName);
}

size_t Buffer::AddReader(size_t readSize, const std::string& readerName)
{
    //添加读指针
    if (readerName.empty()) {
        qDebug() << "ERROR: Reader name is empty";
        return SIZE_MAX;
    }

    // 检查是否已存在
    if (m_readerObjects.find(readerName) != m_readerObjects.end()) {
        qDebug() << "Reader '" << QString::fromStdString(readerName) << "' already registered";

        // 返回现有索引
        for (size_t i = 0; i < m_readerReadSizes.size(); i++) {
            if (m_readerPositions.count(readerName) > 0) {
                return i;
            }
        }
        return SIZE_MAX;
    }

    // 添加读取大小
    size_t readerIndex = AddReader(readSize);

    // 记录读取器名称
    m_readerNames[readerIndex] = readerName;

    // 关键：在 m_readerObjects 中注册（但暂时没有 reader 对象指针）
    // 我们将在后续通过 RegisterReader 设置
    m_readerObjects[readerName] = nullptr;  // 先设置为 nullptr

    // 初始化读取器位置
    m_readerPositions[readerName] = 0;

    qDebug() << "Successfully added reader '" << QString::fromStdString(readerName)
              << "' at index " << readerIndex;

    // 打印当前所有读取器
     qDebug() << "Current readers: ";
    for (const auto& name : m_readerNames) {
        if (!name.empty()) {
             qDebug() << QString::fromStdString(name) << " ";
        }
    }

    return readerIndex;
}

size_t Buffer::GetAvailableDataForReader(const std::string& readerName)
{
    //获取对应读指针此时的可读数量

    // 检查是否为普通类型端口
    if (!m_externalBasicOutputPorts.empty()) {

        // 对于外部端口，如果有数据写入，总是有数据可用
        // 因为数据是直接传递的，没有缓冲区概念
        if (m_totalWritten > 0) {
            // 查找是否有这个读取器
            auto it = m_readerPositions.find(readerName);
            if (it != m_readerPositions.end()) {
                size_t readerPos = it->second;

                // 对于外部端口，如果读取器还没有读过数据，就返回有数据可用
                if (readerPos < m_totalWritten) {
                    return 1; // 外部端口一次只能读一个数据
                } else {
                    return 0;
                }
            }
        }
        qDebug() << "External port has no data or reader not found";
        return 0;
    }


    if (readerName.empty()) {
        qDebug() << "ERROR: Reader name is empty";
        return 0;
    }

    auto it = m_readerPositions.find(readerName);
    if (it == m_readerPositions.end()) {
//        qDebug() << "Reader '" << QString::fromStdString(readerName) << "' not found in m_readerPositions";
//        qDebug() << "Registered readers: ";
        for (const auto& pair : m_readerPositions) {
            qDebug() << QString::fromStdString(pair.first) << " ";
        }
        return 0;
    }

    //获取可读数量
    size_t readerPos = it->second;
    size_t available = (m_totalWritten >= readerPos) ? (m_totalWritten - readerPos) : 0;

//    qDebug() << "Reader position: " << readerPos
//              << ", Total written: " << m_totalWritten
//              << ", Available: " << available;

    return available;
}

void Buffer::RegisterReader(const std::string &readerName, BufferReader *reader)
{
    //注册读指针
    if (readerName.empty()) {
        qDebug() << "ERROR: Reader name is empty";
        return;
    }

    if (!reader) {
        qDebug() << "ERROR: Cannot register NULL reader";
        return;
    }

    // 检查是否已存在
    auto it = m_readerObjects.find(readerName);
    if (it != m_readerObjects.end()) {
        if (it->second != nullptr && it->second != reader) {
            qDebug() << "WARNING: Overwriting existing reader object for '" << QString::fromStdString(readerName) << "'";
        }
    }

    // 注册读取器对象
    m_readerObjects[readerName] = reader;

    qDebug() << "Reader '" << QString::fromStdString(readerName) << "' registered successfully";
}

void Buffer::UnRegisterReader(const std::string &readerName)
{
    //注销已注册的读指针
    auto it = m_readerPositions.find(readerName);
    if (it != m_readerPositions.end()) {
        m_readerPositions.erase(it);
    }
    m_readerObjects.erase(readerName);
}

bool Buffer::FindRegisterReader(const std::string &readerName)
{
    //判断读指针是否注册成功
    auto it = m_readerPositions.find(readerName);
    if (it != m_readerPositions.end()) {
        return true;
    }
    return false;
}
void Buffer::SetDataType(DataType type)
{
    m_dataType = type;
}

Buffer::DataType Buffer::GetDataType() const
{
    return m_dataType;
}

size_t Buffer::GetReaderCount() const
{
    //获取读指针数量
    return m_readerReadSizes.size();
}

std::string Buffer::GetName() const
{
    return m_name;
}

size_t Buffer::GetBufferSize() const
{
    //获取Buffer的大小
    return m_bufferSize;
}

size_t Buffer::GetTotalWritten() const
{
    //获取总写入大小
    return m_totalWritten;
}

size_t Buffer::GetUsedSpace() const
{
    if (m_totalWritten == 0) return 0;

    //找到最慢的读指针位置
    size_t slowestReader = FindSlowestReaderPosition();
    return m_totalWritten - slowestReader;
}

size_t Buffer::GetBufferFreeSpace() const
{
    //获取bus类型空闲空间
    if (m_dataType == DataType::INT_BUS ||
        m_dataType == DataType::DOUBLE_BUS ||
        m_dataType == DataType::FLOAT_BUS ||
        m_dataType == DataType::BOOL_BUS ||
        m_dataType == DataType::DCOMPLEX_BUS ||
        m_dataType == DataType::FCOMPLEX_BUS) {
            return GetBusBufferFreeSpace();
        }
    if (m_usingExternalCircularBuffer && m_externalCircularBuffer) {
        // 外部缓冲区：使用实际的数据计数
        if (m_dataCount >= m_bufferSize) {
            return 0; // 缓冲区已满
        }
        return m_bufferSize - m_dataCount;
        }
    return m_bufferSize - m_dataCount;
}

size_t Buffer::GetReaderPosition(const std::string &readerName) const {
    //获取读指针位置
    auto it = m_readerPositions.find(readerName);
    if (it != m_readerPositions.end()) {
        return it->second;
    }
    throw std::runtime_error("Reader not found: " + readerName);
}

bool Buffer::SetReaderPosition(const std::string &readerName, size_t newPosition)
{
    // 1. 检查 reader 是否存在
    auto it = m_readerPositions.find(readerName);
    if (it == m_readerPositions.end()) {
        // Reader 不存在
        return false;
    }

    // 2. 边界检查
    if (newPosition > m_bufferSize) {  // 假设 m_data 存储实际数据
        // 位置超出缓冲区大小
        LOG_ERROR("Can not set reader position");
        return false;
    }

    // 3. 设置新位置
    it->second = newPosition;
    return true;
}

std::vector<BufferReader *> Buffer::GetReaders() const
{
    // 创建一个空的 vector 用于存放结果
    std::vector<BufferReader*> readers;

    // 预分配内存以提高效率（可选）
    readers.reserve(m_readerObjects.size());

    // 遍历 unordered_map，将每个键值对（Key-Value Pair）中的 value（即 BufferReader*）存入 vector
    for (const auto& pair : m_readerObjects) {
        readers.push_back(pair.second); // pair.second 是 BufferReader* 类型的值
    }

    return readers;
}

void Buffer::SetMaxSize(size_t maxSize) { m_maxSize = maxSize; }

size_t Buffer::GetMaxSize() const { return m_maxSize; }

float Buffer::GetUsage() const {
    if (m_maxSize == 0) return 0.0f;
    size_t usedSpace = GetUsedSpace();
    return (float)usedSpace / m_maxSize * 100.0f;
}

void Buffer::setBackpressureCallback(Buffer::BackpressureCallback callback) {
    m_backpressureCallback = callback;
}

void Buffer::NotifySpaceAvailable()
{
    // 当下游消费数据释放空间时调用
    size_t currentUsed = GetUsedSpace();
    if (m_backpressureCallback && currentUsed < m_maxSize * 0.7f) {
        // 空间充足，解除背压
        m_backpressureCallback(this, false);
    }
}

void *Buffer::GetWriter() const { return m_writerPtr; }

void Buffer::SetWriter(void *writer) { m_writerPtr = writer; }

size_t Buffer::GetWriteSize() const { return m_writeSize; }

void Buffer::SetWriteSize(size_t size) { m_writeSize = size; }

bool Buffer::WriteData(int data)
{
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(double data)
{
    //写入方法
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(float data)
{
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(bool data)
{
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(std::complex<float> data)
{
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(std::complex<double> data)
{
    return m_writer->WriteData(data);
}


bool Buffer::WriteData(const std::vector<int> &data)
{
    //写入方法
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<double> &data)
{
    //写入方法
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<float> &data)
{
    //写入方法
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<bool> &data)
{
    //写入方法
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<std::complex<float> > &data)
{
    //写入方法
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<std::complex<double> > &data)
{
    //写入方法
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<int *> &data)
{
    //写入方法
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<double *> &data)
{
    //写入方法
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<std::complex<double> *> &data)
{
    //写入方法
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const SystemVueModelBuilder::CircularBufferBase &data)
{
    //写入方法
    return m_writer->WriteData(data);
}

bool Buffer::WriteEnvelopeDataToChannel(int channelIndex, const std::vector<EnvelopeSignal> &data, double fc)
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

    connection.bridgeWriter->setCharacterizationFrequency(fc);

    // 调用桥接写入器的 WriteData 方法
    connection.bridgeWriter->WriteData(data);
}

bool Buffer::WriteData(const SystemVueModelBuilder::CircularBufferBus &data)
{
    //写入方法
    m_busProcessor->WriteBusData(data);
    return true;
}

bool Buffer::WriteData(const SystemVueModelBuilder::EnvelopeSignal &data)
{
    //写入方法
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<SystemVueModelBuilder::EnvelopeSignal> &data)
{
    //写入方法
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<IntMatrix> &data)
{
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<DoubleMatrix> &data)
{
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<FloatMatrix> &data)
{
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<BoolMatrix> &data)
{
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<FComplexMatrix> &data)
{
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<DComplexMatrix> &data)
{
    return m_writer->WriteData(data);
}

bool Buffer::WriteData(const std::vector<EnvelopeMatrix> &data)
{
    return m_writer->WriteData(data);
}

bool Buffer::ReadDataForReader(int &outputData, const std::string &readerName)
{
    return m_readerImpl->ReadDataForReader(outputData, readerName);
}

bool Buffer::ReadDataForReader(double &outputData, const std::string &readerName)
{
    return m_readerImpl->ReadDataForReader(outputData, readerName);
}

bool Buffer::ReadDataForReader(float &outputData, const std::string &readerName)
{
    return m_readerImpl->ReadDataForReader(outputData, readerName);
}

bool Buffer::ReadDataForReader(bool &outputData, const std::string &readerName)
{
    return m_readerImpl->ReadDataForReader(outputData, readerName);
}

bool Buffer::ReadDataForReader(std::complex<float> &outputData, const std::string &readerName)
{
    return m_readerImpl->ReadDataForReader(outputData, readerName);
}

bool Buffer::ReadDataForReader(std::complex<double> &outputData, const std::string &readerName)
{
    return m_readerImpl->ReadDataForReader(outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<int> &outputData, const std::string &readerName)
{
    //读指针读取方法
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<double> &outputData, const std::string &readerName)
{
    //读指针读取方法
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<float> &outputData, const std::string &readerName)
{
    //读指针读取方法
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<bool> &outputData, const std::string &readerName)
{
    //读指针读取方法
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<std::complex<float> > &outputData, const std::string &readerName)
{
    //读指针读取方法
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<std::complex<double>>& outputData, const std::string& readerName)
{
    //读指针读取方法
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<int *> &outputData, const std::string &readerName)
{
    //读指针读取方法
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<double *> &outputData, const std::string &readerName)
{
    //读指针读取方法
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<std::complex<double> *> &outputData, const std::string &readerName)
{
    //读指针读取方法
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, SystemVueModelBuilder::CircularBufferBase &outputData, const std::string &readerName)
{
    //读指针读取方法
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, SystemVueModelBuilder::CircularBufferBus &outputData, const std::string &readerName)
{
    //读指针读取方法
    return m_busProcessor->ReadBusDataForReader(readSize, outputData, readerName);
}


bool Buffer::ReadDataForReader(size_t readSize, std::vector<EnvelopeSignal> &outputData, const std::string &readerName)
{
    //读指针读取方法
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, SystemVueModelBuilder::EnvelopeCircularBuffer &outputData, const std::string &readerName)
{
    //读指针读取方法
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<IntMatrix> &outputData, const std::string &readerName)
{
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<DoubleMatrix> &outputData, const std::string &readerName)
{
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<FloatMatrix> &outputData, const std::string &readerName)
{
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<BoolMatrix> &outputData, const std::string &readerName)
{
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<FComplexMatrix> &outputData, const std::string &readerName)
{
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<DComplexMatrix> &outputData, const std::string &readerName)
{
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

bool Buffer::ReadDataForReader(size_t readSize, std::vector<EnvelopeMatrix> &outputData, const std::string &readerName)
{
    return m_readerImpl->ReadDataForReader(readSize, outputData, readerName);
}

void Buffer::UpdateBufferSize()
{
    //更新buffer的大小
    m_memoryImpl->UpdateBufferSize();
}

void Buffer::ReallocateBufferMemory()
{
    //分配buffer的空间
    m_memoryImpl->ReallocateBufferMemory();
}

void Buffer::ReallocateExternalBuffer()
{
    m_memoryImpl->ReallocateExternalBuffer();
}

size_t Buffer::CalculateLCM(size_t a, size_t b)
{
    //计算最小公倍数
    if (a == 0 || b == 0) {
        return 0;
    }
    return (a * b) / CalculateGCD(a, b);
}

size_t Buffer::CalculateGCD(size_t a, size_t b)
{
    //计算公因子
    while (b != 0) {
        size_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

void Buffer::ResetBuffer()
{

    // 重置写入位置和数据计数
    m_writePosition = 0;
    m_dataCount = 0;
    // 清空物理缓冲区
    std::visit([](auto&& buffer){
        using T = std::decay_t<decltype(buffer)>;

        if constexpr (std::is_pointer_v<T>) {
            // 对于总线指针，如果存在则重置每个端口的缓冲区
            if (buffer) {
                try {
                    // 通过 dynamic_cast 获取具体类型
                    auto* intBus = dynamic_cast<SystemVueModelBuilder::IntCircularBufferBus*>(buffer);
                    if (intBus) {
                        size_t portCount = intBus->GetSize();
                        for (size_t i = 0; i < portCount; i++) {
                            auto& portBuffer = (*intBus)[i];
                            //初始化
                            portBuffer.Initialize();
                        }
                        return;
                    }

                    auto* doubleBus = dynamic_cast<SystemVueModelBuilder::DoubleCircularBufferBus*>(buffer);
                    if (doubleBus) {
                        size_t portCount = doubleBus->GetSize();
                        for (size_t i = 0; i < portCount; i++) {
                            auto& portBuffer = (*doubleBus)[i];
                            //初始化
                            portBuffer.Initialize();
                        }
                        return;
                    }

                    auto* dcomplexBus = dynamic_cast<SystemVueModelBuilder::DComplexCircularBufferBus*>(buffer);
                    if (dcomplexBus) {
                        size_t portCount = dcomplexBus->GetSize();
                        for (size_t i = 0; i < portCount; i++) {
                            auto& portBuffer = (*dcomplexBus)[i];
                            //初始化
                            portBuffer.Initialize();
                        }
                        return;
                    }

                    auto* floatBus = dynamic_cast<SystemVueModelBuilder::FloatCircularBufferBus*>(buffer);
                    if (floatBus) {
                        size_t portCount = floatBus->GetSize();
                        for (size_t i = 0; i < portCount; i++) {
                            auto& portBuffer = (*floatBus)[i];
                            //初始化
                            portBuffer.Initialize();
                        }
                        return;
                    }

                    auto* boolBus = dynamic_cast<SystemVueModelBuilder::BoolCircularBufferBus*>(buffer);
                    if (boolBus) {
                        size_t portCount = boolBus->GetSize();
                        for (size_t i = 0; i < portCount; i++) {
                            auto& portBuffer = (*boolBus)[i];
                            //初始化
                            portBuffer.Initialize();
                        }
                        return;
                    }

                    auto* fcomplexBus = dynamic_cast<SystemVueModelBuilder::FComplexCircularBufferBus*>(buffer);
                    if (fcomplexBus) {
                        size_t portCount = fcomplexBus->GetSize();
                        for (size_t i = 0; i < portCount; i++) {
                            auto& portBuffer = (*fcomplexBus)[i];
                            //初始化
                            portBuffer.Initialize();
                        }
                        return;
                    }

                } catch (...) {
                    qDebug() << "ERROR: Failed to reset bus buffer";
                }
            }
        } else {
            // 对于普通 CircularBuffer
            if(buffer) {
                buffer->Initialize();
            }
        }
    }, m_outputBuffer);

}
void Buffer::ResetReaderPoint()
{
    // 重置所有reader位置
    for (auto& reader : m_readerPositions) {
        reader.second = 0;
    }
}

size_t Buffer::FindSlowestReaderPosition() const
{
    if (m_readerPositions.empty()) {
        return 0; // 没有reader时，所有数据都可用
    }

    size_t slowest = m_totalWritten; // 初始化为最大值

    for (const auto& reader : m_readerPositions) {
        if (reader.second < slowest) {
            slowest = reader.second;
        }
    }

    // 确保不会出现负数情况
    return (slowest <= m_totalWritten) ? slowest : m_totalWritten;
}

void Buffer::EnsureCircularBuffer()
{
    m_memoryImpl->EnsureCircularBuffer();
}

void Buffer::EnsureTimedCircularBuffer()
{
    m_memoryImpl->EnsureTimedCircularBuffer();
}



void Buffer::CreateBufferVariantWithoutAllocation()
{
    //内部缓冲区指针初始化
    m_memoryImpl->CreateBufferVariantWithoutAllocation();
}

void Buffer::WireInternalBufferToExternalMemory()
{
    //将内部缓冲区指针连接到外部内存上面
    m_memoryImpl->WireInternalBufferToExternalMemory();
}

void Buffer::SetUpstreamDone(bool done) { m_upstreamDone = done; }

bool Buffer::IsUpstreamDone() const { return m_upstreamDone; }

bool Buffer::IsDownstreamDone() const
{
    // 如果是总线类型，需要检查所有下游
    if (m_dataType == DataType::INT_BUS ||
        m_dataType == DataType::BOOL_BUS ||
        m_dataType == DataType::FLOAT_BUS ||
        m_dataType == DataType::DOUBLE_BUS ||
        m_dataType == DataType::FCOMPLEX_BUS ||
        m_dataType == DataType::DCOMPLEX_BUS) {
        return IsBusDownstreamDone();
    }
    // 如果没有reader，认为下游已完成
    if (m_readerPositions.empty()) {
//        qDebug() << "DEBUG: "<< "Buffer '"<< QString::fromStdString(GetName()) << "': No readers registered in m_readerPositions";]
        return true;
    }

    // 检查所有reader是否都已完成
    for (const auto& readerPair : m_readerPositions) {
        const std::string& readerName = readerPair.first;
//        size_t readerPosition = readerPair.second;

        auto it = m_readerObjects.find(readerName);
        if (it == m_readerObjects.end()) {
            qDebug() << "DEBUG: Reader '" << QString::fromStdString(readerName)
                      << "' not found in m_readerObjects!";
            return false;
        }
        BufferReader* reader = it->second;
        if (reader) {
            qDebug() << "reader name: " << QString::fromStdString(reader->GetName());
            if (!reader->IsDownstreamDone()) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

void Buffer::SetDownstreamDone(bool done)
{
    //设置下游完成状态
    for (auto& readerPair : m_readerObjects) {
        BufferReader* reader = readerPair.second;
        if (reader) {
            reader->SetDownstreamDone(done);
        }
    }
}

bool Buffer::SetExternalCircularBuffer(SystemVueModelBuilder::CircularBufferBase* externalBuffer)
{
    return m_memoryImpl->SetExternalCircularBuffer(externalBuffer);
}

bool Buffer::SetExternalCircularBufferBus(CircularBufferBus *externalBus)
{
    if (!externalBus) return false;

    m_externalCircularBufferBus = externalBus;
    m_usingExternalCircularBufferBus = true;
    return true;
}

CircularBufferBus *Buffer::GetExternalCircularBufferBus() const
{
    return m_externalCircularBufferBus;
}

Buffer::WriterType Buffer::GetWriterType() const
{
    return m_writerType;
}

void Buffer::SetWriterType(Buffer::WriterType type)
{
    m_writerType = type;
}

std::string Buffer::WriterTypeToString(Buffer::WriterType type)
{
    switch (type) {
    case STANDARD: return "STANDARD";
    case BUS_MASTER: return "BUS_MASTER";
    case BUS_BRIDGE: return "BUS_BRIDGE";
    default: return "UNKNOWN";
    }
}

CircularBufferBase *Buffer::CreateCircularBufferByDataType(Buffer::DataType dataType)
{
    SystemVueModelBuilder::CircularBufferBase* channelBuffer = nullptr;

    switch (dataType) {
    case DataType::CIRCULAR_BUFFER_INT:
    {
        channelBuffer = new IntCircularBuffer();
        int* newBuffer = new int[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::TIMED_INT:
    {
        channelBuffer = new TimedCircularBuffer<int>();
        int* newBuffer = new int[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::CIRCULAR_BUFFER_DOUBLE:
    {
        channelBuffer = new DoubleCircularBuffer();
        double* newBuffer = new double[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::TIMED_DOUBLE:
    {
        channelBuffer = new TimedCircularBuffer<double>();
        double* newBuffer = new double[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::CIRCULAR_BUFFER_FLOAT:
    {
        channelBuffer = new FloatCircularBuffer;
        float* newBuffer = new float[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::TIMED_FLOAT:
    {
        channelBuffer = new TimedCircularBuffer<float>();
        float* newBuffer = new float[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::CIRCULAR_BUFFER_BOOL:
    {
        channelBuffer = new BoolCircularBuffer;
        bool* newBuffer = new bool[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::TIMED_BOOL:
    {
        channelBuffer = new TimedCircularBuffer<bool>();
        bool* newBuffer = new bool[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::CIRCULAR_BUFFER_DCOMPLEX:
    {
        channelBuffer = new DComplexCircularBuffer();
        std::complex<double>* newBuffer = new std::complex<double>[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::TIMED_DCOMPLEX:
    {
        channelBuffer = new TimedCircularBuffer<std::complex<double>>();
        std::complex<double>* newBuffer = new std::complex<double>[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::CIRCULAR_BUFFER_FCOMPLEX:
    {
        channelBuffer = new FComplexCircularBuffer();
        std::complex<float>* newBuffer = new std::complex<float>[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::TIMED_FCOMPLEX:
    {
        channelBuffer = new TimedCircularBuffer<std::complex<float>>();
        std::complex<float>* newBuffer = new std::complex<float>[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::ENVELOPE_SIGNAL: {
        channelBuffer = new EnvelopeCircularBuffer();
        EnvelopeSignal* newBuffer = new EnvelopeSignal[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::MATRIX_INT:
    {
        channelBuffer = new IntMatrixCircularBuffer;
        IntMatrix* newBuffer = new IntMatrix[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::MATRIX_TIME_INT:
    {
        channelBuffer = new TimedCircularBuffer<IntMatrix>();
        IntMatrix* newBuffer = new IntMatrix[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::MATRIX_DOUBLE:
    {
        channelBuffer = new DoubleMatrixCircularBuffer;
        DoubleMatrix* newBuffer = new DoubleMatrix[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::MATRIX_TIME_DOUBLE:
    {
        channelBuffer = new TimedCircularBuffer<DoubleMatrix>();
        DoubleMatrix* newBuffer = new DoubleMatrix[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::MATRIX_FLOAT:
    {
        channelBuffer = new FloatMatrixCircularBuffer;
        FloatMatrix* newBuffer = new FloatMatrix[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::MATRIX_TIME_FLOAT:
    {
        channelBuffer = new TimedCircularBuffer<FloatMatrix>();
        FloatMatrix* newBuffer = new FloatMatrix[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::MATRIX_BOOL:
    {
        channelBuffer = new BoolMatrixCircularBuffer;
        BoolMatrix* newBuffer = new BoolMatrix[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::MATRIX_TIME_BOOL:
    {
        channelBuffer = new TimedCircularBuffer<BoolMatrix>();
        BoolMatrix* newBuffer = new BoolMatrix[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::MATRIX_FCOMPLEX:
    {
        channelBuffer = new FComplexMatrixCircularBuffer;
        FComplexMatrix* newBuffer = new FComplexMatrix[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::MATRIX_TIME_FCOMPLEX:
    {
        channelBuffer = new TimedCircularBuffer<FComplexMatrix>();
        FComplexMatrix* newBuffer = new FComplexMatrix[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::MATRIX_DCOMPLEX:
    {
        channelBuffer = new DComplexMatrixCircularBuffer();
        DComplexMatrix* newBuffer = new DComplexMatrix[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::MATRIX_TIME_DCOMPLEX:
    {
        channelBuffer = new TimedCircularBuffer<DComplexMatrix>();
        DComplexMatrix* newBuffer = new DComplexMatrix[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    case DataType::MATRIX_ENVELOPE: {
        channelBuffer = new EnvelopeMatrixCircularBuffer;
        EnvelopeMatrix* newBuffer = new EnvelopeMatrix[1024];
        channelBuffer->SetBuffer(newBuffer, 1024, 1);
        channelBuffer->Initialize();
        return channelBuffer;
    }
    default:
        qDebug() << "ERROR: Unsupported data type for CircularBuffer creation: "
                 << static_cast<int>(dataType);
        return nullptr;
    }
    return nullptr;
}

const std::vector<OutPutBusConnection> &Buffer::GetBusConnections() const
{
    return m_busConnections;
}

size_t Buffer::GetBusConnectionCount() const
{
    return m_busConnections.size();
}

void Buffer::AddBusConnection(const OutPutBusConnection &connection)
{
    m_busConnections.push_back(connection);

    // 如果是第一次添加总线连接，标记为总线类型
    if (m_busConnections.size() == 1) {
        m_isBusType = true;
        qDebug() << "Buffer '" << QString::fromStdString(m_name) << "': Marked as bus type";
    }
}

void Buffer::ClearBusConnections()
{
    //清空bus
    m_busConnections.clear();
    m_isBusType = false;
}

void Buffer::SetBusConnectionPermitWrite(size_t connectionIndex, bool permit)
{
    if (connectionIndex < m_busConnections.size()) {
        m_busConnections[connectionIndex].setPermitWrite(permit);
    }
}

bool Buffer::GetBusConnectionPermitWrite(size_t connectionIndex) const
{
    if (connectionIndex < m_busConnections.size()) {
        return m_busConnections[connectionIndex].getPermitWrite();
    }
    return false;
}

size_t Buffer::GetBusBufferFreeSpace() const
{
    // 总线类型：返回最小可用空间
    if (m_busPortBuffers.empty()) {
        return 0;
    }

    size_t minFreeSpace = SIZE_MAX;
    for (const auto& portBuffer : m_busPortBuffers) {
        if (portBuffer) {
            //检查每个bus连接的可用空间
            size_t freeSpace = portBuffer->GetSize() - portBuffer->GetCurrentIndex();
            minFreeSpace = std::min(minFreeSpace, freeSpace);
        }
    }

    return (minFreeSpace == SIZE_MAX) ? 0 : minFreeSpace;
}

bool Buffer::IsBusDownstreamDone() const
{
    // 总线类型：所有下游都完成才算完成
    for (const auto& readerPair : m_readerObjects) {
        if (readerPair.second && !readerPair.second->IsDownstreamDone()) {
            return false;
        }
    }
    return true;
}

bool Buffer::CheckAllBusReaderHaveData(const std::string &readerName, size_t readSize)
{
    std::ignore = readerName;
    if (!IsBusType(m_dataType)) {
        return true; // 非总线类型不需要协调
    }

    // 检查所有连接到总线的读取器是否都有足够数据
    for (const auto& readerPair : m_readerObjects) {
        size_t available = GetAvailableDataForReader(readerPair.first);
        if (available < readSize) {
            return false; // 有读取器数据不足
        }
    }
    return true; // 所有读取器都准备好
}

bool Buffer::IsBusType(Buffer::DataType type) {
    return BufferReader::IsBusType(type);
}

void Buffer::setCharacterizationFrequency(double fc) {
    //设置buffer的表征频率
    m_envelopeFc = fc;
    m_hasEnvelopeFc = true;
//    qDebug() << "Buffer '" << QString::fromStdString(m_name) << "': Set characterization frequency to " << fc;
}

double Buffer::getCharacterizationFrequency() const {
    return m_envelopeFc;
}

bool Buffer::hasCharacterizationFrequency() const {
    //判断是否能传递表征频率
    return m_hasEnvelopeFc;
}

void Buffer::PropagateCharacterizationFrequencyFromInput() {
    //通过端口传递表征频率
    for(auto &readers : m_readerObjects) {
        BufferReader* reader = readers.second;
        if(reader->hasCharacterizationFrequency()) {
            setCharacterizationFrequency(reader->getCharacterizationFrequency());
        }
    }
}

void Buffer::SetExternalIntPort(const std::string &portName, int value)
{
    m_externalIntBasicPorts[portName] = value;
}

std::map<std::string, int> Buffer::GetExternalIntPorts() const
{
    return m_externalIntBasicPorts;
}

std::map<std::string, int> &Buffer::GetExternalIntPortsRef()
{
    return m_externalIntBasicPorts;
}

void Buffer::SetExternalDoublePort(const std::string &portName, double value)
{
    m_externalDoubleBasicPorts[portName] = value;
}

std::map<std::string, double> Buffer::GetExternalDoublePorts() const
{
    return m_externalDoubleBasicPorts;
}

std::map<std::string, double> &Buffer::GetExternalDoublePortsRef()
{
    return m_externalDoubleBasicPorts;
}

void Buffer::SetExternalFloatPort(const std::string &portName, float value)
{
    m_externalFloatBasicPorts[portName] = value;
}

std::map<std::string, float> Buffer::GetExternalFloatPorts() const
{
    return m_externalFloatBasicPorts;
}

std::map<std::string, float> &Buffer::GetExternalFloatPortsRef()
{
    return m_externalFloatBasicPorts;
}

void Buffer::SetExternalBoolPort(const std::string &portName, bool value)
{
    m_externalBoolBasicPorts[portName] = value;
}

std::map<std::string, bool> Buffer::GetExternalBoolPorts() const
{
    return m_externalBoolBasicPorts;
}

std::map<std::string, bool> &Buffer::GetExternalBoolPortsRef()
{
    return m_externalBoolBasicPorts;
}

void Buffer::SetExternalFComplexPort(const std::string &portName, std::complex<float> value)
{
    m_externalFComplexBasicPorts[portName] = value;
}

std::map<std::string, std::complex<float> > Buffer::GetExternalFComplexPorts() const
{
    return m_externalFComplexBasicPorts;
}

std::map<std::string, std::complex<float> > &Buffer::GetExternalFComplexPortsRef()
{
    return m_externalFComplexBasicPorts;
}

void Buffer::SetExternalDComplexPort(const std::string &portName, std::complex<double> value)
{
    m_externalDComplexBasicPorts[portName] = value;
}

std::map<std::string, std::complex<double> > Buffer::GetExternalDComplexPorts() const
{
    return m_externalDComplexBasicPorts;
}

std::map<std::string, std::complex<double> > &Buffer::GetExternalDComplexPortsRef()
{
    return m_externalDComplexBasicPorts;
}



bool Buffer::ExpandBufferForRead(size_t requiredSize, const std::string &readerName)
{
    return m_expansionImpl->ExpandBufferForRead(requiredSize, readerName);
}

bool Buffer::RestoreBufferSize(size_t newSize)
{
    return m_expansionImpl->RestoreBufferSize(newSize);
}

void Buffer::RearrangeBufferAfterRead(const std::string& readerName, size_t readSize)
{
    m_expansionImpl->RearrangeBufferAfterRead(readerName, readSize);
}

bool Buffer::SmartExpandIfNeeded(size_t requiredWriteSize, size_t requiredReadSize)
{
    return m_expansionImpl->SmartExpandIfNeeded(requiredWriteSize, requiredReadSize);
}

void Buffer::AutoRestoreIfPossible()
{
    m_expansionImpl->AutoRestoreIfPossible();
}



SystemVueModelBuilder::IntCircularBuffer* Buffer::getIntCircularBuffer()
{
    // 1. 检查外部缓冲区
    if (m_usingExternalCircularBuffer && m_externalCircularBuffer) {
        auto* intBuffer = dynamic_cast<SystemVueModelBuilder::IntCircularBuffer*>(m_externalCircularBuffer);
        if (intBuffer) {
            return intBuffer;
        } else {
            //获取上游缓冲区时，下游读指针
            //兼容
            //double  time_double
            //bool time_bool
            //float time_float
            if (GetDataType() == DataType::CIRCULAR_BUFFER_BOOL ||
                    GetDataType() == DataType::TIMED_BOOL
                )
            {
                //兼容类型不报警告
                return nullptr;
            }
            LOG_WARN("External CircularBuffer is not IntCircularBuffer type");
            return nullptr;
        }
    }
    // 2. 检查是否是 CircularBuffer
    try {
        return std::get<std::unique_ptr<SystemVueModelBuilder::IntCircularBuffer>>(m_outputBuffer).get();
    } catch (const std::exception& e) {
        // 记录其他异常
        qDebug() << "Unexpected error in getBuffer: " << e.what();
    }

    // 3. 检查是否是 TimedCircularBuffer
    try {
        auto* bufferPtr = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<int>>>(&m_outputBuffer);
        if (bufferPtr && *bufferPtr) {
            qDebug() << "Found TimedCircularBuffer: " << (*bufferPtr).get();
            return reinterpret_cast<SystemVueModelBuilder::IntCircularBuffer*>((*bufferPtr).get());
        }
    } catch (...) {
        qDebug() << "Not a TimedCircularBuffer<double> either";
    }

    // 4. 都不是，返回 nullptr

    qDebug() << "ERROR: Buffer '" << QString::fromStdString(m_name) << "': Cannot get IntCircularBuffer, type mismatch";
    return nullptr;
}

SystemVueModelBuilder::DoubleCircularBuffer* Buffer::getDoubleCircularBuffer()
{
    // 1. 检查外部缓冲区
    if (m_usingExternalCircularBuffer && m_externalCircularBuffer) {
        auto* doubleBuffer = dynamic_cast<SystemVueModelBuilder::DoubleCircularBuffer*>(m_externalCircularBuffer);
        if (doubleBuffer) {
            return doubleBuffer;
        } else {
            //获取上游缓冲区时，下游读指针
            //兼容 int time_int
            if (GetDataType() == DataType::CIRCULAR_BUFFER_INT || GetDataType() == DataType::TIMED_INT)
            {
                //兼容类型不报警告
                //int -> real
                return nullptr;
            }
            LOG_WARN("External CircularBuffer is not DoubleCircularBuffer type");
            return nullptr;
        }
    }
    // 2. 检查是否是 CircularBuffer
    try {
        return std::get<std::unique_ptr<SystemVueModelBuilder::DoubleCircularBuffer>>(m_outputBuffer).get();
    } catch (const std::exception& e) {
        // 记录其他异常
        qDebug() << "Unexpected error in getBuffer: " << e.what();
    }

    // 3. 检查是否是 TimedCircularBuffer
    try {
        auto* bufferPtr = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<double>>>(&m_outputBuffer);
        if (bufferPtr && *bufferPtr) {
            qDebug() << "Found TimedCircularBuffer: " << (*bufferPtr).get();
            return reinterpret_cast<SystemVueModelBuilder::DoubleCircularBuffer*>((*bufferPtr).get());
        }
    } catch (...) {
        qDebug() << "Not a TimedCircularBuffer<double> either";
    }

    // 4. 都不是，返回 nullptr
    qDebug() << "ERROR: Buffer '" << QString::fromStdString(m_name) << "': Cannot get DoubleCircularBuffer, type mismatch";
    return nullptr;
}

SystemVueModelBuilder::FloatCircularBuffer* Buffer::getFloatCircularBuffer()
{
    // 1. 检查外部缓冲区
    if (m_usingExternalCircularBuffer && m_externalCircularBuffer) {
        auto* floatBuffer = dynamic_cast<SystemVueModelBuilder::FloatCircularBuffer*>(m_externalCircularBuffer);
        if (floatBuffer) {
            return floatBuffer;
        } else {
            //获取上游缓冲区时，下游读指针
            //兼容 int time_int
            if (GetDataType() == DataType::CIRCULAR_BUFFER_INT || GetDataType() == DataType::TIMED_INT)
            {
                //兼容类型不报警告
                //int -> real
                return nullptr;
            }
            LOG_WARN("External CircularBuffer is not FloatCircularBuffer type");
            return nullptr;
        }
    }
    // 2. 检查是否是 CircularBuffer
    try {
        return std::get<std::unique_ptr<SystemVueModelBuilder::FloatCircularBuffer>>(m_outputBuffer).get();
    } catch (const std::exception& e) {
        // 记录其他异常
        qDebug() << "Unexpected error in getBuffer: " << e.what();
    }

    // 3. 检查是否是 TimedCircularBuffer
    try {
        auto* bufferPtr = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<float>>>(&m_outputBuffer);
        if (bufferPtr && *bufferPtr) {
            qDebug() << "Found TimedCircularBuffer: " << (*bufferPtr).get();
            return reinterpret_cast<SystemVueModelBuilder::FloatCircularBuffer*>((*bufferPtr).get());
        }
    } catch (...) {
        qDebug() << "Not a TimedCircularBuffer<double> either";
    }

    // 4. 都不是，返回 nullptr
    qDebug() << "ERROR: Buffer '" << QString::fromStdString(m_name) << "': Cannot get FloatCircularBuffer, type mismatch";
    return nullptr;
}

SystemVueModelBuilder::BoolCircularBuffer* Buffer::getBoolCircularBuffer()
{
    // 1. 检查外部缓冲区
    if (m_usingExternalCircularBuffer && m_externalCircularBuffer) {
        auto* boolBuffer = dynamic_cast<SystemVueModelBuilder::BoolCircularBuffer*>(m_externalCircularBuffer);
        if (boolBuffer) {
            return boolBuffer;
        } else {
            //获取上游缓冲区时，下游读指针
            //兼容 int time_int
            if (GetDataType() == DataType::CIRCULAR_BUFFER_INT || GetDataType() == DataType::TIMED_INT)
            {
                //兼容类型不报警告
                return nullptr;
            }
            LOG_WARN("External CircularBuffer is not BoolCircularBuffer type");
            return nullptr;
        }
    }
    // 2. 检查是否是 CircularBuffer
    try {
        return std::get<std::unique_ptr<SystemVueModelBuilder::BoolCircularBuffer>>(m_outputBuffer).get();
    } catch (const std::exception& e) {
        // 记录其他异常
        qDebug() << "Unexpected error in getBuffer: " << e.what();
    }

    // 3. 检查是否是 TimedCircularBuffer
    try {
        auto* bufferPtr = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<bool>>>(&m_outputBuffer);
        if (bufferPtr && *bufferPtr) {
            qDebug() << "Found TimedCircularBuffer: " << (*bufferPtr).get();
            return reinterpret_cast<SystemVueModelBuilder::BoolCircularBuffer*>((*bufferPtr).get());
        }
    } catch (...) {
        qDebug() << "Not a TimedCircularBuffer<double> either";
    }

    // 4. 都不是，返回 nullptr
    qDebug() << "ERROR: Buffer '" << QString::fromStdString(m_name) << "': Cannot get BoolCircularBuffer, type mismatch";
    return nullptr;
}

SystemVueModelBuilder::FComplexCircularBuffer* Buffer::getFComplexCircularBuffer()
{
    // 1. 检查外部缓冲区
    if (m_usingExternalCircularBuffer && m_externalCircularBuffer) {
        auto* fcomplexBuffer = dynamic_cast<SystemVueModelBuilder::FComplexCircularBuffer*>(m_externalCircularBuffer);
        if (fcomplexBuffer) {
            return fcomplexBuffer;
        } else {
            //获取上游缓冲区时，下游读指针
            //兼容
            //int time_int
            //double  time_double
            //float time_float
            if (GetDataType() == DataType::CIRCULAR_BUFFER_INT ||
                GetDataType() == DataType::TIMED_INT ||

                GetDataType() == DataType::CIRCULAR_BUFFER_DOUBLE ||
                GetDataType() == DataType::TIMED_DOUBLE ||

                GetDataType() == DataType::CIRCULAR_BUFFER_FLOAT ||
                GetDataType() == DataType::TIMED_FLOAT
                    )
            {
                //兼容类型不报警告
                //int -> complex
                //real -> complex
                return nullptr;
            }
            LOG_WARN("External CircularBuffer is not FComplexCircularBuffer type");
            return nullptr;
        }
    }
    // 2. 检查是否是 CircularBuffer
    try {
        return std::get<std::unique_ptr<SystemVueModelBuilder::FComplexCircularBuffer>>(m_outputBuffer).get();
    } catch (const std::exception& e) {
        // 记录其他异常
        qDebug() << "Unexpected error in getBuffer: " << e.what();
    }

    // 3. 检查是否是 TimedCircularBuffer
    try {
        auto* bufferPtr = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<std::complex<float>>>>(&m_outputBuffer);
        if (bufferPtr && *bufferPtr) {
            qDebug() << "Found TimedCircularBuffer: " << (*bufferPtr).get();
            return reinterpret_cast<SystemVueModelBuilder::FComplexCircularBuffer*>((*bufferPtr).get());
        }
    } catch (...) {
        qDebug() << "Not a TimedCircularBuffer<double> either";
    }

    // 4. 都不是，返回 nullptr
    qDebug() << "ERROR: Buffer '" << QString::fromStdString(m_name) << "': Cannot get FComplexCircularBuffer, type mismatch";
    return nullptr;
}

SystemVueModelBuilder::DComplexCircularBuffer* Buffer::getDComplexCircularBuffer()
{
    // 1. 检查外部缓冲区
    if (m_usingExternalCircularBuffer && m_externalCircularBuffer) {
        auto* dcomplexBuffer = dynamic_cast<SystemVueModelBuilder::DComplexCircularBuffer*>(m_externalCircularBuffer);
        if (dcomplexBuffer) {
            return dcomplexBuffer;
        } else {
            //获取上游缓冲区时，下游读指针
            //兼容
            //int time_int
            //double  time_double
            //float time_float
            if (GetDataType() == DataType::CIRCULAR_BUFFER_INT ||
                GetDataType() == DataType::TIMED_INT ||

                GetDataType() == DataType::CIRCULAR_BUFFER_DOUBLE ||
                GetDataType() == DataType::TIMED_DOUBLE ||

                GetDataType() == DataType::CIRCULAR_BUFFER_FLOAT ||
                GetDataType() == DataType::TIMED_FLOAT
                    )
            {
                //兼容类型不报警告
                //int -> complex
                //real -> complex
                return nullptr;
            }
            LOG_WARN("External CircularBuffer is not DComplexCircularBuffer type");
            return nullptr;
        }
    }
    // 2. 检查是否是 CircularBuffer
    try {
        return std::get<std::unique_ptr<SystemVueModelBuilder::DComplexCircularBuffer>>(m_outputBuffer).get();
    } catch (const std::exception& e) {
        // 记录其他异常
        qDebug() << "Unexpected error in getBuffer: " << e.what();
    }

    // 3. 检查是否是 TimedCircularBuffer
    try {
        auto* bufferPtr = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>>>>(&m_outputBuffer);
        if (bufferPtr && *bufferPtr) {
            qDebug() << "Found TimedCircularBuffer: " << (*bufferPtr).get();
            return reinterpret_cast<SystemVueModelBuilder::DComplexCircularBuffer*>((*bufferPtr).get());
        }
    } catch (...) {
        qDebug() << "Not a TimedCircularBuffer<double> either";
    }

    // 4. 都不是，返回 nullptr
    qDebug() << "ERROR: Buffer '" << QString::fromStdString(m_name) << "': Cannot get DComplexCircularBuffer, type mismatch";
    return nullptr;
}

SystemVueModelBuilder::EnvelopeCircularBuffer *Buffer::getEnvelopeCircularBuffer()
{
    if (m_usingExternalCircularBuffer && m_externalCircularBuffer) {
            auto* envBuffer = dynamic_cast<SystemVueModelBuilder::EnvelopeCircularBuffer*>(m_externalCircularBuffer);
            if (envBuffer) {
                return envBuffer;
            } else {
                //兼容
                //int time_int
                //double  time_double
                //float time_float
                if (GetDataType() == DataType::CIRCULAR_BUFFER_INT ||
                    GetDataType() == DataType::TIMED_INT ||

                    GetDataType() == DataType::CIRCULAR_BUFFER_DOUBLE ||
                    GetDataType() == DataType::TIMED_DOUBLE ||

                    GetDataType() == DataType::CIRCULAR_BUFFER_FLOAT ||
                    GetDataType() == DataType::TIMED_FLOAT
                        )
                {
                    //兼容类型不报警告
                    //int -> envelope
                    //real -> envelope
                    return nullptr;
                }
                LOG_WARN("External CircularBuffer is not EnvelopeCircularBuffer type");
                return nullptr;
            }
    }
    try {
        auto* bufferPtr = std::get_if<std::unique_ptr<SystemVueModelBuilder::EnvelopeCircularBuffer>>(&m_outputBuffer);
        if (bufferPtr && *bufferPtr) {
            return bufferPtr->get();
        }
    } catch (const std::bad_variant_access&) {
        qDebug() << "ERROR: Cannot get EnvelopeCircularBuffer, type mismatch";
    }
    return nullptr;
}

IntMatrixCircularBuffer *Buffer::getIntMatrixCircularBuffer()
{
    // 1. 检查外部缓冲区
    if (m_usingExternalCircularBuffer && m_externalCircularBuffer) {
        auto* intBuffer = dynamic_cast<SystemVueModelBuilder::IntMatrixCircularBuffer*>(m_externalCircularBuffer);
        if (intBuffer) {
            return intBuffer;
        } else {
            //兼容
            //double_M time_double_M
            //bool_M  time_bool_M
            //float_M time_float_M
            if (
                GetDataType() == DataType::MATRIX_BOOL ||
                GetDataType() == DataType::MATRIX_TIME_BOOL
                    )
            {
                //兼容类型不报警告
                //int_M -> real_M
                //int_M -> bool_M
                return nullptr;
            }
            LOG_WARN("External CircularBuffer is not IntMatrixCircularBuffer type");
            return nullptr;
        }
    }
    // 2. 检查是否是 CircularBuffer
    try {
        return std::get<std::unique_ptr<SystemVueModelBuilder::IntMatrixCircularBuffer>>(m_outputBuffer).get();
    } catch (const std::exception& e) {
        // 记录其他异常
        qDebug() << "Unexpected error in getBuffer: " << e.what();
    }

    // 3. 检查是否是 TimedCircularBuffer
    try {
        auto* bufferPtr = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<Matrix<int>>>>(&m_outputBuffer);
        if (bufferPtr && *bufferPtr) {
            qDebug() << "Found TimedCircularBuffer: " << (*bufferPtr).get();
            return reinterpret_cast<SystemVueModelBuilder::IntMatrixCircularBuffer*>((*bufferPtr).get());
        }
    } catch (...) {
        qDebug() << "Not a TimedCircularBuffer<double> either";
    }

    // 4. 都不是，返回 nullptr
    qDebug() << "ERROR: Buffer '" << QString::fromStdString(m_name) << "': Cannot get IntCircularBuffer, type mismatch";
    return nullptr;
}

DoubleMatrixCircularBuffer *Buffer::getDoubleMatrixCircularBuffer()
{
    // 1. 检查外部缓冲区
    if (m_usingExternalCircularBuffer && m_externalCircularBuffer) {
        auto* intBuffer = dynamic_cast<SystemVueModelBuilder::DoubleMatrixCircularBuffer*>(m_externalCircularBuffer);
        if (intBuffer) {
            return intBuffer;
        } else {
            //兼容
            //int_M time_int_M
            if(GetDataType() == DataType::MATRIX_INT || GetDataType() == DataType::MATRIX_TIME_INT) {
                return nullptr;
            }
            LOG_WARN("External CircularBuffer is not DoubleMatrixCircularBuffer type");
            return nullptr;
        }
    }
    // 2. 检查是否是 CircularBuffer
    try {
        return std::get<std::unique_ptr<SystemVueModelBuilder::DoubleMatrixCircularBuffer>>(m_outputBuffer).get();
    } catch (const std::exception& e) {
        // 记录其他异常
        qDebug() << "Unexpected error in getBuffer: " << e.what();
    }

    // 3. 检查是否是 TimedCircularBuffer
    try {
        auto* bufferPtr = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<Matrix<double>>>>(&m_outputBuffer);
        if (bufferPtr && *bufferPtr) {
            qDebug() << "Found TimedCircularBuffer: " << (*bufferPtr).get();
            return reinterpret_cast<SystemVueModelBuilder::DoubleMatrixCircularBuffer*>((*bufferPtr).get());
        }
    } catch (...) {
        qDebug() << "Not a TimedCircularBuffer<double> either";
    }

    // 4. 都不是，返回 nullptr
    qDebug() << "ERROR: Buffer '" << QString::fromStdString(m_name) << "': Cannot get IntCircularBuffer, type mismatch";
    return nullptr;
}

FloatMatrixCircularBuffer *Buffer::getFloatMatrixCircularBuffer()
{
    // 1. 检查外部缓冲区
    if (m_usingExternalCircularBuffer && m_externalCircularBuffer) {
        auto* intBuffer = dynamic_cast<SystemVueModelBuilder::FloatMatrixCircularBuffer*>(m_externalCircularBuffer);
        if (intBuffer) {
            return intBuffer;
        } else {
            //兼容
            //int_M time_int_M
            if(GetDataType() == DataType::MATRIX_INT || GetDataType() == DataType::MATRIX_TIME_INT) {
                return nullptr;
            }
            LOG_WARN("External CircularBuffer is not FloatMatrixCircularBuffer type");
            return nullptr;
        }
    }
    // 2. 检查是否是 CircularBuffer
    try {
        return std::get<std::unique_ptr<SystemVueModelBuilder::FloatMatrixCircularBuffer>>(m_outputBuffer).get();
    } catch (const std::exception& e) {
        // 记录其他异常
        qDebug() << "Unexpected error in getBuffer: " << e.what();
    }

    // 3. 检查是否是 TimedCircularBuffer
    try {
        auto* bufferPtr = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<Matrix<float>>>>(&m_outputBuffer);
        if (bufferPtr && *bufferPtr) {
            qDebug() << "Found TimedCircularBuffer: " << (*bufferPtr).get();
            return reinterpret_cast<SystemVueModelBuilder::FloatMatrixCircularBuffer*>((*bufferPtr).get());
        }
    } catch (...) {
        qDebug() << "Not a TimedCircularBuffer<double> either";
    }

    // 4. 都不是，返回 nullptr
    qDebug() << "ERROR: Buffer '" << QString::fromStdString(m_name) << "': Cannot get IntCircularBuffer, type mismatch";
    return nullptr;
}

BoolMatrixCircularBuffer *Buffer::getBoolMatrixCircularBuffer()
{
    // 1. 检查外部缓冲区
    if (m_usingExternalCircularBuffer && m_externalCircularBuffer) {
        auto* intBuffer = dynamic_cast<SystemVueModelBuilder::BoolMatrixCircularBuffer*>(m_externalCircularBuffer);
        if (intBuffer) {
            return intBuffer;
        } else {
            //兼容
            //int_M time_int_M
            if(GetDataType() == DataType::MATRIX_INT ||
                    GetDataType() == DataType::MATRIX_TIME_INT) {
                return nullptr;
            }
            LOG_WARN("External CircularBuffer is not BoolMatrixCircularBuffer type");
            return nullptr;
        }
    }
    // 2. 检查是否是 CircularBuffer
    try {
        return std::get<std::unique_ptr<SystemVueModelBuilder::BoolMatrixCircularBuffer>>(m_outputBuffer).get();
    } catch (const std::exception& e) {
        // 记录其他异常
        qDebug() << "Unexpected error in getBuffer: " << e.what();
    }

    // 3. 检查是否是 TimedCircularBuffer
    try {
        auto* bufferPtr = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<Matrix<bool>>>>(&m_outputBuffer);
        if (bufferPtr && *bufferPtr) {
            qDebug() << "Found TimedCircularBuffer: " << (*bufferPtr).get();
            return reinterpret_cast<SystemVueModelBuilder::BoolMatrixCircularBuffer*>((*bufferPtr).get());
        }
    } catch (...) {
        qDebug() << "Not a TimedCircularBuffer<double> either";
    }

    // 4. 都不是，返回 nullptr
    qDebug() << "ERROR: Buffer '" << QString::fromStdString(m_name) << "': Cannot get IntCircularBuffer, type mismatch";
    return nullptr;
}

FComplexMatrixCircularBuffer *Buffer::getFComplexMatrixCircularBuffer()
{
    // 1. 检查外部缓冲区
    if (m_usingExternalCircularBuffer && m_externalCircularBuffer) {
        auto* intBuffer = dynamic_cast<SystemVueModelBuilder::FComplexMatrixCircularBuffer*>(m_externalCircularBuffer);
        if (intBuffer) {
            return intBuffer;
        } else {
            LOG_WARN("External CircularBuffer is not FComplexMatrixCircularBuffer type");
            return nullptr;
        }
    }
    // 2. 检查是否是 CircularBuffer
    try {
        return std::get<std::unique_ptr<SystemVueModelBuilder::FComplexMatrixCircularBuffer>>(m_outputBuffer).get();
    } catch (const std::exception& e) {
        // 记录其他异常
        qDebug() << "Unexpected error in getBuffer: " << e.what();
    }

    // 3. 检查是否是 TimedCircularBuffer
    try {
        auto* bufferPtr = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<Matrix<std::complex<float>>>>>(&m_outputBuffer);
        if (bufferPtr && *bufferPtr) {
            qDebug() << "Found TimedCircularBuffer: " << (*bufferPtr).get();
            return reinterpret_cast<SystemVueModelBuilder::FComplexMatrixCircularBuffer*>((*bufferPtr).get());
        }
    } catch (...) {
        qDebug() << "Not a TimedCircularBuffer<double> either";
    }

    // 4. 都不是，返回 nullptr
    qDebug() << "ERROR: Buffer '" << QString::fromStdString(m_name) << "': Cannot get IntCircularBuffer, type mismatch";
    return nullptr;
}

DComplexMatrixCircularBuffer *Buffer::getDComplexMatrixCircularBuffer()
{
    // 1. 检查外部缓冲区
    if (m_usingExternalCircularBuffer && m_externalCircularBuffer) {
        auto* intBuffer = dynamic_cast<SystemVueModelBuilder::DComplexMatrixCircularBuffer*>(m_externalCircularBuffer);
        if (intBuffer) {
            return intBuffer;
        } else {
            LOG_WARN("External CircularBuffer is not DComplexMatrixCircularBuffer type");
            return nullptr;
        }
    }
    // 2. 检查是否是 CircularBuffer
    try {
        return std::get<std::unique_ptr<SystemVueModelBuilder::DComplexMatrixCircularBuffer>>(m_outputBuffer).get();
    } catch (const std::exception& e) {
        // 记录其他异常
        qDebug() << "Unexpected error in getBuffer: " << e.what();
    }

    // 3. 检查是否是 TimedCircularBuffer
    try {
        auto* bufferPtr = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<Matrix<std::complex<double>>>>>(&m_outputBuffer);
        if (bufferPtr && *bufferPtr) {
            qDebug() << "Found TimedCircularBuffer: " << (*bufferPtr).get();
            return reinterpret_cast<SystemVueModelBuilder::DComplexMatrixCircularBuffer*>((*bufferPtr).get());
        }
    } catch (...) {
        qDebug() << "Not a TimedCircularBuffer<double> either";
    }

    // 4. 都不是，返回 nullptr
    qDebug() << "ERROR: Buffer '" << QString::fromStdString(m_name) << "': Cannot get IntCircularBuffer, type mismatch";
    return nullptr;
}

EnvelopeMatrixCircularBuffer *Buffer::getEnvelopeMatrixCircularBuffer()
{
    if (m_usingExternalCircularBuffer && m_externalCircularBuffer) {
            auto* envBuffer = dynamic_cast<SystemVueModelBuilder::EnvelopeMatrixCircularBuffer*>(m_externalCircularBuffer);
            if (envBuffer) {
                return envBuffer;
            } else {
                LOG_WARN("External CircularBuffer is not EnvelopeMatrixCircularBuffer type");
                return nullptr;
            }
    }
    try {
        auto* bufferPtr = std::get_if<std::unique_ptr<SystemVueModelBuilder::EnvelopeMatrixCircularBuffer>>(&m_outputBuffer);
        if (bufferPtr && *bufferPtr) {
            return bufferPtr->get();
        }
    } catch (const std::bad_variant_access&) {
        qDebug() << "ERROR: Cannot get EnvelopeCircularBuffer, type mismatch";
    }
    return nullptr;
}
