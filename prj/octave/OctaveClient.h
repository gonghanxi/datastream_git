#ifndef OCTAVECLIENT_H
#define OCTAVECLIENT_H


#include "SharedMemory.h"
#include <chrono>
#include <thread>

#include "ShareSerialization.h"
#include "OctaveUtl.h"
class OctaveClient {
private:

//    HeadData ocInfo;
    ShareMInfo shmInfo;
     
    ShareMInfo initServerSharedMemory;

//    void setShmName_sprintf(HeadData& info, int cmpId) {
//        // 使用sprintf拼接字符串
//        int written = sprintf(info.shmName, "/shm_%d", cmpId);

//        if (written < 0) {
//            // 写入失败
//            info.shmName[0] = '\0';
//        }
//    }
public:
    
//    void sendInitInfo(HeadData &info)
//    {
//        std::vector<char> bytes ;
////        bytes.resize(sizeof(HeadData));
//        ShareSerialization::headToBuffer(bytes, info);
//        initServerSharedMemory.write(true, bytes);
//    }

    void removeShmServer()
    {
        HeadData head;
        head.setOperate(CmdClearShm);;
        std::vector<char> clearHeadBuffer;
        ShareSerialization::headToBuffer(clearHeadBuffer, head);
        shmInfo.write(true, clearHeadBuffer);
    }

    void waitRemoveShmServer()
    {
        std::cout<<"waitRemoveShmServer"<<std::endl;
        shmInfo.head.operate=CmdReadHead;
        std::vector<char> headBuffer;
        uint64_t sizeHead= sizeof(HeadData);
        bool isSucc = false;
        while(true)
        {
//            if (shmInfo.getStop())
//            {
//                std::cout<<"client break"<<std::endl;
//                break;
//            }

            if (shmInfo.head.operate==CmdReadHead)
            {
                isSucc = OctaveUtl::readHead(false, shmInfo, headBuffer, sizeHead);
                if (isSucc)
                    std::cout<<"client read Head "<<shmInfo.head.cmdName<<std::endl;
                end(isSucc);
                continue;
            }

            if (shmInfo.head.operate==CmdClearShmComplete)
            {
                std::cout<<"client waitRemoveShmServer break"<<std::endl;
                break;
            }
            end(false);
        }
    }

    ParamInfo &getParamINfo()
    {
        return shmInfo.pSet;
    }
    void sendInfo(ParamInfo &pSet, std::string &code)
    {
        std::vector<char> headBuffer ;
//        headBuffer.resize(sizeof(HeadData));
//        bool isWrite = false;
        for (auto it = pSet.paramSet.begin(); it != pSet.paramSet.end(); ++it) {
            const Param& p = it.value();

            ShareSerialization::headToBuffer(headBuffer, p.head);
            shmInfo.write(true, headBuffer);
            shmInfo.write(true, p.dataBuffer);
        }

//        while(1){};
//        return ;

        HeadData head;
        head.setOperate(CmdReadCode);

//        headBuffer.resize(sizeof(HeadData));

        std::vector<char> codeBuffer;
        codeBuffer.resize(code.size());
        codeBuffer.assign(code.begin(), code.end());
        head.bytes = codeBuffer.size();

        ShareSerialization::headToBuffer(headBuffer, head);
        shmInfo.write(true, headBuffer);

        shmInfo.write(true, codeBuffer);
        head.setOperate(CmdRunCode);
        head.dataType = DataTypeCmd;

//        headBuffer.resize(sizeof(HeadData));
        ShareSerialization::headToBuffer(headBuffer, head);
        shmInfo.write(true, headBuffer);

        receive_results();

//        shmInfo.write(dataBuffer);
    }

//    void sendData(int cmpId)
//    {
//        HeadData info;
//        info.cmpId = cmpId;
//        info.operate = CmdReadData;
//        info.col = 1;
//        info.setParamName("kj");


//        std::vector<char> dataBuffer;
//        dataBuffer.push_back('i');
//        dataBuffer.push_back('j');
//        dataBuffer.push_back('k');
//        dataBuffer.push_back('y');
//        dataBuffer.push_back('w');
//         dataBuffer.push_back('\0');

//         info.row = dataBuffer.size();


//        std::vector<char> headBuffer ;
//        headBuffer.resize(sizeof(HeadData));
//        ShareSerialization::headToBuffer(headBuffer, info);
//        shmInfo.write(headBuffer);
//        shmInfo.write(dataBuffer);
//    }
    
    OctaveClient(int cmpId) {

        std::cout <<"xxxxx1"<<std::endl;
        initServerSharedMemory.setShm_name("/init_server_shmxx");
        std::cout <<"xxxxx2"<<std::endl;
        initServerSharedMemory.mapShare();
        std::cout <<"xxxxx23"<<std::endl;

        initServerSharedMemory.reset();

        std::cout <<"xxxxx22"<<std::endl;
        std::string shmName = "/shm_"+std::to_string(cmpId);
//        const char * shmName = "/shm_"+;

        shmInfo.head.setShmName(shmName.c_str());
        // 复制到字符数组
//        size_t copy_size = std::min(shmName.size(), sizeof(shmInfo.head .shmName) - 1);
//        std::strncpy(shmInfo.head.shmName, shmName.c_str(), copy_size);
//        shmInfo.head .shmName[copy_size] = '\0';  // 确保以\0结尾

//        // 如果被截断，可以记录日志
//        if (shmName.size() >= sizeof(shmInfo.head.shmName)) {
//            throw std::runtime_error("截断");
//            // 处理截断情况
//        }

        shmInfo.setShm_name(shmName);
        shmInfo.createShare();

        std::cout <<"xxxxx3"<<std::endl;


//        shmInfo.head.setOperate(CmdRegisterCmp);
        shmInfo.head.cmpId = cmpId;

        HeadData head;
        head.setShmName(shmName.c_str());
        head.cmpId = cmpId;
        std::vector<char> bytes ;
        head.setOperate(CmdReadHead);
//        bytes.resize(sizeof(HeadData));
        ShareSerialization::headToBuffer(bytes, head);
        initServerSharedMemory.write(true, bytes);

//        sendInitInfo(shmInfo.head);
    }

