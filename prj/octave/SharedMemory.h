#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

// 高性能共享内存通信系统
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#include "DataInterface.h"

struct OctaveClientInfo
{
    char shmName[30];
    int cmpId{-1};
    CmdType operate=CmdRegisterCmp;
    //    uint64_t total[20];
    OctaveClientInfo()
    {

    }
    // 直接写入到预分配的内存

    static void writeToBuffer(std::vector<char> &buffer, const OctaveClientInfo& info) {
        std::memcpy(buffer.data(), &info, sizeof(OctaveClientInfo));
    }

    static void readFromBuffer(const std::vector<char> &buffer, OctaveClientInfo& info) {
        std::memcpy(&info, buffer.data(), sizeof(OctaveClientInfo));
        //        return src + sizeof(OctaveClientInfo);
    }



    static char* writeToMemory(char* dest, const OctaveClientInfo& info) {
        std::memcpy(dest, &info, sizeof(OctaveClientInfo));
        return dest + sizeof(OctaveClientInfo);
    }

    // 从内存直接读取
    static const char* readFromMemory(const char* src, OctaveClientInfo& info) {
        std::memcpy(&info, src, sizeof(OctaveClientInfo));
        return src + sizeof(OctaveClientInfo);
    }

    // 批量写入到预分配内存
    static char* writeMultipleToMemory(char* dest, const OctaveClientInfo* infos, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            dest = writeToMemory(dest, infos[i]);
        }
        return dest;
    }

    // 从内存批量读取
    static const char* readMultipleFromMemory(const char* src, OctaveClientInfo* infos, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            src = readFromMemory(src, infos[i]);
        }
        return src;
    }

    // 计算需要的内存大小
    static size_t calculateRequiredSize(size_t count) {
        return count * sizeof(OctaveClientInfo);
    }

    // 计算带头的内存大小
    static size_t calculateRequiredSizeWithHeader(size_t count) {
        return sizeof(uint32_t) + count * sizeof(OctaveClientInfo);
    }
};

// 共享内存结构定义
struct InitServerSharedMemory {
    std::atomic<bool> stopInit{false}; // 服务端就绪标志
};

const size_t SHM_SIZE = 1024 * 1024 * 11; // 100MB共享内存
const size_t BUFFER_SIZE = 5*1024 * 1024;
struct SharedMemory {
    std::atomic<bool> lockClient{false};    // 写入位置
    std::atomic<bool> lockServer{false};    // 写入位置
    std::atomic<uint64_t> write_pos{0};    // 写入位置
    std::atomic<uint64_t> read_pos{0};     // 读取位置

    std::atomic<uint64_t> write_posClient{0};    // 写入位置
    std::atomic<uint64_t> read_posClient{0};     // 读取位置

    std::atomic<bool> stop{false};
    char bufferClient[BUFFER_SIZE];                       // 数据缓冲区（柔性数组）
    char bufferSever[BUFFER_SIZE];                       // 数据缓冲区（柔性数组）

};


template<typename T>
class ShareMInfoBase{

protected:
    int shm_fd{0};
    T* shm{nullptr};
    std::string shm_name;
    int shmSize=SHM_SIZE;
    //    bool isEnd {false};

public:


    virtual ~ShareMInfoBase()
    {

    }

    virtual void init()
    {

    }


    void deleteShare() {
        if (shm) {
            munmap(shm, shmSize);
            shm = nullptr;
        }
        if (shm_fd >= 0) {
            close(shm_fd);
            shm_unlink(shm_name.c_str());
            shm_fd=-1;
        }
    }

    void createShare()
    {
        // 创建/打开共享内存
        std::cerr << "UID=" << getuid() << " EUID=" << geteuid() << std::endl;
        shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
        if (shm_fd==-1)
        {
            std::cerr << "shm_open failed: " << strerror(errno) << std::endl;
            throw new std::runtime_error("shm_open failed");

        }
        std::cout <<"shm_fd "<<shm_fd<<std::endl;

        if (ftruncate(shm_fd, shmSize)==-1)
            throw new std::runtime_error("ftruncate failed");

        shm = static_cast<SharedMemory*>(
                    mmap(nullptr, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)
                    );
        if (shm==MAP_FAILED)
            throw new std::runtime_error("mmap failed");
        init();
    }

    void mapShare()
    {
        // 打开共享内存

        shm_fd = shm_open(shm_name.c_str(), O_RDWR, 0666);
        if (shm_fd==-1){
            std::cerr << "shm_open failed: " << strerror(errno) << std::endl;
            std::cerr << "UID=" << getuid() << " EUID=" << geteuid() << std::endl;
            throw new std::runtime_error("shm_open failed");

        }
        std::cout <<"mapShare "<<shm_fd<<std::endl;
        shm = static_cast<SharedMemory*>(
                    mmap(nullptr, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)
                    );
        if (shm==MAP_FAILED)
            throw new std::runtime_error("mmap failed");
    }

