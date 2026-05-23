#ifndef BUFFERMEMORYIMPL_H
#define BUFFERMEMORYIMPL_H

#include "Buffer.h"


namespace SystemVueModelBuilder {
class BufferMemoryImpl
{
private:
    Buffer* m_buffer;
public:
    //Buffer初始化，读写时的分配空间的实现类
    explicit BufferMemoryImpl(Buffer* buffer) : m_buffer(buffer) {}

    void UpdateBufferSize(); //更新端口的buffer大小，同样适用于外部端口的buffer
    void ReallocateBufferMemory(); //分配内部端口的buffer大小，即不是模型设置的端口
    void ReallocateExternalBuffer(); //分配外部端口的buffer大小
    // 使用外部缓冲区
    bool SetExternalCircularBuffer(SystemVueModelBuilder::CircularBufferBase* externalBuffer); //设置访问外部端口缓冲区的指针
    void EnsureCircularBuffer(); //确保内部缓冲区的指针能访问到外部缓冲区
    void EnsureTimedCircularBuffer();
    void CreateBufferVariantWithoutAllocation(); //将内部缓冲区的指针初始化
    void WireInternalBufferToExternalMemory(); //将内部缓冲区的指针连接到外部缓冲区的内存上


};
}
#endif // BUFFERMEMORYIMPL_H
