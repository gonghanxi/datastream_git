#include "BufferBusDataImpl.h"

using namespace SystemVueModelBuilder;
bool SystemVueModelBuilder::BufferBusDataImpl::WriteBusData(const SystemVueModelBuilder::CircularBufferBus &data)
{
    // 1. 提取总线元数据
    BusMetadata metadata;
    if (!ExtractBusMetadata(data, metadata)) {
        qDebug() << "ERROR: Failed to extract bus metadata";
        return false;
    }
    qDebug() << "Bus metadata: ports=" << metadata.portCount
                  << ", totalElements=" << metadata.totalElements
                  << ", elementType=" << static_cast<int>(metadata.elementType);
    // 2. 保存元数据
    m_busMetadata = metadata;

    // 3. 检查是否需要扩容
    if (!m_buffer->SmartExpandIfNeeded(metadata.totalElements, 0)) {
        qDebug() << "ERROR: Cannot expand buffer for bus data";
        return false;
    }
    // 4. 根据元素类型调用相应的写入实现
        bool success = false;
        switch (metadata.elementType) {
        case DataType::INT_BUS:
            success = WriteTypedBusData<int>(data);
            break;
        case DataType::DOUBLE_BUS:
            success = WriteTypedBusData<double>(data);
            break;
        case DataType::FLOAT_BUS:
            success = WriteTypedBusData<float>(data);
            break;
        case DataType::BOOL_BUS:
            success = WriteTypedBusData<bool>(data);
            break;
        case DataType::CHAR_BUS:
            success = WriteTypedBusData<char>(data);
            break;
        case DataType::FCOMPLEX_BUS:
            success = WriteTypedBusData<std::complex<float>>(data);
            break;
        case DataType::DCOMPLEX_BUS:
            success = WriteTypedBusData<std::complex<double>>(data);
            break;
        default:
            qDebug() << "ERROR: Unsupported bus element type: "
                      << static_cast<int>(metadata.elementType);
            return false;
        }

        if (success) {
            qDebug() << "Bus data written successfully. Total written: " << m_buffer->m_totalWritten;
        } else {
            qDebug() << "ERROR: Failed to write bus data";
            return false;
        }

        m_buffer->AutoRestoreIfPossible();
        qDebug() << "=== WriteData(CircularBufferBus) end ===";
        return true;
}

bool SystemVueModelBuilder::BufferBusDataImpl::ReadBusDataForReader(size_t readSize, SystemVueModelBuilder::CircularBufferBus &outputData, const std::string &readerName)
{
    qDebug() << "=== Buffer::ReadDataForReader (Bus) ===";
    qDebug() << "Reader: " << QString::fromStdString(readerName) << ", ReadSize: " << readSize;

    // 总线类型：检查数据协调
    if (IsBusType(m_buffer->m_dataType)) {
        qDebug() << "Checking bus data coordination...";
        if(!m_buffer->CheckAllBusReaderHaveData(readerName, readSize)) {
            qDebug() << "Bus data not ready for all readers";
            return false;
        }
    }

    // 检查是否有总线元数据
    if (!m_busMetadata.isValid()) {
        qDebug() << "ERROR: No valid bus metadata available";
        return false;
    }

    qDebug() << "Bus metadata - Ports: " << m_busMetadata.portCount
              << ", TotalElements: " << m_busMetadata.totalElements;

    // 检查读取器是否注册
    if (m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
        qDebug() << "ERROR: Reader '" << QString::fromStdString(readerName) << "' not registered";
        return false;
    }

    // 计算需要读取的总数据量（元数据 + 实际数据）
    size_t metadataElements = 1 + m_busMetadata.portCount; // 端口数量 + 每个端口大小
    size_t requiredSize = metadataElements + m_busMetadata.totalElements;

    if (!m_buffer->SmartExpandIfNeeded(0, requiredSize)) {
        qDebug() << "ERROR: Cannot expand buffer for bus read operation";
        return false;
    }

    // 根据元素类型调用相应的读取实现
    bool success = false;
    switch (m_busMetadata.elementType) {
    case DataType::INT_BUS:
        success = ReadTypedBusDataForReader<int>(requiredSize, outputData, readerName);
        break;
    case DataType::DOUBLE_BUS:
        success = ReadTypedBusDataForReader<double>(requiredSize, outputData, readerName);
        break;
    case DataType::FLOAT_BUS:
        success = ReadTypedBusDataForReader<float>(requiredSize, outputData, readerName);
        break;
    case DataType::BOOL_BUS:
        success = ReadTypedBusDataForReader<bool>(requiredSize, outputData, readerName);
        break;
    case DataType::FCOMPLEX_BUS:
        success = ReadTypedBusDataForReader<std::complex<float>>(requiredSize, outputData, readerName);
        break;
    case DataType::DCOMPLEX_BUS:
        success = ReadTypedBusDataForReader<std::complex<double>>(requiredSize, outputData, readerName);
        break;
    default:
        qDebug() << "ERROR: Unsupported bus element type: "
                  << static_cast<int>(m_busMetadata.elementType);
        return false;
    }

    m_buffer->AutoRestoreIfPossible();
    return success;
}