    void closeMapShare()
    {
        if (shm) {
            munmap(shm, shmSize);
        }
        if (shm_fd >= 0) {
            close(shm_fd); //??
        }
    }

    void setShm_name(const std::string &value)
    {
        shm_name = value;
    }

    std::string getShm_name() const
    {
        return shm_name ;
    }

    //    ParamSet & getPSet() ;
    //    bool getIsEnd() const;
    //    void setIsEnd(bool value);

    //    bool getIsEnd() const
    //    {
    //        return isEnd;
    //    }

    //    void setIsEnd(bool value)
    //    {
    //        isEnd = value;
    //    }


};
#include "ShareSerialization.h"
class ShareMInfo  : public ShareMInfoBase<SharedMemory>{
private:
    int cmpId = -1;

public:
    ParamInfo pSet;
    HeadData head;
    std::string code;

    //    bool isServerSend{false};
    bool isInitShm = false;
    ShareMInfo ()
    {
        head.operate = CmdReadHead;
        shmSize = SHM_SIZE;
    }

    void reset()
    {
        shm->stop.store(false, std::memory_order_acquire);
        shm->write_pos.store(0, std::memory_order_acquire);
        shm->read_pos.store(0, std::memory_order_acquire);

        shm->write_posClient.store(0, std::memory_order_acquire);
        shm->read_posClient.store(0, std::memory_order_acquire);

    }

    ~ShareMInfo ()
    {
        clear();
    }

    void sendResult()
    {
        for (auto it = pSet.paramSet.begin(); it != pSet.paramSet.end(); ++it) {
            Param &param = it.value();
            if (param.head.putType ==Put_Out)
            {
                std::vector<char> headBuffer;
                param.head.operate = CmdReadParamData;
                ShareSerialization::headToBuffer(headBuffer, param.head);
                write(false, headBuffer);
                write(false, param.dataBuffer);
            }
        }

        HeadData overHead;
        overHead.setOperate(CmdSendOver);
        std::vector<char> overHeadBuffer;
        ShareSerialization::headToBuffer(overHeadBuffer, overHead);
        write(false, overHeadBuffer);
    }
    void clear()
    {
        head.operate = CmdReadHead;
        pSet.paramSet.clear();
        code.clear();
        if (isInitShm)
        {
            deleteShare();
        }else
            closeMapShare();
    }

    //    bool runCode()
    //    {
    //       return false;
    //    }
    void init()
    {
        // 初始化原子变量
        new (&shm->write_pos) std::atomic<uint64_t>(0);
        new (&shm->read_pos) std::atomic<uint64_t>(0);

        new (&shm->write_posClient) std::atomic<uint64_t>(0);
        new (&shm->read_posClient) std::atomic<uint64_t>(0);
        //        new (&shm->server_ready) std::atomic<bool>(false);
        //        new (&shm->clientData_ready) std::atomic<bool>(false);
        //        new (&shm->writeLock) std::atomic<bool>(false);
        new (&shm->stop) std::atomic<bool>(false);
        new (&shm->lockClient) std::atomic<bool>(false);
        new (&shm->lockServer) std::atomic<bool>(false);
    }

    bool getStop()const
    {
        return shm->stop.load(std::memory_order_acquire);
    }
    void setStop(bool stop_)
    {
        shm->stop.store(stop_, std::memory_order_release); // 更新读取位置

    }

    //    bool isClientDataReady()
    //    {
    //        return  shm->clientData_ready.load(std::memory_order_acquire);
    //    }

    //    void setClientDataReady(bool isReady)
    //    {
    //          shm->clientData_ready.store(isReady, std::memory_order_release);
    //    }


    //    bool isStopInit()
    //    {
    ////        return shm->isStopInit_.load(std::memory_order_acquire);
    //    }


    //size需要读取的数据长度
    bool read(bool isSever, std::vector<char> &local_buffer, uint64_t size)
    {
        if (size == 0)
            return true;
        uint64_t write_pos = 0;
        uint64_t read_pos = 0;
        char*buffer = nullptr;
        // 检查是否有新数据  // memory_order_acquire 保证：在此操作之后的所有读写不会被重排到它前面
        if (isSever)
        {
            write_pos = shm->write_pos.load(std::memory_order_acquire);
            read_pos = shm->read_pos.load(std::memory_order_acquire);
            buffer = shm->bufferSever;
        }else {
            write_pos = shm->write_posClient.load(std::memory_order_acquire);
            read_pos = shm->read_posClient.load(std::memory_order_acquire);
            buffer = shm->bufferClient;
        }


        size_t to_read = write_pos - read_pos;
        if (size>to_read)
            return false;

        if (write_pos > read_pos) {
            // 计算可用数据大小

            to_read = size;
            local_buffer.resize(to_read);

            // 从共享内存读取数据
            uint64_t buffer_pos = read_pos % BUFFER_SIZE;
            if (buffer_pos + to_read <= BUFFER_SIZE) {
                memcpy(local_buffer.data(), buffer + buffer_pos, to_read);
            } else
            {
                // 处理环形跨越
                size_t first_part = BUFFER_SIZE - buffer_pos;
                memcpy(local_buffer.data(), buffer + buffer_pos, first_part);
                memcpy(local_buffer.data() + first_part, buffer, to_read - first_part);
            }
            if (isSever)
            {
                shm->read_pos.store(read_pos + to_read, std::memory_order_release); // 更新读取位置
            }else{
                shm->read_posClient.store(read_pos + to_read, std::memory_order_release); // 更新读取位置
            }

            return true;
        }else {
            return false;
        }

    }

