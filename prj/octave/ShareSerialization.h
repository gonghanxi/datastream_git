#ifndef SHARESERIALIZATION_H
#define SHARESERIALIZATION_H


#include "DataInterface.h"
class ShareSerialization
{
public:
    ShareSerialization();

    static void headToBuffer(std::vector<char> &buffer, const HeadData& head)
    {
        buffer.resize(sizeof(HeadData));
        std::memcpy(buffer.data(), &head, sizeof(HeadData));
    }

    static void bufferToHead(const std::vector<char> &buffer,  HeadData& head)
    {
//        buffer.resize(sizeof(HeadData));
        std::memcpy(&head,  buffer.data(), sizeof(HeadData));
    }

    template<typename T>
    static void writeToBuffer(std::vector<char> &buffer, const HeadData& head, const T *data) {

        uint64_t dataBytes = head.getBytes();
        uint64_t totalSize = sizeof(HeadData) + dataBytes;

        buffer.resize(totalSize);

        if (buffer.size()<totalSize)
        {
            throw std::runtime_error("buffer.size()<sizeof(MetaData)+totalSize*sizeof(T)");
        }

        std::memcpy(buffer.data(), &head, sizeof(HeadData));

        char* dataPtr = buffer.data() + sizeof(HeadData);
        std::memcpy(dataPtr, data, dataBytes);
    }

    template<typename T>
    static void paramToData(const Param &param, T *data) {
//       std::cout<<"param.dataBuffer.size() "<<param.dataBuffer.size()<<std::endl;
        uint64_t dataSizeByte = param.dataBuffer.size()*sizeof(char);
        std::memcpy(data, param.dataBuffer.data(), dataSizeByte);
    }

    template<typename T>
    static void dataToParam(const T *data, Param &param) {
        uint64_t dataSizeByte = param.head.getBytes();
        param.dataBuffer.resize(dataSizeByte);
        std::memcpy(param.dataBuffer.data(), data, dataSizeByte);
    }



//    template<typename T>
//    static void readDataFromDataBuffer(const std::vector<char> &dataBuffer, HeadData& head, T *dataOut) {


//        uint64_t dataSize = head.getTotalElements();
//        if (dataBuffer.size()<dataSize*sizeof(T))
//        {
//            throw std::runtime_error("buffer.size()<sizeof(MetaData)+totalSize*sizeof(T)");
//        }
//        std::memcpy(dataOut, dataBuffer.data(), dataSize*sizeof(T));
////        DataType dataType = info.dataType;
//////        int totalSize = info.getTotalElements();
////        switch (dataType) {
////        case DataType::INT:
////        case DataType::BOOL:
////        case DataType::DOUBLE:
////        case DataType::COMPLEX:
////            std::memcpy(dataOut, dataSrcPtr, sizeof(T));
////            break;
////        case DataType::DOUBLE_ARRAY:
////        case DataType::INT_ARRAY:
////        case DataType::CHAR_ARRAY:
////        case DataType::COMPLEX_ARRAY:
////            std::memcpy(dataOut, dataSrcPtr, totalSize*sizeof(T));
////            break;
////        }
//    }


};

#endif // SHARESERIALIZATION_H
