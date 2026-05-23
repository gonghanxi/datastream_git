#ifndef BUFFEREXPANSIONIMPL_H
#define BUFFEREXPANSIONIMPL_H

#include "Buffer.h"

// 在全局作用域添加特化（不在任何命名空间内）
template<>
struct circular_buffer_value_type<SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::IntMatrix>> {
    using type = SystemVueModelBuilder::IntMatrix;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::DoubleMatrix>> {
    using type = SystemVueModelBuilder::DoubleMatrix;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::FloatMatrix>> {
    using type = SystemVueModelBuilder::FloatMatrix;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::BoolMatrix>> {
    using type = SystemVueModelBuilder::BoolMatrix;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::FComplexMatrix>> {
    using type = SystemVueModelBuilder::FComplexMatrix;
};

template<>
struct circular_buffer_value_type<SystemVueModelBuilder::TimedCircularBuffer<SystemVueModelBuilder::DComplexMatrix>> {
    using type = SystemVueModelBuilder::DComplexMatrix;
};

namespace SystemVueModelBuilder {
class BufferExpansionImpl
{

private:
    Buffer* m_buffer;
public:
    //Buffer动态扩容的实现类
    explicit BufferExpansionImpl(Buffer* buffer) : m_buffer(buffer) {}

    //扩容用于读取
    bool ExpandBufferForRead(size_t requiredSize, const std::string& readerName);
    //检查扩容后是否需要恢复
    void CheckBuffer();
    //恢复buffer到扩容前的大小
    bool RestoreBufferSize(size_t newSize);
    //读取后检查
    void RearrangeBufferAfterRead(const std::string& readerName, size_t readSize);

    //判断扩容数量
    bool CheckCapacityRequirements(size_t requiredWriteSize, size_t requiredReadSize);
    //判断是否应该扩容
    bool Should_or_not_ExpandBuffer(size_t requiredWriteSize, size_t requiredReadSize) const;
    //动态扩容方法
    bool SmartExpandIfNeeded(size_t requiredWriteSize, size_t requiredReadSize);
    //判断是否需要恢复
    bool ShouldRestoreToOriginalSize() const;
    //自动恢复方法
    void AutoRestoreIfPossible();
    //计算最小需要扩容大小
    size_t CalculateMinimumRequiredSize(size_t requiredWriteSize, size_t requiredReadSize);
    size_t CalculateLCMForAllReaders();
    size_t AlignToPowerOfTwo(size_t size) const;
};

// 辅助函数定义
template<typename CircularBufferType>
using circular_buffer_value_t = typename circular_buffer_value_type<CircularBufferType>::type;
}
#endif // BUFFEREXPANSIONIMPL_H