    void end(bool isSucc)
    {
        if (!isSucc)
            std::this_thread::yield();
    }

    void receive_results()
    {
        std::vector<char> headBuffer;
        uint64_t sizeHead= sizeof(HeadData);
        bool isSucc = false;
        while(true)
        {

            if (shmInfo.head.operate==CmdReadHead)
            {

                isSucc = OctaveUtl::readHead(false, shmInfo, headBuffer, sizeHead);
                if (isSucc)
                    std::cout<<"client read Head "<<shmInfo.head.cmdName<<std::endl;
                end(isSucc);
                continue;
            }

            if (shmInfo.head.operate==CmdReadParamData)
            {

                 isSucc = OctaveUtl::readParamData(false, shmInfo);
                 if (isSucc)
                     std::cout<<"client read data "<<shmInfo.head.paramName<<std::endl;
                 end(isSucc);
                 continue;
            }

            if (shmInfo.head.operate == CmdSendOver)
            {
                break;

//                HeadData head;
//                head.setOperate(CmdClearShm);;
//                std::vector<char> clearHeadBuffer;
//                ShareSerialization::headToBuffer(clearHeadBuffer, head);
//                shmInfo.write(true, clearHeadBuffer);

//                shmInfo.head.operate = CmdReadHead;
//                std::cout<<"send to server CmdClearShm"<<std::endl;
//                end(isSucc);
//                continue;

            }

//            if (shmInfo.head.operate == CmdClearShmComplete)
//            {
//                std::cout<<"client break"<<std::endl;
//                break;
//            }


            if (!isSucc)
                std::this_thread::yield();
        }
    }

//    // 异步发送数据
//    bool send_data_async(const char* data, size_t size) {
//        uint64_t write_pos = shm->write_pos.load(std::memory_order_acquire);
//        uint64_t read_pos = shm->read_pos.load(std::memory_order_acquire);

//        // 检查缓冲区空间
//        uint64_t used = write_pos - read_pos;
//        if ( size > BUFFER_SIZE-used) {
//            return false; // 缓冲区满
//        }

//        // 计算写入位置
//        uint64_t new_write_pos = write_pos + size;
//        uint64_t buffer_pos = write_pos % BUFFER_SIZE;

//        // 写入数据
//        if (buffer_pos + size <= BUFFER_SIZE) {
//            memcpy(shm->buffer + buffer_pos, data, size);
//        } else {
//            // 处理环形跨越
//            size_t first_part = BUFFER_SIZE - buffer_pos;
//            memcpy(shm->buffer + buffer_pos, data, first_part);
//            memcpy(shm->buffer, data + first_part, size - first_part);
//        }

//        // 更新写入位置
//        shm->write_pos.store(new_write_pos, std::memory_order_release);
//        shm->clientData_ready.store(true, std::memory_order_release);
//        return true;
//    }

//    // 接收结果
//    size_t receive_results(char* buffer, size_t buffer_size, int timeout_ms = 100) {
//        auto start = std::chrono::steady_clock::now();

//        while (true) {
//            uint64_t write_pos = shm->write_pos.load(std::memory_order_acquire);
//            uint64_t read_pos = shm->read_pos.load(std::memory_order_acquire);

//            if (write_pos > read_pos) {
//                size_t available = write_pos - read_pos;
//                size_t to_read = std::min(available, buffer_size);

//                uint64_t buffer_pos = read_pos % BUFFER_SIZE;

//                if (buffer_pos + to_read <= BUFFER_SIZE) {
//                    memcpy(buffer, shm->buffer + buffer_pos, to_read);
//                } else {
//                    size_t first_part = BUFFER_SIZE - buffer_pos;
//                    memcpy(buffer, shm->buffer + buffer_pos, first_part);
//                    memcpy(buffer + first_part, shm->buffer, to_read - first_part);
//                }

//                // 更新读取位置
//                shm->read_pos.store(read_pos + to_read, std::memory_order_release);
//                return to_read;
//            }

//            auto now = std::chrono::steady_clock::now();
//            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
//            if (elapsed.count() > timeout_ms) {
//                return 0; // 超时
//            }

//            std::this_thread::yield();
//        }
        
//        shm->clientData_ready.store(false, std::memory_order_release);
//    }

//    // 批量发送大数据
//    bool send_large_data(const char* data, size_t total_size, size_t chunk_size = 65536) {
//        for (size_t offset = 0; offset < total_size; offset += chunk_size) {
//            size_t current_chunk = std::min(chunk_size, total_size - offset);

//            // 等待缓冲区空间
//            int retry_count = 0;
//            while (!send_data_async(data + offset, current_chunk)) {
//                if (++retry_count > 100) {
//                    return false;
//                }
//                std::this_thread::sleep_for(std::chrono::microseconds(100));
//            }
//        }
//        return true;
//    }

    ~OctaveClient() {
        std::cout<<"~OctaveClient"<<std::endl;
        initServerSharedMemory.closeMapShare();
        shmInfo.deleteShare();
    }
    ShareMInfo& getShmInfo()
    {
        return shmInfo;
    }

};

#endif // OCTAVECLIENT_H