const SystemVueModelBuilder::BusMetadata &SystemVueModelBuilder::BufferBusDataImpl::GetBusMetadata() const { return m_busMetadata; }

void SystemVueModelBuilder::BufferBusDataImpl::SetBusMetadata(const SystemVueModelBuilder::BusMetadata &metadata) { m_busMetadata = metadata; }

bool SystemVueModelBuilder::BufferBusDataImpl::ExtractBusMetadata(const SystemVueModelBuilder::CircularBufferBus &bus, SystemVueModelBuilder::BusMetadata &metadata)
{
    metadata.clear();

    // 使用 const_cast 去除 const（不推荐，但作为临时方案）
    SystemVueModelBuilder::CircularBufferBus& nonConstBus = const_cast<SystemVueModelBuilder::CircularBufferBus&>(bus);

    // 获取总线大小（端口数量）
    size_t portCount = bus.GetSize();  // GetSize() 是 const 的
    if (portCount == 0) {
        qDebug() << "ERROR: Bus has no ports";
        return false;
    }

    // 获取第一个端口的数据类型
    SystemVueModelBuilder::CircularBufferBase* firstPort = nonConstBus.Get(0);
    if (!firstPort) {
        qDebug() << "ERROR: First port is null";
        return false;
    }

    // 确定元素类型
    metadata.elementType = GetBusElementType(bus);
    if (metadata.elementType == DataType::ANY) {
        qDebug() << "ERROR: Cannot determine bus element type";
        return false;
    }

    metadata.portCount = portCount;
    metadata.portSizes.reserve(portCount);
    metadata.totalElements = 0;

    // 收集每个端口的数据量
    for (size_t i = 0; i < portCount; i++) {
        SystemVueModelBuilder::CircularBufferBase* port = nonConstBus.Get(i);
        if (port) {
            size_t portSize = port->GetSize();
            metadata.portSizes.push_back(portSize);
            metadata.totalElements += portSize;
        } else {
            metadata.portSizes.push_back(0);
        }
    }

    return true;
}

DataType SystemVueModelBuilder::BufferBusDataImpl::GetBusElementType(const SystemVueModelBuilder::CircularBufferBus &bus) const
{
    //获取桥接读取器所连接的端口的数据类型
    SystemVueModelBuilder::CircularBufferBus& nonConstBus = const_cast<SystemVueModelBuilder::CircularBufferBus&>(bus);
    size_t portCount = nonConstBus.GetSize();
    if (portCount == 0) return DataType::ANY;

    for (size_t i = 0; i < portCount; i++) {
        SystemVueModelBuilder::CircularBufferBase* port = nonConstBus.Get(i);
        if (!port) continue;

        // 动态类型检查
        if (dynamic_cast<SystemVueModelBuilder::IntCircularBuffer*>(port)) {
            return DataType::CIRCULAR_BUFFER_INT;
        } else if (dynamic_cast<SystemVueModelBuilder::DoubleCircularBuffer*>(port)) {
            return DataType::CIRCULAR_BUFFER_DOUBLE;
        } else if (dynamic_cast<SystemVueModelBuilder::FloatCircularBuffer*>(port)) {
            return DataType::CIRCULAR_BUFFER_FLOAT;
        } else if (dynamic_cast<SystemVueModelBuilder::BoolCircularBuffer*>(port)) {
            return DataType::CIRCULAR_BUFFER_BOOL;
        } else if (dynamic_cast<SystemVueModelBuilder::FComplexCircularBuffer*>(port)) {
            return DataType::CIRCULAR_BUFFER_FCOMPLEX;
        } else if (dynamic_cast<SystemVueModelBuilder::DComplexCircularBuffer*>(port)) {
            return DataType::CIRCULAR_BUFFER_DCOMPLEX;
        }
    }

    return DataType::ANY;
}

