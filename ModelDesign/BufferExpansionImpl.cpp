#include "BufferExpansionImpl.h"
#include "BufferReader.h"

using namespace SystemVueModelBuilder;

bool BufferExpansionImpl::ExpandBufferForRead(size_t requiredSize, const std::string &readerName)
{
    if (m_buffer->m_readerPositions.find(readerName) == m_buffer->m_readerPositions.end()) {
        return false;
    }

    size_t& readerPos = m_buffer->m_readerPositions[readerName];
    size_t available = m_buffer->m_totalWritten - readerPos;
    size_t shortage = requiredSize - available;

    // 如果是总线类型，不支持扩展
    if (m_buffer->IsBusType(m_buffer->m_dataType)) {
        qDebug() << "WARNING: ExpandBufferForRead not supported for bus type";
        return false;
    }

    // 计算需要扩容的大小（至少为短缺量的2倍，确保有足够空间）
    size_t expandSize = shortage * 2;
    size_t newBufferSize = m_buffer->m_bufferSize + expandSize;

//    qDebug() << "Buffer '" << QString::fromStdString(m_buffer->m_name) << "': Expanding buffer from "
//              << m_buffer->m_bufferSize << " to " << newBufferSize
//              << " (shortage: " << shortage << ")";

    bool success = std::visit([this, newBufferSize](auto&& bufferPtr) -> bool {
        using T = std::decay_t<decltype(bufferPtr)>;

        // 如果是指针类型（总线），返回false
        if constexpr (std::is_pointer_v<T>) {
            qDebug() << "ERROR: Cannot expand bus buffer";
            return false;
        }
        // 如果是unique_ptr类型（普通缓冲区）
        else if constexpr (
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::IntCircularBuffer>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::DoubleCircularBuffer>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::FloatCircularBuffer>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::BoolCircularBuffer>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::DComplexCircularBuffer>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::FComplexCircularBuffer>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<int>>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<double>>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<float>>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<bool>>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<std::complex<float>>>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>>>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::IntMatrixCircularBuffer>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::DoubleMatrixCircularBuffer>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::FloatMatrixCircularBuffer>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::BoolMatrixCircularBuffer>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::FComplexMatrixCircularBuffer>> ||
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::DComplexMatrixCircularBuffer>>
                ) {

            if (!bufferPtr) {
                qDebug() << "ERROR: Buffer pointer is null";
                return false;
            }

            using BufferType = std::decay_t<decltype(*bufferPtr)>;
//            using ValueType = circular_buffer_value_t<BufferType>;
            using ValueType = circular_buffer_value_t<BufferType>;

            std::vector<ValueType> currentData;
            for (size_t i = 0; i < m_buffer->m_dataCount; i++) {
                size_t index = (m_buffer->m_writePosition - m_buffer->m_dataCount + i) % m_buffer->m_bufferSize;
                currentData.push_back((*bufferPtr)[index]);
            }

            bufferPtr->DeallocateMemory();
            void* newBufferMem = bufferPtr->AllocateMemory(newBufferSize);
            if (!newBufferMem) {
                qDebug() << "Buffer expansion failed: memory allocation error!";
                return false;
            }

            bufferPtr->SetBuffer(newBufferMem, newBufferSize);
            bufferPtr->Initialize();

            m_buffer->m_writePosition = 0;
            for (size_t i = 0; i < currentData.size(); i++) {
                (*bufferPtr)[i] = currentData[i];
                m_buffer->m_writePosition++;
            }

            m_buffer->m_dataCount = currentData.size();
            m_buffer->m_bufferSize = newBufferSize;
            m_buffer->m_isExpanded = true;

            return true;
        }
                // 添加对 EnvelopeCircularBuffer 的专门处理
                else if constexpr (std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::EnvelopeCircularBuffer>>) {
                    if (!bufferPtr) {
                        qDebug() << "ERROR: Buffer pointer is null";
                        return false;
                    }

                    // 获取当前所有数据
                    std::vector<SystemVueModelBuilder::EnvelopeSignal> currentData;
                    for (size_t i = 0; i < m_buffer->m_dataCount; i++) {
                        size_t index = (m_buffer->m_writePosition - m_buffer->m_dataCount + i) % m_buffer->m_bufferSize;
                        currentData.push_back((*bufferPtr)[index]);
                    }

                    // 释放旧内存，分配新内存
                    bufferPtr->DeallocateMemory();
                    void* newBufferMem = bufferPtr->AllocateMemory(newBufferSize);
                    if (!newBufferMem) {
                        qDebug() << "Buffer expansion failed: memory allocation error!";
                        return false;
                    }

                    bufferPtr->SetBuffer(newBufferMem, newBufferSize);
                    bufferPtr->Initialize();

                    // 恢复数据到新缓冲区
                    m_buffer->m_writePosition = 0;
                    for (size_t i = 0; i < currentData.size(); i++) {
                        (*bufferPtr)[i] = currentData[i];
                        m_buffer->m_writePosition++;
                    }

                    m_buffer->m_dataCount = currentData.size();
                    m_buffer->m_bufferSize = newBufferSize;
                    m_buffer->m_isExpanded = true;

                    return true;
                }
                // 添加对 EnvelopeMatrixCircularBuffer 的专门处理
                else if constexpr (std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::EnvelopeMatrixCircularBuffer>>) {
                    if (!bufferPtr) {
                        qDebug() << "ERROR: Buffer pointer is null";
                        return false;
                    }

                    // 获取当前所有数据
                    std::vector<SystemVueModelBuilder::EnvelopeMatrix> currentData;
                    for (size_t i = 0; i < m_buffer->m_dataCount; i++) {
                        size_t index = (m_buffer->m_writePosition - m_buffer->m_dataCount + i) % m_buffer->m_bufferSize;
                        currentData.push_back((*bufferPtr)[index]);
                    }

                    // 释放旧内存，分配新内存
                    bufferPtr->DeallocateMemory();
                    void* newBufferMem = bufferPtr->AllocateMemory(newBufferSize);
                    if (!newBufferMem) {
                        qDebug() << "Buffer expansion failed: memory allocation error!";
                        return false;
                    }

                    bufferPtr->SetBuffer(newBufferMem, newBufferSize);
                    bufferPtr->Initialize();

                    // 恢复数据到新缓冲区
                    m_buffer->m_writePosition = 0;
                    for (size_t i = 0; i < currentData.size(); i++) {
                        (*bufferPtr)[i] = currentData[i];
                        m_buffer->m_writePosition++;
                    }

                    m_buffer->m_dataCount = currentData.size();
                    m_buffer->m_bufferSize = newBufferSize;
                    m_buffer->m_isExpanded = true;

                    return true;
                }
                // 添加对 TimedCircularBuffer<Matrix<T>> 类型的专门处理
                else if constexpr (
                std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<IntMatrix>>> ||
                        std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<DoubleMatrix>>> ||
                        std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<FloatMatrix>>> ||
                        std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<BoolMatrix>>> ||
                        std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<DComplexMatrix>>> ||
                        std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<FComplexMatrix>>>
                        ) {

                    if (!bufferPtr) {
                        qDebug() << "ERROR: Buffer pointer is null";
                        return false;
                    }

                    using BufferType = std::decay_t<decltype(*bufferPtr)>;
//                    using ValueType = typename BufferType::value_type;  // 矩阵类型
                    using ValueType = circular_buffer_value_t<BufferType>;

                    // 获取当前所有数据
                    std::vector<ValueType> currentData;
                    for (size_t i = 0; i < m_buffer->m_dataCount; i++) {
                        size_t index = (m_buffer->m_writePosition - m_buffer->m_dataCount + i) % m_buffer->m_bufferSize;
                        currentData.push_back((*bufferPtr)[index]);
                    }

                    // 释放旧内存，分配新内存
                    bufferPtr->DeallocateMemory();
                    void* newBufferMem = bufferPtr->AllocateMemory(newBufferSize);
                    if (!newBufferMem) {
                        qDebug() << "Buffer expansion failed: memory allocation error!";
                        return false;
                    }

                    bufferPtr->SetBuffer(newBufferMem, newBufferSize);
                    bufferPtr->Initialize();

                    // 恢复数据到新缓冲区
                    m_buffer->m_writePosition = 0;
                    for (size_t i = 0; i < currentData.size(); i++) {
                        (*bufferPtr)[i] = currentData[i];
                        m_buffer->m_writePosition++;
                    }

                    m_buffer->m_dataCount = currentData.size();
                    m_buffer->m_bufferSize = newBufferSize;
                    m_buffer->m_isExpanded = true;

                    return true;
                }
        else {
            // 未知类型
            qDebug() << "ERROR: Unknown buffer type in ExpandBufferForRead";
            return false;
        }
    }, m_buffer->m_outputBuffer);

    if(success) {
        // 更新写指针位置（保持相对位置）
//        size_t oldWriteIndex = m_buffer->m_writePosition;
//        size_t newWriteIndex = oldWriteIndex;  // 计算新的写指针位置

        // 如果缓冲区大小改变了，可能需要调整写指针
        if (newBufferSize != m_buffer->m_bufferSize) {
            // 计算新的写指针位置（保持数据在缓冲区中的位置）
//            size_t dataStart = m_buffer->m_totalWritten - m_buffer->m_dataCount;
            m_buffer->m_writePosition = m_buffer->m_dataCount % newBufferSize;
        }

        for (auto& reader : m_buffer->m_readerPositions) {
            // 保持相对偏移量不变
            size_t relativePos = reader.second - (m_buffer->m_totalWritten - m_buffer->m_dataCount);
            reader.second = relativePos;
        }

        m_buffer->m_totalWritten = m_buffer->m_dataCount;

        qDebug() << "Buffer expansion successful. New size: " << m_buffer->m_bufferSize;
    }

    return success;
}

