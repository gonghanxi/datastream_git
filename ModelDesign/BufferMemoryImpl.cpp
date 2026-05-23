#include "BufferMemoryImpl.h"
#include "BufferReader.h"
#include "Block.h"

using namespace SystemVueModelBuilder;


void BufferMemoryImpl::UpdateBufferSize()
{
    if (m_buffer->m_readerReadSizes.empty()) {
        return;
    }

    // 计算所有读取器读取大小的最小公倍数
    size_t lcmValue = m_buffer->m_readerReadSizes[0];
    for (size_t i = 1; i < m_buffer->m_readerReadSizes.size(); i++) {
        lcmValue = m_buffer->CalculateLCM(lcmValue, m_buffer->m_readerReadSizes[i]);
    }

    // 重要：设置合理的缓冲区大小限制
    const size_t MAX_BUFFER_SIZE = 1024 * 1024; // 1MB 限制
    size_t newBufferSize = std::min(lcmValue, m_buffer->m_originalBufferSize);
    newBufferSize = std::min(newBufferSize, MAX_BUFFER_SIZE); // 添加上限

    //    qDebug() << "DEBUG: LCM=" << lcmValue << ", NewSize=" << lcmValue;

    // 如果缓冲区大小需要改变
    if (newBufferSize != m_buffer->m_bufferSize) {
        qDebug() << "Buffer '" << QString::fromStdString(m_buffer->m_name) << "': buffer size updated from "
                 << m_buffer->m_bufferSize << " to " << lcmValue
                 << " (LCM of reader sizes: ";
        for (size_t size : m_buffer->m_readerReadSizes) {
            qDebug() << size << " ";
        }
        qDebug() << ")";

        // 保存旧大小
        size_t oldSize = m_buffer->m_bufferSize;
        // 更新大小
        m_buffer->m_bufferSize = lcmValue;
        m_buffer->m_originalBufferSize = m_buffer->m_bufferSize;

        // 重要：只有在需要重新分配时才调用ReallocateBufferMemory
        // 但对于外部缓冲区，我们可能需要重新分配
        qDebug() << (m_buffer->m_usingExternalCircularBuffer ? "true" : "false");
        if (m_buffer->m_usingExternalCircularBuffer) {
            // 外部缓冲区：总是重新分配
            ReallocateBufferMemory();
        } else {
            // 内部缓冲区：只有需要更大内存时才重新分配
            if (lcmValue > oldSize) {
                ReallocateBufferMemory();
            } else {
                qDebug() << "Buffer shrunk, keeping existing memory";
            }
        }
    }
}