template<typename T>
bool BufferBusDataImpl::ReadTypedBusDataForReader(size_t readSize, CircularBufferBus &outputData, const std::string &readerName)
{        qDebug() << "=== ReadTypedBusDataForReader<" << typeid(T).name() << "> ===";

         // 获取正确的 CircularBufferBus 类型
         using BusType = SystemVueModelBuilder::CircularBufferBusT<SystemVueModelBuilder::CircularBuffer<T>>;
         BusType* typedBus = dynamic_cast<BusType*>(&outputData);
         if (!typedBus) {
             qDebug() << "ERROR: Output bus type mismatch";
             return false;
         }

         // 获取对应的 CircularBuffer（总线数据存储在普通 CircularBuffer 中）
         SystemVueModelBuilder::CircularBuffer<T>* buffer = nullptr;

         // 根据类型获取相应的缓冲区
         if constexpr (std::is_same_v<T, int>) {
             buffer = m_buffer->getIntCircularBuffer();
         } else if constexpr (std::is_same_v<T, double>) {
             buffer = m_buffer->getDoubleCircularBuffer();
         } else if constexpr (std::is_same_v<T, float>) {
             buffer = m_buffer->getFloatCircularBuffer();
         } else if constexpr (std::is_same_v<T, bool>) {
             buffer = m_buffer->getBoolCircularBuffer();
         } else if constexpr (std::is_same_v<T, std::complex<float>>) {
             buffer = m_buffer->getFComplexCircularBuffer();
         } else if constexpr (std::is_same_v<T, std::complex<double>>) {
             buffer = m_buffer->getDComplexCircularBuffer();
         }

         if (!buffer) {
             qDebug() << "ERROR: Failed to get buffer for type";
             return false;
         }

         //获取读指针位置，以确定有足够数据供读取
         size_t& readerPos = m_buffer->m_readerPositions[readerName];
         size_t available = m_buffer->m_totalWritten - readerPos;

         if (available < readSize) {
             qDebug() << "ERROR: Insufficient data. Available: " << available
                       << ", Needed: " << readSize;
             return false;
         }

         // 1. 读取端口数量（元数据）
         // 注意：总线数据是以 [端口数量][端口1大小][端口2大小]...[数据] 的格式存储的

         size_t readIndex = readerPos % m_buffer->m_bufferSize;
         T portCountValue = (*buffer)[readIndex];
         readerPos++;

         // 将 T 类型转换为 size_t
         size_t portCount = 0;
         if constexpr (std::is_same_v<T, std::complex<float>> || std::is_same_v<T, std::complex<double>>) {
             // 复数类型：取实数部分作为端口数量
             portCount = static_cast<size_t>(portCountValue.real());
         } else {
             // 标量类型：直接转换
             portCount = static_cast<size_t>(portCountValue);
         }

         qDebug() << "Reading bus with " << portCount << " ports";

         if (portCount == 0) {
             qDebug() << "ERROR: Port count is 0";
             return false;
         }

         // 2. 读取每个端口的大小
         std::vector<size_t> portSizes;
         portSizes.reserve(portCount);

         for (size_t i = 0; i < portCount; i++) {
             readIndex = readerPos % m_buffer->m_bufferSize;
             T portSizeValue = (*buffer)[readIndex];

             size_t portSize = 0;
             if constexpr (std::is_same_v<T, std::complex<float>> || std::is_same_v<T, std::complex<double>>) {
                 portSize = static_cast<size_t>(portSizeValue.real());
             } else {
                 portSize = static_cast<size_t>(portSizeValue);
             }

             portSizes.push_back(portSize);
             readerPos++;

             qDebug() << "Port " << i << " size: " << portSize;
         }

         // 3. 初始化输出总线
         typedBus->Initialize(portCount);

         // 4. 为每个端口分配足够大小的缓冲区
         for (size_t portIdx = 0; portIdx < portCount; portIdx++) {
             auto& portBuffer = (*typedBus)[portIdx];
             size_t portSize = portSizes[portIdx];

             // 确保端口缓冲区有足够的大小
             if (portBuffer.GetSize() < portSize) {
                  qDebug() << "WARNING: Port buffer size mismatch. Port " << portIdx
                           << " needs " << portSize << ", has " << portBuffer.GetSize();

                 // 重新分配内存（简化处理，实际使用时应该确保缓冲区大小匹配）
                 portBuffer.DeallocateMemory();
                 void* newMem = portBuffer.AllocateMemory(portSize);
                 if (newMem) {
                     portBuffer.SetBuffer(newMem, portSize);
                 }
             }
         }

         // 5. 读取实际数据到每个端口
         for (size_t portIdx = 0; portIdx < portCount; portIdx++) {
             size_t portSize = portSizes[portIdx];
             auto& portBuffer = (*typedBus)[portIdx];

             for (size_t elemIdx = 0; elemIdx < portSize; elemIdx++) {
                 if (readerPos >= m_buffer->m_totalWritten) {
                     qDebug() << "ERROR: Reached end of data while reading port "
                               << portIdx << " element " << elemIdx;
                     return false;
                 }

                 readIndex = readerPos % m_buffer->m_bufferSize;
                 T value = (*buffer)[readIndex];

                 // 写入到端口缓冲区
                 portBuffer[elemIdx] = value;
                 readerPos++;

                 // 调试输出前几个值
                 if (portIdx < 2 && elemIdx < 3) {
                     if constexpr (std::is_same_v<T, std::complex<float>> || std::is_same_v<T, std::complex<double>>) {
                         qDebug() << "Port " << portIdx << "[" << elemIdx << "] = ("
                                   << value.real() << ", " << value.imag() << "i)";
                     } else {
                         qDebug() << "Port " << portIdx << "[" << elemIdx << "] = " << value;
                     }
                 }
             }
         }

         // 6. 验证读取的数据量
         size_t metadataElements = 1 + portCount; // 端口数量 + 每个端口大小
         size_t dataElements = 0;
         for (size_t size : portSizes) {
             dataElements += size;
         }
         size_t totalExpected = metadataElements + dataElements;
         size_t actualRead = readerPos - m_buffer->m_readerPositions[readerName];

         qDebug() << "Read " << actualRead << " elements (expected: " << totalExpected
                   << ", metadata: " << metadataElements << ", data: " << dataElements << ")";

         if (actualRead != totalExpected) {
             qDebug() << "WARNING: Mismatch in expected vs actual read count";
         }

         // 7. 更新读取器位置（确保不超过总写入量）
         if (readerPos > m_buffer->m_totalWritten) {
             qDebug() << "WARNING: Correcting reader position from " << readerPos
                       << " to " << m_buffer->m_totalWritten;
             readerPos = m_buffer->m_totalWritten;
         }

         // 8. 重新排列缓冲区（只针对非总线类型）
         if (!IsBusType(m_buffer->m_dataType)) {
 //            m_buffer->RearrangeBufferAfterRead(readerName, actualRead);
         } else {
             // 对于总线类型，更新数据计数
             m_buffer->m_dataCount = std::max(m_buffer->m_dataCount, actualRead);
             if (m_buffer->m_dataCount > m_buffer->m_bufferSize) {
                 m_buffer->m_dataCount = m_buffer->m_bufferSize;
             }

             qDebug() << "Bus data reading completed. Reader position: " << readerPos
                       << ", Data count: " << m_buffer->m_dataCount;
         }

         return true;
}
template<typename T>
bool BufferBusDataImpl::WriteTypedBusData(const CircularBufferBus &bus)
{
    qDebug() << "=== WriteTypedBusData<" << typeid(T).name() << "> ===";

    // 获取正确的 CircularBufferBus 类型
    using BusType = SystemVueModelBuilder::CircularBufferBusT<SystemVueModelBuilder::CircularBuffer<T>>;
    const BusType* typedBus = dynamic_cast<const BusType*>(&bus);
    if (!typedBus) {
        qDebug() << "ERROR: Bus type mismatch";
        return false;
    }

    size_t portCount = typedBus->GetSize();
    if (portCount == 0) {
        qDebug() << "ERROR: Bus has no ports";
        return false;
    }

    // 1. 计算总数据量并收集端口大小
    std::vector<size_t> portSizes;
    portSizes.reserve(portCount);
    size_t totalDataElements = 0;

    for (size_t i = 0; i < portCount; i++) {
        auto& portBuffer = (*typedBus)[i];
        size_t portSize = portBuffer.GetSize();
        portSizes.push_back(portSize);
        totalDataElements += portSize;

        qDebug() << "Port " << i << " size: " << portSize;
    }

    // 元数据大小：端口数量 + 每个端口的大小
    size_t metadataElements = 1 + portCount;
    size_t totalElements = metadataElements + totalDataElements;

    qDebug() << "Total elements to write: " << totalElements
              << " (metadata: " << metadataElements
              << ", data: " << totalDataElements << ")";

    // 2. 检查是否需要扩容
    if (!m_buffer->SmartExpandIfNeeded(totalElements, 0)) {
        qDebug() << "ERROR: Cannot expand buffer for bus data";
        return false;
    }

    // 3. 写入元数据
    std::vector<T> allData;
    allData.reserve(totalElements);

    // 写入端口数量
    if constexpr (std::is_same_v<T, std::complex<float>> || std::is_same_v<T, std::complex<double>>) {
        // 复数类型：实数部分存储端口数量，虚数部分为0
        allData.push_back(T(static_cast<typename T::value_type>(portCount), 0));
    } else {
        // 标量类型：直接转换
        allData.push_back(static_cast<T>(portCount));
    }

    // 写入每个端口的大小
    for (size_t portSize : portSizes) {
        if constexpr (std::is_same_v<T, std::complex<float>> || std::is_same_v<T, std::complex<double>>) {
            allData.push_back(T(static_cast<typename T::value_type>(portSize), 0));
        } else {
            allData.push_back(static_cast<T>(portSize));
        }
    }

    // 4. 写入实际数据
    for (size_t portIdx = 0; portIdx < portCount; portIdx++) {
        auto& portBuffer = (*typedBus)[portIdx];
        size_t portSize = portSizes[portIdx];

        for (size_t elemIdx = 0; elemIdx < portSize; elemIdx++) {
            allData.push_back(portBuffer[elemIdx]);
        }
    }

    // 5. 保存总线元数据
    m_busMetadata.portCount = portCount;
    m_busMetadata.portSizes = std::move(portSizes);
    m_busMetadata.elementType = m_buffer->m_dataType;
    m_busMetadata.totalElements = totalElements;

    // 6. 使用相应的 WriteDataImpl 函数写入数据
    bool success = false;
    if constexpr (std::is_same_v<T, int>) {
        std::vector<int> intData(allData.begin(), allData.end());
        m_buffer->WriteData(intData);
        success = true;
    } else if constexpr (std::is_same_v<T, double>) {
        std::vector<double> doubleData(allData.begin(), allData.end());
        m_buffer->WriteData(doubleData);
        success = true;
    } else if constexpr (std::is_same_v<T, float>) {
        std::vector<float> floatData(allData.begin(), allData.end());
        m_buffer->WriteData(floatData);
        success = true;
    } else if constexpr (std::is_same_v<T, bool>) {
        std::vector<bool> boolData(allData.begin(), allData.end());
        m_buffer->WriteData(boolData);
        success = true;
    } else if constexpr (std::is_same_v<T, std::complex<float>>) {
        std::vector<std::complex<float>> complexData(allData.begin(), allData.end());
        m_buffer->WriteData(complexData);
        success = true;
    } else if constexpr (std::is_same_v<T, std::complex<double>>) {
        std::vector<std::complex<double>> complexData(allData.begin(), allData.end());
        m_buffer->WriteData(complexData);
        success = true;
    }

    if (success) {
        qDebug() << "Bus data written successfully. Ports: " << portCount
                  << ", Total elements written: " << allData.size();

        // 验证写入
        qDebug() << "Verification: m_totalWritten = " << m_buffer->m_totalWritten
                  << ", m_dataCount = " << m_buffer->m_dataCount;
    } else {
        qDebug() << "ERROR: Failed to write bus data";
    }

    return success;
}