void BufferExpansionImpl::CheckBuffer()
{
    // 检查是否已扩容
    if (!m_buffer->m_isExpanded) {
        return;  // 未扩容，无需恢复
    }

    // 找到最慢的读取器位置
    size_t slowestReaderPos = m_buffer->FindSlowestReaderPosition();

    // 计算最慢读取器已经读取了多少数据（从扩容点开始）
    if (m_buffer->m_expansionStartPoint == SIZE_MAX) {
        m_buffer->m_expansionStartPoint = slowestReaderPos;  // 记录扩容时的读取点
    }

    // 如果最慢读取器已经读过了扩容点（即读走了导致扩容的数据）
    bool hasReadPastExpansion = (slowestReaderPos > m_buffer->m_expansionStartPoint);

    // 或者检查是否有足够空间恢复
    size_t validDataSize = 0;
    if (m_buffer->m_totalWritten > slowestReaderPos) {
        validDataSize = m_buffer->m_totalWritten - slowestReaderPos;
    }

    // 立即恢复的条件：最慢读取器读走了数据 且 有效数据能放入原始缓冲区
    if (hasReadPastExpansion && validDataSize <= m_buffer->m_originalBufferSize) {
//        qDebug() << "Buffer '" << QString::fromStdString(m_buffer->m_name)<< "': Reader has read past expansion point. "
//                  << "Restoring to original size " << m_buffer->m_originalBufferSize;

        RestoreBufferSize(m_buffer->m_originalBufferSize);
        m_buffer->m_isExpanded = false;
        m_buffer->m_expansionStartPoint = SIZE_MAX;  // 重置扩容点
    }
}

