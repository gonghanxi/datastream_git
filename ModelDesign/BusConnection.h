// BusConnection.h
#ifndef BUSCONNECTION_H
#define BUSCONNECTION_H

#include <string>
#include <sstream>

namespace SystemVueModelBuilder {

class Block;
class Buffer;
class BufferReader;

struct BusConnection {
    Block* upstreamBlock = nullptr;          // 上游块
    std::string upstreamPortName;            // 上游端口名称
    BufferReader* bridgeReader = nullptr;     // 桥接读取器
    Buffer* connectedBuffer = nullptr;       // 连接的缓冲区
    bool isUpstreamDone = false;             // 上游是否完成

    //Bus链接的辅助类
    // 默认构造函数
    BusConnection() = default;

    // 带参数的构造函数（实现放在cpp文件中）
    BusConnection(Block* upstream, const std::string& portName,
                 BufferReader* reader, Buffer* buffer);

    // 检查连接是否有效
    bool isValid() const;

    // 获取连接信息
    std::string getInfo() const
    {
        std::stringstream ss;
        ss << "Upstream: ";

        //简化实现
        ss << "Bridge connection";
        return ss.str();
    }
};

struct OutPutBusConnection {
    Block* downstreamBlock = nullptr;          // 下游块
    std::string downstreamPortName;            // 下游端口名称
    Buffer* bridgeWriter = nullptr;     // 桥接写入器
    BufferReader* connectedReader = nullptr;       // 连接的读指针
    bool isDownstreamDone = false;             // 下游是否完成
    bool PermitWrite = false;                    // 允许写入标志位，false时跳过该连接的写入

    //Bus链接的辅助类
    // 默认构造函数
    OutPutBusConnection() = default;

    // 带参数的构造函数（实现放在cpp文件中）
    OutPutBusConnection(Block* downstream, const std::string& portName,
                 BufferReader* reader, Buffer* buffer);

    // 检查连接是否有效
    bool isValid() const;

    // 获取连接信息
    std::string getInfo() const
    {
        std::stringstream ss;
        ss << "Upstream: ";

        //简化实现
        ss << "Bridge connection";
        return ss.str();
    }

    // 设置跳过写入标志
    void setPermitWrite(bool permit) { PermitWrite = permit; }

    // 获取跳过写入标志
    bool getPermitWrite() const { return PermitWrite; }
};

}

#endif // BUSCONNECTION_H