    void write_to_buffer(char*buffer, uint64_t pos, const char* src, size_t size) {
        uint64_t off = pos % BUFFER_SIZE;
        if (off + size <= BUFFER_SIZE) {
            std::memcpy(buffer + off, src, size);
        } else {
            size_t first = BUFFER_SIZE - off;
            std::memcpy(buffer + off, src, first);
            std::memcpy(buffer, src + first, size - first);
        }
    }

    bool write(bool isServer , const std::vector<char> &inData) {
        //        const char* src = static_cast<const char*>(data);
        size_t inData_size = inData.size();
        uint64_t wp, rp;
        uint64_t new_wp;

         if (inData_size == 0)
             return true;;

        char * buffer = nullptr;

        if (isServer)
        {
            bool expected = false;
            // 期望值是false，如果当前是false（未锁），则设置为true（上锁）
            while (!shm->lockServer.compare_exchange_weak(expected, true,
                   std::memory_order_acquire)) {
                expected = false;  // 重置期望值
                // 自旋等待
                // 可以添加退让策略
                std::this_thread::yield();
            }

            buffer = shm->bufferSever;
            wp = shm->write_pos.load(std::memory_order_acquire);
            rp = shm->read_pos.load(std::memory_order_acquire);

        }else {

            bool expected = false;
            // 期望值是false，如果当前是false（未锁），则设置为true（上锁）
            while (!shm->lockClient.compare_exchange_weak(expected, true,
                   std::memory_order_acquire)) {
                expected = false;  // 重置期望值
                // 自旋等待
                // 可以添加退让策略
                std::this_thread::yield();
            }

            buffer = shm->bufferClient;
            wp = shm->write_posClient.load(std::memory_order_acquire);
            rp = shm->read_posClient.load(std::memory_order_acquire);


        }

        new_wp = wp + inData_size;


        bool noBuffer = (inData_size > BUFFER_SIZE - (wp - rp));


        // ✅ 已预留空间，安全
        if (!noBuffer)
            write_to_buffer(buffer, wp, inData.data(), inData_size);

        if (isServer)
        {
            shm->write_pos.store(new_wp, std::memory_order_release);
            shm->lockServer.store(false, std::memory_order_release);
        }else
        {
            shm->write_posClient.store(new_wp, std::memory_order_release);

            shm->lockClient.store(false, std::memory_order_release);
        }
        return true;
    }

    //    void write(const std::vector<char> &inData)
    //    {
    //        size_t inData_size = inData.size();
    //        if (inData_size==0)
    //            return;


    //        while(true)
    //        {
    //            bool writeLock = shm->writeLock.load(std::memory_order_acquire);
    //            if (!writeLock)
    //                break;
    //            std::this_thread::yield();
    //        }
    //        shm->writeLock.store(true, std::memory_order_release);


    //        uint64_t write_pos = shm->write_pos.load(std::memory_order_acquire);


    //        while(true)
    //        {
    //             uint64_t read_pos = shm->read_pos.load(std::memory_order_acquire);
    //             // 检查缓冲区空间
    //             uint64_t used = write_pos - read_pos;
    //             if ( inData_size > BUFFER_SIZE-used) {
    //                 std::this_thread::yield();
    ////                 return;
    //                 continue; // 缓冲区满
    //             }
    //             break;
    //        }

    //        // 将结果写回共享内存

    //        uint64_t new_write_pos = write_pos + inData_size;
    //        uint64_t inData_pos = write_pos % BUFFER_SIZE;

    //        if (inData_pos + inData_size <= BUFFER_SIZE) {
    //            memcpy(shm->buffer + inData_pos, inData.data(), inData_size);
    //        } else { // 处理环形跨越
    //            size_t first_part = BUFFER_SIZE - inData_pos;
    //            memcpy(shm->buffer + inData_pos, inData.data(), first_part);
    //            memcpy(shm->buffer, inData.data() + first_part, inData_size - first_part);
    //        }

    //        // 更新写入位置
    //        shm->write_pos.store(new_write_pos, std::memory_order_release);


    //        shm->writeLock.store(false, std::memory_order_release); //unlock

    //    }
    int getCmpId() const
    {
        return cmpId;
    }

    void setCmpId(int value)
    {
        cmpId = value;
    }

};


#endif // SHAREDMEMORY_H