void BufferMemoryImpl::ReallocateBufferMemory()
{
    if (m_buffer->m_usingExternalCircularBuffer) {
        //        qDebug() << "Reallocating external buffer...";
        return ReallocateExternalBuffer();
    }

    if (m_buffer->IsBusType(m_buffer->m_dataType)) {
        qDebug() << "WARNING: Cannot reallocate memory for bus type buffer '"
                 << QString::fromStdString(m_buffer->m_name) << "'";
        return;
    }
    try {
        // 根据数据类型分发处理
        switch (m_buffer->m_dataType) {
        case DataType::CIRCULAR_BUFFER_INT:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::IntCircularBuffer>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::CIRCULAR_BUFFER_DOUBLE:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::DoubleCircularBuffer>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::CIRCULAR_BUFFER_FLOAT:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::FloatCircularBuffer>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::CIRCULAR_BUFFER_BOOL:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::BoolCircularBuffer>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::CIRCULAR_BUFFER_DCOMPLEX:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::DComplexCircularBuffer>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::CIRCULAR_BUFFER_FCOMPLEX:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::FComplexCircularBuffer>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::TIMED_INT:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<int>>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::TIMED_BOOL:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<bool>>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::TIMED_FLOAT:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<float>>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::TIMED_DOUBLE:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<double>>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::TIMED_FCOMPLEX:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<std::complex<float>>>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::TIMED_DCOMPLEX:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>>>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::ENVELOPE_SIGNAL:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::EnvelopeCircularBuffer>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::MATRIX_INT:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::IntMatrixCircularBuffer>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::MATRIX_DOUBLE:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::DoubleMatrixCircularBuffer>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::MATRIX_FLOAT:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::FloatMatrixCircularBuffer>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::MATRIX_BOOL:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::BoolMatrixCircularBuffer>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::MATRIX_FCOMPLEX:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::FComplexMatrixCircularBuffer>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::MATRIX_DCOMPLEX:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::DComplexMatrixCircularBuffer>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::MATRIX_ENVELOPE:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::EnvelopeMatrixCircularBuffer>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::MATRIX_TIME_INT:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<IntMatrix>>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::MATRIX_TIME_DOUBLE:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<DoubleMatrix>>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::MATRIX_TIME_FLOAT:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<FloatMatrix>>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::MATRIX_TIME_BOOL:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<BoolMatrix>>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::MATRIX_TIME_FCOMPLEX:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<FComplexMatrix>>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        case DataType::MATRIX_TIME_DCOMPLEX:
            try {
            auto* buffer = std::get_if<std::unique_ptr<SystemVueModelBuilder::TimedCircularBuffer<DComplexMatrix>>>(&m_buffer->m_outputBuffer);
            if (!buffer || !(*buffer)) {
                qDebug() << "ERROR: Buffer not found or null";
                return;
            }

            qDebug() << "Reallocating buffer '" << QString::fromStdString(m_buffer->m_name) << "' to size " << m_buffer->m_bufferSize;

            // 释放旧内存，分配新内存
            (*buffer)->DeallocateMemory();

            // 检查大小是否合理
            if (m_buffer->m_bufferSize == 0 || m_buffer->m_bufferSize > 1024 * 1024 * 100) {
                qDebug() << "ERROR: Invalid buffer size: " << m_buffer->m_bufferSize;
                return;
            }

            void* newMemory = (*buffer)->AllocateMemory(m_buffer->m_bufferSize);
            if (!newMemory) {
                qDebug() << "ERROR: Memory allocation failed for size " << m_buffer->m_bufferSize;
                return;
            }

            (*buffer)->SetBuffer(newMemory, m_buffer->m_bufferSize);
            (*buffer)->Initialize();

            qDebug() << "Buffer reallocation successful";
        }
            catch (const std::bad_variant_access&) {
                qDebug() << "ERROR: Type mismatch in ReallocateImpl";
            }
            break;
        default:
            qDebug() << "WARNING: Unsupported data type for reallocation: "
                     << static_cast<int>(m_buffer->m_dataType);
            break;
        }
    }
    catch (const std::exception& e) {
        qDebug() << "ERROR during reallocation: " << e.what();
    }
}