bool BufferExpansionImpl::RestoreBufferSize(size_t newSize)
{
    // 如果是总线类型，不支持恢复大小
    if (m_buffer->IsBusType(m_buffer->m_dataType)) {
        qDebug() << "WARNING: RestoreBufferSize not supported for bus type";
        return true; // 总线不需要改变大小
    }

    if (newSize == m_buffer->m_bufferSize) {
        return true;
    }

    // 边界检查
    if (newSize < m_buffer->m_originalBufferSize) {
        qDebug() << "WARNING: Requested size " << newSize
                  << " is smaller than original size " << m_buffer->m_originalBufferSize
                  << ". Using original size instead.";
        newSize = m_buffer->m_originalBufferSize;
    }

    const size_t MAX_ALLOWED_SIZE = 1024 * 1024 * 100;
    if (newSize > MAX_ALLOWED_SIZE) {
        qDebug() << "ERROR: Buffer size " << newSize
                  << " exceeds limit " << MAX_ALLOWED_SIZE;
        return false;
    }

    try {
        // 使用明确的返回类型
        bool result = std::visit([this, newSize](auto&& bufferPtr) -> bool {
            using T = std::decay_t<decltype(bufferPtr)>;

            // 如果是指针类型（总线），跳过并返回false
            if constexpr (std::is_pointer_v<T>) {
                qDebug() << "ERROR: Cannot resize bus buffer";
                return false;
            }
            // 如果是unique_ptr类型（普通缓冲区）
            else if constexpr (
            std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::IntCircularBuffer>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::DoubleCircularBuffer>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::FloatCircularBuffer>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::BoolCircularBuffer>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::DComplexCircularBuffer>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::FComplexCircularBuffer>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<int>>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<double>>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<float>>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<bool>>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<std::complex<float>>>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>>>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::IntMatrixCircularBuffer>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::DoubleMatrixCircularBuffer>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::FloatMatrixCircularBuffer>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::BoolMatrixCircularBuffer>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::FComplexMatrixCircularBuffer>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::DComplexMatrixCircularBuffer>>
                    ) {

                if (!bufferPtr) {
                    qDebug() << "ERROR: Buffer pointer is null";
                    return false;
                }

                using BufferType = std::decay_t<decltype(*bufferPtr)>;
                using ValueType = circular_buffer_value_t<BufferType>;

                // 1. 获取当前所有有效数据
                size_t slowestReaderPos = m_buffer->FindSlowestReaderPosition();
                size_t validDataCount = 0;

                if (m_buffer->m_totalWritten > slowestReaderPos) {
                    validDataCount = m_buffer->m_totalWritten - slowestReaderPos;
                    validDataCount = std::min(validDataCount, m_buffer->m_bufferSize);
                }

                // 2. 提取有效数据
                std::vector<ValueType> validData;
                validData.reserve(validDataCount);

                for (size_t i = 0; i < validDataCount; i++) {
                    size_t index = (slowestReaderPos + i) % m_buffer->m_bufferSize;
                    validData.push_back((*bufferPtr)[index]);
                }

                // 3. 保存当前所有读取器的相对偏移量
                std::unordered_map<std::string, size_t> readerOffsets;
                for (const auto& readerPair : m_buffer->m_readerPositions) {
                    const std::string& readerName = readerPair.first;
                    size_t readerPos = readerPair.second;

                    // 计算相对于最慢读取器的偏移
                    if (readerPos >= slowestReaderPos) {
                        readerOffsets[readerName] = readerPos - slowestReaderPos;
                    } else {
                        // 如果读取器位置小于最慢读取器，说明该读取器已经读取了超过当前有效数据范围的数据
                        readerOffsets[readerName] = 0;
                        qDebug() << "WARNING: Reader '" << QString::fromStdString(readerName) << "' position " << readerPos
                                  << " is behind slowest reader " << slowestReaderPos
                                  << ". Resetting offset to 0.";
                    }
                }
                // 3. 释放旧内存，分配新内存
                bufferPtr->DeallocateMemory();
                void* newMemory = bufferPtr->AllocateMemory(newSize);
                if (!newMemory) {
                    qDebug() << "ERROR: Failed to allocate " << newSize
                              << " bytes for buffer '" << QString::fromStdString(m_buffer->m_name) << "'";
                    return false;
                }

                bufferPtr->SetBuffer(newMemory, newSize);
                bufferPtr->Initialize();

                // 在重新分配内存后，更新外部缓冲区指针
                if (m_buffer->m_usingExternalCircularBuffer && m_buffer->m_externalCircularBuffer) {
                    // 获取新分配的内存地址
                    void* newMemory = bufferPtr->GetBufferMemory();
                    if (newMemory) {
                        // 更新外部缓冲区的内存指针
                        m_buffer->m_externalCircularBuffer->SetBuffer(newMemory, newSize);
                        qDebug() << "External buffer pointer updated to: " << newMemory;
                    }
                }
                // 4. 恢复数据到新缓冲区
               // 数据应该从新的写位置开始写入
                size_t newWritePos = 0;  // 从0开始，因为数据已经重新整理
                for (size_t i = 0; i < validData.size(); i++) {
                    size_t writeIndex = (newWritePos + i) % newSize;
                    (*bufferPtr)[writeIndex] = validData[i];
                }


                // 5. 更新状态
                m_buffer->m_bufferSize = newSize;
                m_buffer->m_writePosition = validData.size() % newSize;  // 更新写指针位置
                m_buffer->m_dataCount = validData.size();

                // 7. 更新读取器位置到新缓冲区中的正确位置
                for (auto& readerPair : m_buffer->m_readerPositions) {
                    const std::string& readerName = readerPair.first;

                    // 计算新位置：从0开始，加上原有的偏移量
                    if (readerOffsets.find(readerName) != readerOffsets.end()) {
                        size_t newPos = readerOffsets[readerName];

                        // 确保新位置不超过新缓冲区大小
                        if (newPos >= newSize) {
                            qDebug() << "WARNING: Reader '" << QString::fromStdString(readerName)
                                      << "' offset " << newPos
                                      << " exceeds new buffer size " << newSize
                                      << ". Clamping to " << (newSize - 1);
                            newPos = newSize - 1;
                        }

                        readerPair.second = newPos;
                        qDebug() << "Reader '" << QString::fromStdString(readerName) << "' moved from offset "
                                  << readerOffsets[readerName] << " to position " << newPos;
                    }
                }

                // 7. 更新总写入量
                m_buffer->m_totalWritten = slowestReaderPos + validData.size();

                // 8. 更新扩容标志
                m_buffer->m_isExpanded = (newSize > m_buffer->m_originalBufferSize);
                qDebug() << "before Resize: " << m_buffer->m_originalBufferSize
                          << ", after Resize: " << m_buffer->m_bufferSize
                          << " then, renew the originalBufferSize...";
                m_buffer->m_originalBufferSize = m_buffer->m_bufferSize;

                // 10. 通知所有关联的BufferReader缓冲区地址已更新
                for (auto& readerPair : m_buffer->m_readerObjects) {
                    BufferReader* reader = readerPair.second;
                    if (reader) {
                        // 通知BufferReader缓冲区已重新分配
                        reader->OnBufferReallocated();

                        // 重新建立连接，但不重置读取器位置
                        reader->ReconnectToBuffer(m_buffer);
                    }
                }
                return true;
            }
            else if constexpr (std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::EnvelopeCircularBuffer>>) {
                if (!bufferPtr) {
                    qDebug() << "ERROR: Buffer pointer is null";
                    return false;
                }

                // 1. 获取当前所有有效数据
                size_t slowestReaderPos = m_buffer->FindSlowestReaderPosition();
                size_t validDataCount = 0;

                if (m_buffer->m_totalWritten > slowestReaderPos) {
                    validDataCount = m_buffer->m_totalWritten - slowestReaderPos;
                    validDataCount = std::min(validDataCount, m_buffer->m_bufferSize);
                }

                // 2. 提取有效数据
                std::vector<SystemVueModelBuilder::EnvelopeSignal> validData;
                validData.reserve(validDataCount);

                for (size_t i = 0; i < validDataCount; i++) {
                    size_t index = (slowestReaderPos + i) % m_buffer->m_bufferSize;
                    validData.push_back((*bufferPtr)[index]);
                }

                // 保存所有读取器的绝对位置（而不仅仅是偏移）
                std::unordered_map<std::string, size_t> absolutePositions = m_buffer->m_readerPositions;
                // 3. 释放旧内存，分配新内存
                bufferPtr->DeallocateMemory();
                void* newMemory = bufferPtr->AllocateMemory(newSize);
                if (!newMemory) {
                    qDebug() << "ERROR: Failed to allocate " << newSize
                              << " bytes for buffer '" << QString::fromStdString(m_buffer->m_name) << "'";
                    return false;
                }

                bufferPtr->SetBuffer(newMemory, newSize);
                bufferPtr->Initialize();

                // 在重新分配内存后，更新外部缓冲区指针
                if (m_buffer->m_usingExternalCircularBuffer && m_buffer->m_externalCircularBuffer) {
                    // 获取新分配的内存地址
                    void* newMemory = bufferPtr->GetBufferMemory();
                    if (newMemory) {
                        // 更新外部缓冲区的内存指针
                        m_buffer->m_externalCircularBuffer->SetBuffer(newMemory, newSize);
                        m_buffer->m_externalCircularBuffer->Initialize();
                        qDebug() << "External buffer pointer updated to: " << newMemory;
                    }
                }
                // 4. 恢复数据到新缓冲区
                for (size_t i = 0; i < validData.size(); i++) {
                    (*bufferPtr)[i] = validData[i];
                }

                // 5. 更新状态
                m_buffer->m_bufferSize = newSize;
                m_buffer->m_writePosition = validData.size() % newSize;  // 更新写指针位置
                m_buffer->m_dataCount = validData.size();

                // 6. 更新读取器位置
                // 更新读取器位置：保持相对于新起点的位置
                for (auto& readerPair : m_buffer->m_readerPositions) {
                    const std::string& readerName = readerPair.first;
                    size_t originalPos = absolutePositions[readerName];

                    // 计算相对于新起点的偏移
                    if (originalPos >= slowestReaderPos) {
                        size_t offset = originalPos - slowestReaderPos;
                        readerPair.second = offset;  // 在新缓冲区中从0开始
                    } else {
                        // 处理回绕情况
                        readerPair.second = 0;
                    }
                }



                // 7. 更新总写入量
                m_buffer->m_totalWritten = slowestReaderPos + validData.size();

                // 9. 更新扩容标志和原始大小
                m_buffer->m_isExpanded = (newSize > m_buffer->m_originalBufferSize);
                qDebug() << "before Resize: " << m_buffer->m_originalBufferSize
                          << ", after Resize: " << m_buffer->m_bufferSize
                          << "then, renew the originalBufferSize...";
                m_buffer->m_originalBufferSize = m_buffer->m_bufferSize;

                // 10. 通知所有关联的BufferReader缓冲区地址已更新
                qDebug() << "=== Reconnecting all readers after resize ===";
                for (auto& readerPair : m_buffer->m_readerObjects) {
                    const std::string& readerName = readerPair.first;
                    BufferReader* reader = readerPair.second;

                    if (reader) {
                        qDebug() << "Processing reader: '" << QString::fromStdString(readerName) << "' (" << reader << ")";

                        // 检查当前连接状态
                        if (reader->GetConnectedBuffer() != m_buffer) {
                            qDebug() << "Reader is connected to different buffer, reconnecting...";
                            qDebug() << "  Current connection: " << reader->GetConnectedBuffer();
                            qDebug() << "  Should connect to: " << this;
                        }

                        // 重新连接（保持原有位置）
                        reader->ReconnectToBuffer(m_buffer);

                        // 更新读取器在映射中的位置
                        if (m_buffer->m_readerPositions.find(readerName) != m_buffer->m_readerPositions.end()) {
//                            qDebug() << "Reader position: " << m_buffer->m_readerPositions[readerName];
                        }
                    } else {
                        qDebug() << "WARNING: Null reader found for name: '" << QString::fromStdString(readerName) << "'";
                    }
                }

//                qDebug() << "Restore complete:";
//                qDebug() << "  SlowestReaderPos: " << slowestReaderPos;
//                qDebug() << "  ValidDataSize: " << validData.size();
//                qDebug() << "  TotalWritten: " << m_buffer->m_totalWritten;
//                qDebug() << "  WritePosition: " << m_buffer->m_writePosition;

                // 验证数据迁移是否正确
                qDebug() << "=== Data Migration Verification ===";
                qDebug() << "Valid data size: " << validData.size();
                for (size_t i = 0; i < std::min(validData.size(), (size_t)3); i++) {
                    qDebug() << "  Data[" << i << "]: ("
                              << validData[i].real() << ", "
                              << validData[i].imag() << ")";
                }

                // 验证新缓冲区中的数据
                qDebug() << "Data in new buffer:";
                for (size_t i = 0; i < std::min(validData.size(), (size_t)3); i++) {
                    qDebug() << "  NewBuffer[" << i << "]: ("
                              << (*bufferPtr)[i].real() << ", "
                              << (*bufferPtr)[i].imag() << ")";
                }

                return true;
            }
            else if constexpr (std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::EnvelopeMatrixCircularBuffer>>) {
                if (!bufferPtr) {
                    qDebug() << "ERROR: Buffer pointer is null";
                    return false;
                }

                // 1. 获取当前所有有效数据
                size_t slowestReaderPos = m_buffer->FindSlowestReaderPosition();
                size_t validDataCount = 0;

                if (m_buffer->m_totalWritten > slowestReaderPos) {
                    validDataCount = m_buffer->m_totalWritten - slowestReaderPos;
                    validDataCount = std::min(validDataCount, m_buffer->m_bufferSize);
                }

                // 2. 提取有效数据
                std::vector<SystemVueModelBuilder::EnvelopeMatrix> validData;
                validData.reserve(validDataCount);

                for (size_t i = 0; i < validDataCount; i++) {
                    size_t index = (slowestReaderPos + i) % m_buffer->m_bufferSize;
                    validData.push_back((*bufferPtr)[index]);
                }

                // 保存所有读取器的绝对位置（而不仅仅是偏移）
                std::unordered_map<std::string, size_t> absolutePositions = m_buffer->m_readerPositions;
                // 3. 释放旧内存，分配新内存
                bufferPtr->DeallocateMemory();
                void* newMemory = bufferPtr->AllocateMemory(newSize);
                if (!newMemory) {
                    qDebug() << "ERROR: Failed to allocate " << newSize
                              << " bytes for buffer '" << QString::fromStdString(m_buffer->m_name) << "'";
                    return false;
                }

                bufferPtr->SetBuffer(newMemory, newSize);
                bufferPtr->Initialize();

                // 在重新分配内存后，更新外部缓冲区指针
                if (m_buffer->m_usingExternalCircularBuffer && m_buffer->m_externalCircularBuffer) {
                    // 获取新分配的内存地址
                    void* newMemory = bufferPtr->GetBufferMemory();
                    if (newMemory) {
                        // 更新外部缓冲区的内存指针
                        m_buffer->m_externalCircularBuffer->SetBuffer(newMemory, newSize);
                        m_buffer->m_externalCircularBuffer->Initialize();
                        qDebug() << "External buffer pointer updated to: " << newMemory;
                    }
                }
                // 4. 恢复数据到新缓冲区
                for (size_t i = 0; i < validData.size(); i++) {
                    (*bufferPtr)[i] = validData[i];
                }

                // 5. 更新状态
                m_buffer->m_bufferSize = newSize;
                m_buffer->m_writePosition = validData.size() % newSize;  // 更新写指针位置
                m_buffer->m_dataCount = validData.size();

                // 6. 更新读取器位置
                // 更新读取器位置：保持相对于新起点的位置
                for (auto& readerPair : m_buffer->m_readerPositions) {
                    const std::string& readerName = readerPair.first;
                    size_t originalPos = absolutePositions[readerName];

                    // 计算相对于新起点的偏移
                    if (originalPos >= slowestReaderPos) {
                        size_t offset = originalPos - slowestReaderPos;
                        readerPair.second = offset;  // 在新缓冲区中从0开始
                    } else {
                        // 处理回绕情况
                        readerPair.second = 0;
                    }
                }



                // 7. 更新总写入量
                m_buffer->m_totalWritten = slowestReaderPos + validData.size();

                // 9. 更新扩容标志和原始大小
                m_buffer->m_isExpanded = (newSize > m_buffer->m_originalBufferSize);
                qDebug() << "before Resize: " << m_buffer->m_originalBufferSize
                          << ", after Resize: " << m_buffer->m_bufferSize
                          << "then, renew the originalBufferSize...";
                m_buffer->m_originalBufferSize = m_buffer->m_bufferSize;

                // 10. 通知所有关联的BufferReader缓冲区地址已更新
                qDebug() << "=== Reconnecting all readers after resize ===";
                for (auto& readerPair : m_buffer->m_readerObjects) {
                    const std::string& readerName = readerPair.first;
                    BufferReader* reader = readerPair.second;

                    if (reader) {
                        qDebug() << "Processing reader: '" << QString::fromStdString(readerName) << "' (" << reader << ")";

                        // 检查当前连接状态
                        if (reader->GetConnectedBuffer() != m_buffer) {
                            qDebug() << "Reader is connected to different buffer, reconnecting...";
                            qDebug() << "  Current connection: " << reader->GetConnectedBuffer();
                            qDebug() << "  Should connect to: " << this;
                        }

                        // 重新连接（保持原有位置）
                        reader->ReconnectToBuffer(m_buffer);

                        // 更新读取器在映射中的位置
                        if (m_buffer->m_readerPositions.find(readerName) != m_buffer->m_readerPositions.end()) {
//                            qDebug() << "Reader position: " << m_buffer->m_readerPositions[readerName];
                        }
                    } else {
                        qDebug() << "WARNING: Null reader found for name: '" << QString::fromStdString(readerName) << "'";
                    }
                }

//                qDebug() << "Restore complete:";
//                qDebug() << "  SlowestReaderPos: " << slowestReaderPos;
//                qDebug() << "  ValidDataSize: " << validData.size();
//                qDebug() << "  TotalWritten: " << m_buffer->m_totalWritten;
//                qDebug() << "  WritePosition: " << m_buffer->m_writePosition;

                // 验证数据迁移是否正确
                qDebug() << "=== Data Migration Verification ===";
                qDebug() << "Valid data size: " << validData.size();
                return true;
            }
            else if constexpr (
            std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::IntMatrix>>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::DoubleMatrix>>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::FloatMatrix>>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::BoolMatrix>>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::DComplexMatrix>>> ||
                    std::is_same_v<T, std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::FComplexMatrix>>>
                    ) {

                if (!bufferPtr) {
                    qDebug() << "ERROR: Buffer pointer is null";
                    return false;
                }

                using BufferType = std::decay_t<decltype(*bufferPtr)>;
//                using ValueType = typename BufferType::value_type;  // 矩阵类型
                using ValueType = circular_buffer_value_t<BufferType>;

                // 1. 获取当前所有有效数据
                size_t slowestReaderPos = m_buffer->FindSlowestReaderPosition();
                size_t validDataCount = 0;

                if (m_buffer->m_totalWritten > slowestReaderPos) {
                    validDataCount = m_buffer->m_totalWritten - slowestReaderPos;
                    validDataCount = std::min(validDataCount, m_buffer->m_bufferSize);
                }

                // 2. 提取有效数据
                std::vector<ValueType> validData;
                validData.reserve(validDataCount);

                for (size_t i = 0; i < validDataCount; i++) {
                    size_t index = (slowestReaderPos + i) % m_buffer->m_bufferSize;
                    validData.push_back((*bufferPtr)[index]);
                }

                // 保存所有读取器的绝对位置
                std::unordered_map<std::string, size_t> absolutePositions = m_buffer->m_readerPositions;

                // 3. 释放旧内存，分配新内存
                bufferPtr->DeallocateMemory();
                void* newMemory = bufferPtr->AllocateMemory(newSize);
                if (!newMemory) {
                    qDebug() << "ERROR: Failed to allocate " << newSize
                             << " bytes for buffer '" << QString::fromStdString(m_buffer->m_name) << "'";
                    return false;
                }

                bufferPtr->SetBuffer(newMemory, newSize);
                bufferPtr->Initialize();

                // 在重新分配内存后，更新外部缓冲区指针
                if (m_buffer->m_usingExternalCircularBuffer && m_buffer->m_externalCircularBuffer) {
                    void* newMemory = bufferPtr->GetBufferMemory();
                    if (newMemory) {
                        m_buffer->m_externalCircularBuffer->SetBuffer(newMemory, newSize);
                        m_buffer->m_externalCircularBuffer->Initialize();
                        qDebug() << "External buffer pointer updated to: " << newMemory;
                    }
                }

                // 4. 恢复数据到新缓冲区
                for (size_t i = 0; i < validData.size(); i++) {
                    (*bufferPtr)[i] = validData[i];
                }

                // 5. 更新状态
                m_buffer->m_bufferSize = newSize;
                m_buffer->m_writePosition = validData.size() % newSize;
                m_buffer->m_dataCount = validData.size();

                // 6. 更新读取器位置
                for (auto& readerPair : m_buffer->m_readerPositions) {
                    const std::string& readerName = readerPair.first;
                    size_t originalPos = absolutePositions[readerName];

                    if (originalPos >= slowestReaderPos) {
                        size_t offset = originalPos - slowestReaderPos;
                        readerPair.second = offset;
                    } else {
                        readerPair.second = 0;
                    }
                }

                // 7. 更新总写入量
                m_buffer->m_totalWritten = slowestReaderPos + validData.size();

                // 8. 更新扩容标志和原始大小
                m_buffer->m_isExpanded = (newSize > m_buffer->m_originalBufferSize);
                qDebug() << "before Resize: " << m_buffer->m_originalBufferSize
                         << ", after Resize: " << m_buffer->m_bufferSize
                         << " then, renew the originalBufferSize...";
                m_buffer->m_originalBufferSize = m_buffer->m_bufferSize;

                // 9. 通知所有关联的BufferReader缓冲区地址已更新
                qDebug() << "=== Reconnecting all readers after resize ===";
                for (auto& readerPair : m_buffer->m_readerObjects) {
                    const std::string& readerName = readerPair.first;
                    BufferReader* reader = readerPair.second;

                    if (reader) {
                        qDebug() << "Processing reader: '" << QString::fromStdString(readerName) << "' (" << reader << ")";
                        reader->ReconnectToBuffer(m_buffer);
                    } else {
                        qDebug() << "WARNING: Null reader found for name: '" << QString::fromStdString(readerName) << "'";
                    }
                }

                qDebug() << "=== Data Migration Verification ===";
                qDebug() << "Valid data size: " << validData.size();
                return true;
            }
            else {
                // 未知类型
                qDebug() << "ERROR: Unknown buffer type in RestoreBufferSize";
                return false;
            }
        }, m_buffer->m_outputBuffer);

        return result;
    }
    catch (const std::exception& e) {
        qDebug() << "ERROR during buffer resize: " << e.what();
        return false;
    }
}

