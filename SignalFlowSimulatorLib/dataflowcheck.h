#ifndef DATAFLOWCHECK_H
#define DATAFLOWCHECK_H
#include "algorithmmanager.h"
class DataFlowCheck
{
public:
    DataFlowCheck();
    ~DataFlowCheck();
    //IO连接关系校验
    static bool portPutTypeCheck(const QString& putTypeStart,const QString& putTypeEnd);
    //端口数据类型校验
    static bool portDataTypeCheck(PortMsg::PortDataType dataTypeStart,PortMsg::PortDataType dataTypeEnd);

    //端口类型向下兼容判断 (Complex>Real>Int, 矩阵/多通道同理)
    //当 srcType 可以向下兼容 dstType 时返回 true
    static bool isTypeCompatible(PortMsg::PortDataType srcType, PortMsg::PortDataType dstType);

};

#endif // DATAFLOWCHECK_H