void BufferMemoryImpl::ReallocateExternalBuffer()
{
    if (!m_buffer->m_externalCircularBuffer) {
        qDebug() << "ERROR: External circular buffer is null";
        return;
    }

//    qDebug() << "\n=== ReallocateExternalBuffer for '" << QString::fromStdString(m_buffer->m_name) << "' ===";
//    qDebug() << "Data type: " << static_cast<int>(m_buffer->m_dataType);
//    qDebug() << "Buffer size: " << m_buffer->m_bufferSize;

    // 保存旧内存
    void* oldMemory = m_buffer->m_externalCircularBuffer->GetBufferMemory();
    if (oldMemory) {
        try {
            // 关键：需要知道实际分配的是什么类型
            // 根据m_buffer->m_dataType来判断
            switch (m_buffer->m_dataType) {
            case DataType::CIRCULAR_BUFFER_INT:
            case DataType::TIMED_INT:
                delete[] static_cast<int*>(oldMemory);
                break;
            case DataType::TIMED_DOUBLE:
            case DataType::CIRCULAR_BUFFER_DOUBLE:
                delete[] static_cast<double*>(oldMemory);
                break;
            case DataType::TIMED_FLOAT:
            case DataType::CIRCULAR_BUFFER_FLOAT:
                delete[] static_cast<float*>(oldMemory);
                break;
            case DataType::TIMED_BOOL:
            case DataType::CIRCULAR_BUFFER_BOOL:
                delete[] static_cast<bool*>(oldMemory);
                break;
            case DataType::CIRCULAR_BUFFER_DCOMPLEX:
            case DataType::TIMED_DCOMPLEX:
                delete[] static_cast<std::complex<double>*>(oldMemory);
                break;
            case DataType::CIRCULAR_BUFFER_FCOMPLEX:
            case DataType::TIMED_FCOMPLEX:
                delete[] static_cast<std::complex<float>*>(oldMemory);
                break;
            case DataType::ENVELOPE_SIGNAL:
                delete[] static_cast<SystemVueModelBuilder::EnvelopeSignal*>(oldMemory);
                break;
            case DataType::MATRIX_INT:
            case DataType::MATRIX_TIME_INT:
                delete[] static_cast<SystemVueModelBuilder::IntMatrix*>(oldMemory);
                break;
            case DataType::MATRIX_DOUBLE:
            case DataType::MATRIX_TIME_DOUBLE:
                delete[] static_cast<SystemVueModelBuilder::DoubleMatrix*>(oldMemory);
                break;
            case DataType::MATRIX_FLOAT:
            case DataType::MATRIX_TIME_FLOAT:
                delete[] static_cast<SystemVueModelBuilder::FloatMatrix*>(oldMemory);
                break;
            case DataType::MATRIX_BOOL:
            case DataType::MATRIX_TIME_BOOL:
                delete[] static_cast<SystemVueModelBuilder::BoolMatrix*>(oldMemory);
                break;
            case DataType::MATRIX_FCOMPLEX:
            case DataType::MATRIX_TIME_FCOMPLEX:
                delete[] static_cast<SystemVueModelBuilder::FComplexMatrix*>(oldMemory);
                break;
            case DataType::MATRIX_DCOMPLEX:
            case DataType::MATRIX_TIME_DCOMPLEX:
                delete[] static_cast<SystemVueModelBuilder::DComplexMatrix*>(oldMemory);
                break;
            case DataType::MATRIX_ENVELOPE:
                delete[] static_cast<SystemVueModelBuilder::EnvelopeMatrix*>(oldMemory);
                break;
            default:
                // 默认按double处理
                delete[] static_cast<double*>(oldMemory);
                break;
            }
        }
        catch (const std::exception& e) {
            qDebug() << "ERROR freeing old memory: " << e.what();
        }
        catch (...) {
            qDebug() << "Unknown error freeing old memory";
        }
    }

    // 分配新内存
    void* newMemory = nullptr;
    try {
        // 根据数据类型分配新内存
        switch (m_buffer->m_dataType) {
        case DataType::TIMED_INT:
        case DataType::CIRCULAR_BUFFER_INT:
            newMemory = new int[m_buffer->m_bufferSize];
            break;
        case DataType::TIMED_DOUBLE:
        case DataType::CIRCULAR_BUFFER_DOUBLE:
            newMemory = new double[m_buffer->m_bufferSize];
            break;
        case DataType::TIMED_FLOAT:
        case DataType::CIRCULAR_BUFFER_FLOAT:
            newMemory = new float[m_buffer->m_bufferSize];
            break;
        case DataType::TIMED_BOOL:
        case DataType::CIRCULAR_BUFFER_BOOL:
            newMemory = new bool[m_buffer->m_bufferSize];
            break;
        case DataType::TIMED_DCOMPLEX:
        case DataType::CIRCULAR_BUFFER_DCOMPLEX:
            newMemory = new std::complex<double>[m_buffer->m_bufferSize];
            break;
        case DataType::TIMED_FCOMPLEX:
        case DataType::CIRCULAR_BUFFER_FCOMPLEX:
            newMemory = new std::complex<float>[m_buffer->m_bufferSize];
            break;
        case DataType::ENVELOPE_SIGNAL:
            newMemory = new SystemVueModelBuilder::EnvelopeSignal[m_buffer->m_bufferSize];
            break;
        case DataType::MATRIX_INT:
        case DataType::MATRIX_TIME_INT:
            newMemory = new SystemVueModelBuilder::IntMatrix[m_buffer->m_bufferSize];
            break;
        case DataType::MATRIX_DOUBLE:
        case DataType::MATRIX_TIME_DOUBLE:
            newMemory = new SystemVueModelBuilder::DoubleMatrix[m_buffer->m_bufferSize];
            break;
        case DataType::MATRIX_FLOAT:
        case DataType::MATRIX_TIME_FLOAT:
            newMemory = new SystemVueModelBuilder::FloatMatrix[m_buffer->m_bufferSize];
            break;
        case DataType::MATRIX_BOOL:
        case DataType::MATRIX_TIME_BOOL:
            newMemory = new SystemVueModelBuilder::BoolMatrix[m_buffer->m_bufferSize];
            break;
        case DataType::MATRIX_FCOMPLEX:
        case DataType::MATRIX_TIME_FCOMPLEX:
            newMemory = new SystemVueModelBuilder::FComplexMatrix[m_buffer->m_bufferSize];
            break;
        case DataType::MATRIX_DCOMPLEX:
        case DataType::MATRIX_TIME_DCOMPLEX:
            newMemory = new SystemVueModelBuilder::DComplexMatrix[m_buffer->m_bufferSize];
            break;
        case DataType::MATRIX_ENVELOPE:
            newMemory = new SystemVueModelBuilder::EnvelopeMatrix[m_buffer->m_bufferSize];
            break;

        default:
            newMemory = new double[m_buffer->m_bufferSize];
            break;
        }
    } catch (const std::bad_alloc& e) {
        qDebug() << "ERROR allocating new memory: " << e.what();
        return;
    }
    // 设置到外部缓冲区
    //    qDebug() << "Setting new memory to external circular buffer...";
    m_buffer->m_externalCircularBuffer->SetBuffer(newMemory, m_buffer->m_bufferSize, 1);
    m_buffer->m_externalCircularBuffer->Initialize();
    // 重新连接内部buffer
    WireInternalBufferToExternalMemory();
    qDebug() << "new buffer memory: " << newMemory;
    qDebug() << "m_externalCircularBuffer Get memory: "
             << m_buffer->m_externalCircularBuffer->GetBufferMemory();
    qDebug() << "Reallocation completed successfully";
}