void BufferExpansionImpl::RearrangeBufferAfterRead(const std::string& readerName, size_t readSize)
{
    std::ignore = readerName;
    if(readSize != 0) {

    }
    if (m_buffer->IsBusType(m_buffer->m_dataType)) {
//            qDebug() << "Buffer '" << QString::fromStdString(m_buffer->m_name)<< "': Skip rearrange for bus type";
            return;
    }
    // 1. 找到最慢的读取器位置
    size_t slowestReaderPosition = m_buffer->FindSlowestReaderPosition();
    // 2. 重要修复：确保总写入量不会小于最慢读取器位置
        if (m_buffer->m_totalWritten < slowestReaderPosition) {
            qDebug() << "WARNING: TotalWritten (" << m_buffer->m_totalWritten
                      << ") < SlowestReaderPos (" << slowestReaderPosition
                      << "). Correcting TotalWritten.";
            m_buffer->m_totalWritten = slowestReaderPosition;
        }

    // 2. 重新计算有效数据量
    // 有效数据 = 总写入量 - 最慢读取器位置
    size_t validDataSize = 0;
    if (m_buffer->m_totalWritten > slowestReaderPosition) {
        validDataSize = m_buffer->m_totalWritten - slowestReaderPosition;
    }

    // 3. 有效数据量不能超过缓冲区大小
    m_buffer->m_dataCount = std::min(validDataSize, m_buffer->m_bufferSize);

    // 4. 更新写入位置（环形缓冲区逻辑）
    if (m_buffer->m_dataCount > 0) {
        m_buffer->m_writePosition = (slowestReaderPosition + m_buffer->m_dataCount) % m_buffer->m_bufferSize;
    } else {
        m_buffer->m_writePosition = m_buffer->m_totalWritten % m_buffer->m_bufferSize;
    }
    CheckBuffer();
}

