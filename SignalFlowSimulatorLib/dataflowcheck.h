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

};

#endif // DATAFLOWCHECK_H
