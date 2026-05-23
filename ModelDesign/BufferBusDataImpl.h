#ifndef BUFFERBUSDATAIMPL_H
#define BUFFERBUSDATAIMPL_H

#include "Buffer.h"
#include "CircularBuffer.h"
#include "DataTypesAndParsers.h"
#include <vector>
#include <memory>
#include <iostream>

namespace SystemVueModelBuilder {
//Bus类型结构体
struct BusMetadata {
    //链接的端口数量
    size_t portCount = 0;
    //链接的端口大小容器
    std::vector<size_t> portSizes;
    //链接的端口的数据类型
    DataType elementType = DataType::INT_BUS;
    size_t totalElements = 0;

    void clear() {
        portCount = 0;
        portSizes.clear();
        elementType = DataType::INT;
        totalElements = 0;
    }

    bool isValid() const { return portCount > 0 && !portSizes.empty(); }
};

class BufferBusDataImpl
{
private:
    Buffer* m_buffer;
    BusMetadata m_busMetadata;
public:
    //Buffer总线读写处理的实现类
    explicit BufferBusDataImpl(Buffer* buffer) : m_buffer(buffer) {}

    // 总线写入
    bool WriteBusData(const SystemVueModelBuilder::CircularBufferBus& data);

    // 总线读取
    bool ReadBusDataForReader(size_t readSize, SystemVueModelBuilder::CircularBufferBus& outputData,
                               const std::string& readerName);

    // 元数据管理
    const BusMetadata& GetBusMetadata() const;
    void SetBusMetadata(const BusMetadata& metadata);
    bool ExtractBusMetadata(const SystemVueModelBuilder::CircularBufferBus& bus, BusMetadata& metadata);
    // 获取总线元素类型
    DataType GetBusElementType(const SystemVueModelBuilder::CircularBufferBus& bus) const;

    // 模板函数
    template<typename T>
    bool WriteTypedBusData(const SystemVueModelBuilder::CircularBufferBus& bus);
    template<typename T>
    bool ReadTypedBusDataForReader(size_t readSize, SystemVueModelBuilder::CircularBufferBus& outputData, const std::string& readerName);

    static SystemVueModelBuilder::CircularBufferBase* CreateCircularBufferByDataType(DataType dataType);
private:
    // 辅助方法
    bool IsBusType(DataType type) const {
        return DataTypesAndParsers::IsBusType(type);
    }
};
}
#endif // BUFFERBUSDATAIMPL_H