bool BufferExpansionImpl::CheckCapacityRequirements(size_t requiredWriteSize, size_t requiredReadSize)
{
    // 1. 检查写入容量（是否有足够空闲空间）
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    bool writeCapacityOk = (freeSpace >= requiredWriteSize);

    if (!writeCapacityOk) {
        qDebug() << "Write capacity insufficient: freeSpace=" << freeSpace
                  << ", required=" << requiredWriteSize;

    }

    // 2. 检查读取容量（缓冲区是否足够大以满足所有读取需求）
    //检查缓冲区大小是否能容纳所有读取器的需求

    // 计算最小需要的缓冲区大小
//    size_t slowestReaderPos = m_buffer->FindSlowestReaderPosition();
//    size_t currentDataSize = (m_buffer->m_totalWritten > slowestReaderPos) ?
//                             (m_buffer->m_totalWritten - slowestReaderPos) : 0;

//    size_t neededSize = currentDataSize + requiredWriteSize;

    // 确保缓冲区大小能满足所有读取器的读取大小
    size_t maxReadSize = requiredReadSize;
//    qDebug() << "requiredReadSize: " << requiredReadSize;
    for (size_t readSize : m_buffer->m_readerReadSizes) {
        maxReadSize = std::max(maxReadSize, readSize);
    }

    // 缓冲区大小应该至少能容纳一次最大读取
    bool readCapacityOk = (m_buffer->m_bufferSize >= maxReadSize);

    if (!readCapacityOk) {
        qDebug() << "Read capacity insufficient: bufferSize=" << m_buffer->m_bufferSize
                  << ", maxReadSize=" << maxReadSize;
    }

    return writeCapacityOk && readCapacityOk;
}