bool BufferMemoryImpl::SetExternalCircularBuffer(SystemVueModelBuilder::CircularBufferBase* externalBuffer)
{
    if (!externalBuffer) {
        qDebug() << "ERROR: SetExternalCircularBuffer called with null pointer!";
        return false;
    }
    // 检查是否为总线类型
    SystemVueModelBuilder::CircularBufferBus* bus = nullptr;

    if (m_buffer->IsBusType(m_buffer->m_dataType)) {
        // 对于总线类型，直接存储指针
        bus = dynamic_cast<SystemVueModelBuilder::CircularBufferBus*>(externalBuffer);
        m_buffer->m_usingExternalCircularBuffer = true;
        m_buffer->m_externalCircularBuffer = externalBuffer;

        // 更新 variant
        switch (m_buffer->m_dataType) {
        case DataType::INT_BUS:
            m_buffer->m_outputBuffer = static_cast<SystemVueModelBuilder::IntCircularBufferBus*>(bus);
            break;
        case DataType::DOUBLE_BUS:
            m_buffer->m_outputBuffer = static_cast<SystemVueModelBuilder::DoubleCircularBufferBus*>(bus);
            break;
        case DataType::DCOMPLEX_BUS:
            m_buffer->m_outputBuffer = static_cast<SystemVueModelBuilder::DComplexCircularBufferBus*>(bus);
            break;
        case DataType::FLOAT_BUS:
            m_buffer->m_outputBuffer = static_cast<SystemVueModelBuilder::FloatCircularBufferBus*>(bus);
            break;
        case DataType::BOOL_BUS:
            m_buffer->m_outputBuffer = static_cast<SystemVueModelBuilder::BoolCircularBufferBus*>(bus);
            break;
        case DataType::FCOMPLEX_BUS:
            m_buffer->m_outputBuffer = static_cast<SystemVueModelBuilder::FComplexCircularBufferBus*>(bus);
            break;
        default:
            break;
        }

        if(bus != nullptr) {
            qDebug() << "Buffer '" << QString::fromStdString(m_buffer->m_name) << "' set to use external CircularBufferBus with "
                     << bus->GetSize() << " ports";
        }
        return true;
    }
    else {
        if (m_buffer->m_usingExternalCircularBuffer && m_buffer->m_externalCircularBuffer) {
            qDebug() << "WARNING: Buffer '" << QString::fromStdString(m_buffer->m_name) << "' already has an external CircularBuffer";
            return false;
        }

        // 重要：切换到外部模式前，清理可能存在的智能指针
        if (m_buffer->m_allocatedMemory) {
            qDebug() << "Switching to external mode, releasing old smart pointer...";
            m_buffer->m_allocatedMemory.reset();
        }
        m_buffer->m_externalCircularBuffer = externalBuffer;
        m_buffer->m_usingExternalCircularBuffer = true;

        // 设置缓冲区大小
        size_t externalSize = m_buffer->m_externalCircularBuffer->GetSize();
        m_buffer->m_bufferSize = externalSize;
        m_buffer->m_originalBufferSize = externalSize;

        CreateBufferVariantWithoutAllocation();
        WireInternalBufferToExternalMemory();

        return true;
    }
}

