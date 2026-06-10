#ifndef DATAINTERFACE_H
#define DATAINTERFACE_H

#include <vector>
#include <cstring>
#include <stdexcept>
#include <string>
#include <iostream>
#include <type_traits>
#include <cstdint>

#include <QString>
// 数据类型枚举
enum  CmdType {
    CmdNO,
    CmdReadHead,
    CmdReadParamData,
    CmdReadCode,
    CmdRunCode,
    CmdSendResult,
    CmdSendOver,
    CmdClearShm,
    CmdClearShmComplete,
    CmdRegisterCmp,
    CmdStopServer

};

enum  PutType {
    Put_In,
    Put_Out,
};
//enum  DataType {
//    INT,
//    DOUBLE,
//    BOOL,
//    COMPLEX,
//    COMPLEX_ARRAY,
//    DOUBLE_ARRAY,
//    INT_ARRAY,
//    CHAR_ARRAY
//};

enum DataType {
    INT,
    COMPLEX,
    ANYTYPE,
    ENVELOPE,
    REAL,
    FIXEDPOINT,
    VARIANT,
    MULTIPLE_INT,
    MULTIPLE_COMPLEX,
    MULTIPLE_ANYTYPE,
    MULTIPLE_ENVELOPE,
    MULTIPLE_REAL,
    MULTIPLE_FIXEDPOINT,
    MULTIPLE_VARIANT,
    INT_MATRIX,
    COMPLEX_MATRIX,
    ANYTYPE_MATRIX,
    ENVELOPE_MATRIX,
    REAL_MATRIX,
    FIXEDPOINT_MATRIX,
    VARIANT_MATRIX,
    MULTIPLE_INT_MATRIX,
    MULTIPLE_COMPLEX_MATRIX,
    MULTIPLE_ANYTYPE_MATRIX,
    MULTIPLE_ENVELOPE_MATRIX,
    MULTIPLE_REAL_MATRIX,
    MULTIPLE_FIXEDPOINT_MATRIX,
    MULTIPLE_VARIANT_MATRIX,
    DataTypeCmd
};

const size_t CHAR_SIZE = 30;


struct HeadData{
    CmdType operate;
    char paramName[CHAR_SIZE];   //name
    char shmName[CHAR_SIZE];   //name
    char cmdName[CHAR_SIZE];   //name
    int cmpId {-1};
    int dataType=DataTypeCmd;   //data type
    uint64_t row{1};
    uint64_t col{1};
    uint64_t bytes{0};

    PutType putType;

    void setOperate(CmdType cmdType)
    {
        operate = cmdType;
        switch (cmdType) {
        case CmdType::CmdNO:     {setCmdName("CmdNO"); break;}
        case CmdType::CmdReadHead:     {setCmdName("CmdReadHead"); break;}
        case CmdType::CmdReadParamData:     {setCmdName("CmdReadParamData"); break;}
        case CmdType::CmdReadCode:     {setCmdName("CmdReadCode"); break;}
        case CmdType::CmdRunCode:     {setCmdName("CmdRunCode"); break;}
        case CmdType::CmdSendResult:     {setCmdName("CmdSendResult"); break;}
        case CmdType::CmdSendOver:     {setCmdName("CmdSendOver"); break;}
        case CmdType::CmdClearShm:     {setCmdName("CmdClearShm"); break;}
        case CmdType::CmdClearShmComplete:     {setCmdName("CmdClearShmComplete"); break;}
        case CmdType::CmdRegisterCmp:     {setCmdName("CmdRegisterCmp"); break;}
        case CmdType::CmdStopServer:     {setCmdName("CmdStopServer"); break;}

        default:             return ;
        }

    }

    HeadData()
    {
        paramName[0] = '\0';
    }

    // 3. 拷贝赋值运算符
    HeadData& operator=(const HeadData& other) {
        if (this != &other) {
            setParamName(other.paramName);
            setShmName(other.shmName);
            setCmdName(other.cmdName);
            dataType = other.dataType;
            cmpId = other.cmpId;
            putType = other.putType;
            bytes = other.bytes;

            row = other.row;
            col = other.col;
            operate = other.operate;
        }
        return *this;
    }

    // 安全的参数名设置函数
    void setShmName(const char* shmName_) {
        if (shmName_) {
            strncpy(shmName, shmName_, sizeof(shmName) - 1);
            shmName[sizeof(shmName) - 1] = '\0';  // 确保以null结尾
        } else {
            shmName[0] = '\0';
        }
    }

    // 安全的参数名设置函数
    void setCmdName(const char* cmdName_) {
        if (cmdName_) {
            strncpy(cmdName, cmdName_, sizeof(cmdName) - 1);
            cmdName[sizeof(cmdName) - 1] = '\0';  // 确保以null结尾
        } else {
            cmdName[0] = '\0';
        }
    }

    void setParamName(const char* name) {

//                size_t copy_size = std::min(shmName.size(), sizeof(shmInfo.head .shmName) - 1);
//                std::strncpy(shmInfo.head.shmName, shmName.c_str(), copy_size);
//                shmInfo.head .shmName[copy_size] = '\0';  // 确保以\0结尾

        if (name) {
            strncpy(paramName, name, sizeof(paramName) - 1);
            paramName[sizeof(paramName) - 1] = '\0';  // 确保以null结尾
        } else {
            paramName[0] = '\0';
        }
    }

//    void setParamName(const char* name) {

//    }

    // 获取参数名（返回std::string）
    QString getParamName() const {
//        return std::string(paramName);

        size_t len = strlen(paramName); // 或你知道真实长度

      return QString::fromUtf8(paramName, len);
    }



    // 获取数据总元素数
    uint64_t getTotalElements() const {
//        if (row == 0 || col == 0) return 1;  // 标量
        return row * col;
    }


    uint64_t getBytes() const
    {
        return bytes;
    }

};

struct Param{
    HeadData head;
    std::vector<char> dataBuffer;

};
#include <QHash>
struct ParamInfo{
    QHash<QString, Param>  paramSet;
    void addParamHeadByName(const std::string &paramName)
    {
        HeadData head;
        head.setParamName(paramName.c_str());
        if (paramSet.contains(head.getParamName()))
            return;
        QString pName = head.getParamName();
        paramSet.insert(pName, Param());
        Param& p = paramSet.find(pName).value();
        p.head = head;

    }

    void addParamHead(const HeadData &head)
    {
        if (paramSet.contains(head.getParamName()))
            return;
        QString pName = head.getParamName();
        paramSet.insert(pName, Param());
        Param& p = paramSet.find(pName).value();
        p.head = head;

    }

    std::vector<char> & getData(const QString &pName )
    {
//        std::string pName = head.getParamName();
        auto iter = paramSet.find(pName);
        if (iter==paramSet.end()){
            std::cerr<<"getData failed:"<<pName.toStdString();
            throw std::runtime_error("没有找到参数"+pName.toStdString());
        }
        Param& p = paramSet.find(pName).value();
        return p.dataBuffer;
    }
//       HeadData & getHead(const std::string &pName )
//       {
//          return getParam(QString(pName.c_str())).head;
//       }

    Param & getParam(const QString &pName )
    {
//        std::string pName = head.getParamName();
        auto iter = paramSet.find(pName);
        if (iter==paramSet.end())
            throw std::runtime_error("没有找到参数"+pName.toStdString());
        Param& p = paramSet.find(pName).value();
        return p;
    }
};


#endif // DATAINTERFACE_H