bool BufferExpansionImpl::Should_or_not_ExpandBuffer(size_t requiredWriteSize, size_t requiredReadSize) const
{
    size_t maxReadSize = requiredReadSize;
    for (size_t readSize : m_buffer->m_readerReadSizes) {
        maxReadSize = std::max(maxReadSize, readSize);
    }

    // 如果读取需求远大于缓冲区当前数据量，可能不需要扩容
    size_t slowestReaderPos = m_buffer->FindSlowestReaderPosition();
    size_t currentData = m_buffer->m_totalWritten - slowestReaderPos;
    qDebug() << "Should_or_not_ExpandBuffer --requiredWriteSize: " << requiredWriteSize;
    qDebug() << "Should_or_not_ExpandBuffer --slowestReaderPos: " << slowestReaderPos;
    qDebug() << "Should_or_not_ExpandBuffer --currentData: " << currentData;
    qDebug() << "Should_or_not_ExpandBuffer --BufferSize: " << m_buffer->m_bufferSize;


    if (maxReadSize > currentData * 5 && currentData != 0) {  // 读取需求是当前数据的5倍以上
        return false;
    }

    // 正常情况：如果缓冲区太小，需要扩容
    return (m_buffer->m_bufferSize < maxReadSize) || (m_buffer->m_bufferSize < (currentData + requiredWriteSize));
}