void BufferMemoryImpl::EnsureCircularBuffer()
{
    if (m_buffer->m_usingExternalCircularBuffer) {

        // 确保内部variant被创建
        CreateBufferVariantWithoutAllocation();


        // 将内部buffer连接到外部内存
        WireInternalBufferToExternalMemory();
        return;
    }
    // 只有没有外部缓冲区时才创建内部缓冲区
    try {
        qDebug() << "Creating internal circular buffer for type: " << static_cast<int>(m_buffer->m_dataType);

        switch(m_buffer->m_dataType) {
        case DataType::CIRCULAR_BUFFER_INT:
            m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::IntCircularBuffer>();
            break;
        case DataType::CIRCULAR_BUFFER_DOUBLE:
            m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::DoubleCircularBuffer>();
            break;
        case DataType::CIRCULAR_BUFFER_FLOAT:
            m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::FloatCircularBuffer>();
            break;
        case DataType::CIRCULAR_BUFFER_BOOL:
            m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::BoolCircularBuffer>();
            break;
        case DataType::CIRCULAR_BUFFER_FCOMPLEX:
            m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::FComplexCircularBuffer>();
            break;
        case DataType::CIRCULAR_BUFFER_DCOMPLEX:
            m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::DComplexCircularBuffer>();
            break;
        case DataType::ENVELOPE_SIGNAL:
            m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::EnvelopeCircularBuffer>();
            qDebug() << "Created EnvelopeCircularBuffer";
            break;
        default:
            qDebug() << "WARNING: Unknown CircularBuffer type: " << static_cast<int>(m_buffer->m_dataType);
            m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::IntCircularBuffer>();
            break;
        }

        // 分配内存
        size_t bufferSize = m_buffer->m_bufferSize;
        void* memory = std::visit([bufferSize](auto&& buffer) -> void* {
            using T = std::decay_t<decltype(buffer)>;
            if constexpr (!std::is_pointer_v<T>) {
                return buffer->AllocateMemory(bufferSize);
            }
            return nullptr;
        }, m_buffer->m_outputBuffer);

        if (memory) {
            qDebug() << "Internal memory allocated successfully";
        } else {
            qDebug() << "ERROR: Failed to allocate internal memory";
        }

    } catch (const std::exception& e) {
        qDebug() << "ERROR: Exception in EnsureCircularBuffer: " << e.what();
    }
}

void BufferMemoryImpl::EnsureTimedCircularBuffer()
{

    if (m_buffer->m_usingExternalCircularBuffer) {

        // 确保内部variant被创建
        CreateBufferVariantWithoutAllocation();

        // 将内部buffer连接到外部内存
        WireInternalBufferToExternalMemory();
        return;
    }
}

