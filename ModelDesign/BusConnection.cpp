// BusConnection.cpp
#include "BusConnection.h"
#include "Block.h"
#include "Buffer.h"
#include "BufferReader.h"
#include <iostream>

namespace SystemVueModelBuilder {

BusConnection::BusConnection(Block* upstream, const std::string& portName,
                           BufferReader* reader, Buffer* buffer)
    : upstreamBlock(upstream), upstreamPortName(portName),
      bridgeReader(reader), connectedBuffer(buffer), isUpstreamDone(false)
{
    //初始化
}

bool BusConnection::isValid() const
{
    //检查是否有效
    return upstreamBlock != nullptr &&
           !upstreamPortName.empty() &&
           bridgeReader != nullptr &&
            connectedBuffer != nullptr;
}

OutPutBusConnection::OutPutBusConnection(Block *downstream,
                                         const std::string &portName,
                                         BufferReader *reader, Buffer *buffer)
    :downstreamBlock(downstream),downstreamPortName(portName),
     bridgeWriter(buffer),connectedReader(reader), isDownstreamDone(false)
{

}

bool OutPutBusConnection::isValid() const
{
    //检查是否有效
    return downstreamBlock != nullptr &&
           !downstreamPortName.empty() &&
           bridgeWriter != nullptr &&
            connectedReader != nullptr;
}

}