bool BufferExpansionImpl::SmartExpandIfNeeded(size_t requiredWriteSize, size_t requiredReadSize)
{
    // ========== 1. 检查容量限制（这是数据量限制，不是内存限制）==========
    if (m_buffer->m_maxSize > 0) {
        size_t currentUsed = m_buffer->GetUsedSpace();
        size_t newUsed = currentUsed + requiredWriteSize;

//        // 如果写入后会超过最大容量，触发背压
//        if (newUsed > m_buffer->m_maxSize) {
//            if (m_buffer->m_backpressureCallback) {
//                m_buffer->m_backpressureCallback(m_buffer, true);
//            }
//            qDebug() << "Buffer [" << QString::fromStdString(m_buffer->m_name)
//                     << "] backpressured: would exceed max size " << m_buffer->m_maxSize;
//            return false;
//        }
        // 如果写入后会超过最大容量，触发背压
        if (newUsed > m_buffer->m_maxBufferSize) {
            if (m_buffer->m_backpressureCallback) {
                m_buffer->m_backpressureCallback(m_buffer, true);
            }
            qDebug() << "Buffer [" << QString::fromStdString(m_buffer->m_name)
                     << "] backpressured: would exceed max buffer size " << m_buffer->m_maxBufferSize;
            return false;
        }
    }

    // ========== 2. 检查当前是否满足需求 ==========
    if (CheckCapacityRequirements(requiredWriteSize, requiredReadSize)) {
        if (m_buffer->m_backpressureCallback) {
            size_t currentUsed = m_buffer->GetUsedSpace();
            if (currentUsed < m_buffer->m_bufferSize * 0.7f) {
                m_buffer->m_backpressureCallback(m_buffer, false);
            }
        }
        return true;
    }

    // ========== 3. 计算扩容大小（不受 m_maxSize 限制）==========
    size_t newSize = m_buffer->m_bufferSize;
    size_t expansionAttempts = 0;
    const size_t MAX_EXPANSION_ATTEMPTS = 5;

    // 计算最小需求
    size_t minRequiredSize = CalculateMinimumRequiredSize(requiredWriteSize, requiredReadSize);

    // ========== 4. 修改：使用专门的缓冲区大小限制 ==========
    // 内存上限应该远大于容量限制
    const size_t MAX_BUFFER_MEMORY = 1024 * 1024 * 100;  // 100MB 硬限制
    size_t maxAllowedSize = m_buffer->m_maxBufferSize > 0 ?
                            m_buffer->m_maxBufferSize :
                            MAX_BUFFER_MEMORY;

    if (minRequiredSize > maxAllowedSize) {
        if (m_buffer->m_backpressureCallback) {
            m_buffer->m_backpressureCallback(m_buffer, true);
        }
        qDebug() << "ERROR: Required size " << minRequiredSize
                  << " exceeds maximum buffer memory " << maxAllowedSize;
        return false;
    }

    // ========== 5. 温和扩容 ==========
    while (expansionAttempts < MAX_EXPANSION_ATTEMPTS) {
        newSize = std::max(newSize * 2, minRequiredSize);

        if (newSize > maxAllowedSize) {
            newSize = maxAllowedSize;
        }

        qDebug() << "Attempting to expand buffer from " << m_buffer->m_bufferSize
                 << " to " << newSize;

        if (RestoreBufferSize(newSize)) {
            if (CheckCapacityRequirements(requiredWriteSize, requiredReadSize)) {
                qDebug() << "Buffer expansion successful: " << m_buffer->m_bufferSize;
                return true;
            }
        } else {
            return false;
        }

        expansionAttempts++;
    }

    return false;
}