void BufferMemoryImpl::CreateBufferVariantWithoutAllocation()
{
    // 为外部缓冲区使用创建对应的内部缓冲区占位符
    // 这样在访问m_buffer->m_outputBuffer时不会出现空variant错误
    switch (m_buffer->m_dataType) {
    case DataType::CIRCULAR_BUFFER_DOUBLE:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::DoubleCircularBuffer>();
        break;
    case DataType::CIRCULAR_BUFFER_FLOAT:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::FloatCircularBuffer>();
        break;
    case DataType::CIRCULAR_BUFFER_INT:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::IntCircularBuffer>();
        break;
    case DataType::CIRCULAR_BUFFER_BOOL:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::BoolCircularBuffer>();
        break;
    case DataType::CIRCULAR_BUFFER_DCOMPLEX:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::DComplexCircularBuffer>();
        break;
    case DataType::CIRCULAR_BUFFER_FCOMPLEX:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::FComplexCircularBuffer>();
        break;
    case DataType::ENVELOPE_SIGNAL:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::EnvelopeCircularBuffer>();
        break;
    case DataType::TIMED_INT:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::TimedCircularBuffer<int>>();
        break;
    case DataType::TIMED_BOOL:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::TimedCircularBuffer<bool>>();
        break;
    case DataType::TIMED_FLOAT:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::TimedCircularBuffer<float>>();
        break;
    case DataType::TIMED_DOUBLE:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::TimedCircularBuffer<double>>();
        break;
    case DataType::TIMED_DCOMPLEX:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::TimedCircularBuffer<std::complex<double>>>();
        break;
    case DataType::TIMED_FCOMPLEX:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::TimedCircularBuffer<std::complex<float>>>();
        break;
    case DataType::MATRIX_INT:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::IntMatrixCircularBuffer>();
        break;
    case DataType::MATRIX_DOUBLE:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::DoubleMatrixCircularBuffer>();
        break;
    case DataType::MATRIX_FLOAT:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::FloatMatrixCircularBuffer>();
        break;
    case DataType::MATRIX_BOOL:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::BoolMatrixCircularBuffer>();
        break;
    case DataType::MATRIX_FCOMPLEX:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::FComplexMatrixCircularBuffer>();
        break;
    case DataType::MATRIX_DCOMPLEX:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::DComplexMatrixCircularBuffer>();
        break;
    case DataType::MATRIX_ENVELOPE:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::EnvelopeMatrixCircularBuffer>();
        break;
    case DataType::MATRIX_TIME_INT:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::TimedCircularBuffer<IntMatrix>>();
        break;
    case DataType::MATRIX_TIME_DOUBLE:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::TimedCircularBuffer<DoubleMatrix>>();
        break;
    case DataType::MATRIX_TIME_FLOAT:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::TimedCircularBuffer<FloatMatrix>>();
        break;
    case DataType::MATRIX_TIME_BOOL:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::TimedCircularBuffer<BoolMatrix>>();
        break;
    case DataType::MATRIX_TIME_FCOMPLEX:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::TimedCircularBuffer<FComplexMatrix>>();
        break;
    case DataType::MATRIX_TIME_DCOMPLEX:
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::TimedCircularBuffer<DComplexMatrix>>();
        break;
    default:
        // 使用默认类型作为占位符
        m_buffer->m_outputBuffer = std::make_unique<SystemVueModelBuilder::DoubleCircularBuffer>();
        break;
    }
}

void BufferMemoryImpl::WireInternalBufferToExternalMemory()
{
    if (!m_buffer->m_externalCircularBuffer) {
        qDebug() << "ERROR: External circular buffer is null";
        return;
    }

    void* externalMemory = m_buffer->m_externalCircularBuffer->GetBufferMemory();
    size_t externalSize = m_buffer->m_externalCircularBuffer->GetSize();

    // 使用访问者模式设置所有variant的内存
    std::visit([ externalMemory, externalSize](auto&& buffer) {
        using T = std::decay_t<decltype(buffer)>;

        if constexpr (std::is_pointer_v<T>) {
            // 总线类型指针
            qDebug() << "Bus type pointer, skipping wiring";
        } else {
            // 普通缓冲区类型
            if (buffer) {
                // 将内部buffer指向外部内存
                buffer->SetBuffer(externalMemory, externalSize, 1);
                buffer->Initialize();
            } else {
                qDebug() << "ERROR: Internal buffer is null";
            }
        }
    }, m_buffer->m_outputBuffer);
}