bool BufferExpansionImpl::ShouldRestoreToOriginalSize() const
{
    // 只有处于扩容状态时才考虑恢复
    if (!m_buffer->m_isExpanded) {
        return false;
    }

    // 计算当前有效数据量
    size_t slowestReaderPos = m_buffer->FindSlowestReaderPosition();
    size_t currentDataSize = (m_buffer->m_totalWritten > slowestReaderPos) ?
                             (m_buffer->m_totalWritten - slowestReaderPos) : 0;

    // 如果数据量小于原始缓冲区大小的一半，考虑恢复
    bool dataSmallEnough = (currentDataSize <= m_buffer->m_originalBufferSize / 2);

    // 同时检查是否有空闲空间
    size_t freeSpace = m_buffer->GetBufferFreeSpace();
    bool hasEnoughFreeSpace = (freeSpace >= m_buffer->m_originalBufferSize);

    return dataSmallEnough && hasEnoughFreeSpace;
}

void BufferExpansionImpl::AutoRestoreIfPossible()
{
    //判断是否应该恢复
    if (ShouldRestoreToOriginalSize()) {
//        qDebug() << "Buffer '" << QString::fromStdString(m_buffer->m_name)<< "': Auto-restoring to original size "
//                  << m_buffer->m_originalBufferSize;

        //尝试恢复
        if (RestoreBufferSize(m_buffer->m_originalBufferSize)) {
            m_buffer->m_isExpanded = false;
            m_buffer->m_expansionStartPoint = SIZE_MAX;
//            qDebug() << "Buffer restored successfully";
        }
    }
}

size_t BufferExpansionImpl::CalculateMinimumRequiredSize(size_t requiredWriteSize, size_t requiredReadSize)
{
    // 1. 基础容量需求：当前数据量 + 新的写入需求
    size_t slowestReaderPos = m_buffer->FindSlowestReaderPosition();
    size_t currentDataSize = (m_buffer->m_totalWritten > slowestReaderPos) ?
                                 (m_buffer->m_totalWritten - slowestReaderPos) : 0;

    size_t baseRequirement = currentDataSize + requiredWriteSize;

    // 2. 读取器需求：找出所有读取器中最大的读取需求
    size_t maxReaderRequirement = requiredReadSize; // 传入的读取需求

    // 检查所有注册的读取器的读取大小
    for (const auto& readersize : m_buffer->m_readerReadSizes) {
        if (readersize > maxReaderRequirement) {
            maxReaderRequirement = readersize;
        }
    }

    // 3. 计算最小公倍数，确保缓冲区大小是读取大小的整数倍
    //size_t lcmRequirement = CalculateLCMForAllReaders();
    //计算最大读取值
    size_t lcmRequirement = requiredReadSize;
    for (size_t readSize : m_buffer->m_readerReadSizes) {
        lcmRequirement = std::max(lcmRequirement, readSize);
    }

    // 4. 综合考虑所有需求，取最大值
    size_t minimumRequired = std::max({baseRequirement, maxReaderRequirement, lcmRequirement});

    // 5. 添加安全边界（例如10%的额外空间）
    size_t safetyMargin = static_cast<size_t>(minimumRequired * 0.1);
    minimumRequired += safetyMargin;

    // 6. 确保最小容量不小于原始缓冲区大小
    minimumRequired = std::max(minimumRequired, m_buffer->m_originalBufferSize);

    // 7. 确保是2的倍数（便于内存对齐）
    minimumRequired = AlignToPowerOfTwo(minimumRequired);

//    qDebug() << "Buffer '" << QString::fromStdString(m_buffer->m_name)<< "': Minimum required size calculation:";
//    qDebug() << "  - Current data: " << currentDataSize;
//    qDebug() << "  - Write requirement: " << requiredWriteSize;
//    qDebug() << "  - Base requirement: " << baseRequirement;
//    qDebug() << "  - Max reader requirement: " << maxReaderRequirement;
//    qDebug() << "  - LCM requirement: " << lcmRequirement;
//    qDebug() << "  - Final minimum: " << minimumRequired;

    return minimumRequired;
}

size_t BufferExpansionImpl::CalculateLCMForAllReaders()
{
    if (m_buffer->m_readerObjects.empty()) {
        return 1; // 没有读取器时返回1
    }

    if (m_buffer->m_readerReadSizes.empty()) {
        return 1;
    }

    // 计算所有读取器读取大小的最小公倍数
    size_t lcmValue = m_buffer->m_readerReadSizes[0];
    for (size_t i = 1; i < m_buffer->m_readerReadSizes.size(); i++) {
        lcmValue = m_buffer->CalculateLCM(lcmValue, m_buffer->m_readerReadSizes[i]);
    }

    return lcmValue;
}

size_t BufferExpansionImpl::AlignToPowerOfTwo(size_t size) const
{
    if (size == 0) return 1;

    // 找到不小于size的最小的2的幂
    size_t alignedSize = 1;
    while (alignedSize < size) {
        alignedSize <<= 1; // 乘以2
    }

    return alignedSize;
}
