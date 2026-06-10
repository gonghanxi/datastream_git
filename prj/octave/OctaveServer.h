#ifndef OCTAVESERVER_H
#define OCTAVESERVER_H

#include "SharedMemory.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <vector>
#include <thread>

#include <QHash>
#include "DataInterface.h"

#include "SafeList.h"

#include "OctaveRun.h"
#include "OctaveUtl.h"
class OctaveServer {
private:

    QHash<int, bool> shareMInfoHash;

    OctaveRun runMatlab;

//    QHash<int, ShareMInfo> shareMInfoHash;

//    bool stopExchange {false};

    SafeList<ShareMInfo> shareMList;
//    ShareMInfo initServerSharedMemory;

    bool registerCmp(ShareMInfo& shmInfo, std::vector<char> &headBuffer, uint64_t sizeHead)
    {
        bool isReadHead = shmInfo.read(true, headBuffer,sizeHead);
        if (isReadHead)
        {
            HeadData head;
            ShareSerialization::bufferToHead(headBuffer, head);
            std::cout<<head.cmpId<<std::endl;
            std::cout<<head.shmName<<std::endl;


            Node<ShareMInfo> *sharMNode = new Node<ShareMInfo>();
//            sharMNode->value


            if (shareMInfoHash.contains(head.cmpId))
            {
                std::cout<<head.cmpId<<" has contains "<<std::endl;
                return false;
            }
            shareMInfoHash.insert(head.cmpId,true);
            shareMList.push_back(sharMNode);

            ShareMInfo &mInfo = sharMNode->value;
            mInfo.setCmpId( head.cmpId);
            mInfo.head = head;

            mInfo.setShm_name(std::string(head.shmName));
            mInfo.mapShare();
        }
        return isReadHead;

    }


//    bool readHead(ShareMInfo& shmInfo, std::vector<char> &headBuffer, uint64_t sizeHead)
//    {
//        bool isReadHead = shmInfo.read(headBuffer,sizeHead);
//        if (isReadHead)
//        {
//            HeadData head;
//            std::memcpy(&head, headBuffer.data(), sizeof(HeadData));
//            shmInfo.pSet.addParamHead(head);
//            shmInfo.head=head;
//        }
//        return isReadHead;
//    }

//    bool readData(ShareMInfo& shmInfo)
//    {
//        std::vector<char> &dataBuffer = shmInfo.pSet.getData(shmInfo.head.getParamName());
//        uint64_t totalSize = shmInfo.head.getTotalElements();
//        bool isReadData = shmInfo.read(dataBuffer,totalSize);
//        if (isReadData)
//        {
//            std::cout<<dataBuffer.data();
//          shmInfo.head.operate = CmdReadHead;
//        }
//        return isReadData;
//    }

    bool readCode(ShareMInfo& shmInfo)
    {
        std::vector<char> dataBuffer;
        uint64_t totalSize = shmInfo.head.getBytes();
        bool isReadData = shmInfo.read(true, dataBuffer,totalSize);
        if (isReadData)
        {
          shmInfo.code.assign(dataBuffer.begin(), dataBuffer.end());
          shmInfo.head.operate = CmdReadHead;
        }
        return isReadData;
    }


public:
    OctaveServer() {

        Node<ShareMInfo> *initServerSharedMemory = new Node<ShareMInfo>();
        initServerSharedMemory->value.setShm_name("/init_server_shmxx");
        initServerSharedMemory->value.createShare();
        initServerSharedMemory->value.head.setOperate(CmdRegisterCmp);
        initServerSharedMemory->value.isInitShm = true;
//        initServerSharedMemory->value.init();

        shareMList.push_back(initServerSharedMemory);
    }



    void end(Node<ShareMInfo>* &cur , Node<ShareMInfo>* &next, bool &isSucc)
    {
        if (shareMList.isTail(cur))
            cur = shareMList.first();
        else {
            cur = next;
            if (cur==nullptr)
                cur = shareMList.first();

        }
        if (!isSucc)
            std::this_thread::yield();
        isSucc = false;
    }

    void run() {
        std::cout << "octave服务开启，等待数据..." << std::endl;
//        std::vector<char> buffer;
//        uint64_t size = sizeof(HeadData);

        Node<ShareMInfo>* cur = shareMList.first();
        Node<ShareMInfo>* next = nullptr;

        std::vector<char> headBuffer;
        uint64_t sizeHead= sizeof(HeadData);
        bool isSucc = false;
        while (cur) {
            ShareMInfo& shmInfo = cur->value;
            next = cur->next;
            if (shmInfo.head.operate==CmdRegisterCmp)
            {
               isSucc = registerCmp(shmInfo, headBuffer, sizeHead);
               if (isSucc)
               {
                   std::cout <<shmInfo.head.cmpId<< " server 注册模型共享内存" << std::endl;
                   next = cur->next;
               }

               end(cur, next, isSucc);
               continue;
            }

            if (shmInfo.head.operate==CmdReadHead)
            {
                isSucc = OctaveUtl::readHead(true, shmInfo, headBuffer, sizeHead);
                if(isSucc)
                {
                    std::cout <<shmInfo.head.paramName<< " server 读取数据头..." << std::endl;
                }
                end(cur, next, isSucc);
                continue;
            }
            if (shmInfo.head.operate==CmdReadParamData)
            {
                isSucc = OctaveUtl::readParamData(true, shmInfo);
                if(isSucc)
                {
                    std::cout <<shmInfo.head.paramName<< " server 读取数据体..." << std::endl;
                }
                end(cur, next, isSucc);
                continue;
            }

            if (shmInfo.head.operate==CmdReadCode)
            {
                isSucc = readCode(shmInfo);

                if(isSucc)
                {
                    std::cout <<shmInfo.head.cmpId<< " server 读取代码..."<<shmInfo.code << std::endl;
                }
                end(cur, next, isSucc);
                continue;
            }

            if (shmInfo.head.operate==CmdRunCode)
            {

                isSucc = runMatlab.runCode(shmInfo.pSet, shmInfo.code);
                if(isSucc)
                {
                    std::cout <<shmInfo.head.cmpId<< " server 执行代码..." << std::endl;
                    shmInfo.head.operate = CmdSendResult;
                }
                end(cur, next, isSucc);
                continue;
            }

            if (shmInfo.head.operate==CmdSendResult)
            {
                std::cout <<shmInfo.head.cmpId<< " server 发送结果..." << std::endl;

                shmInfo.sendResult();
                shmInfo.head.operate = CmdReadHead;
                end(cur, next, isSucc);
                continue;
            }

            if (shmInfo.head.operate==CmdClearShm)
            {
                std::cout <<shmInfo.getCmpId()<< " server 删除共享内存..." << std::endl;
                HeadData head;
                std::vector<char> headBuffer;
                head.operate = CmdClearShmComplete;
                ShareSerialization::headToBuffer(headBuffer, head);
                shmInfo.write(false, headBuffer);


                shmInfo.setStop(true);
                shareMInfoHash.remove(shmInfo.getCmpId());
                next = shareMList.erase(cur);
                shmInfo.head.operate = CmdReadHead;



//                HeadData head;
//                head.setOperate(CmdClearShmComplete);
////                head.setParamName("CmdClearShmComplete");
//                std::vector<char> headBuffer;
//                ShareSerialization::headToBuffer(headBuffer, head);
//                shmInfo.write(false, headBuffer);

                end(cur, next, isSucc);
                continue;
//                continue;
            }

           end(cur, next, isSucc);

        }

//        OctaveClientInfo cInfo;
//        while (true)
//        {
//            if (initServerSharedMemory.isStopInit())
//            {
//                break;
//            }
//           bool isRead = initServerSharedMemory.read(buffer, size);
//           if (!isRead)
//               std::this_thread::yield();
//           else {
//               registerCmp(bufferHe);
//           }
//        }

    }

    ~OctaveServer() {

        shareMList.clear();
//        initServerSharedMemory.deleteShare();
//        for (auto it = shareMInfoHash.begin(); it != shareMInfoHash.end(); ++it) {
//            ShareMInfo& shmInfo = it.value();
//            shmInfo.closeMapShare();

//        }
    }
};
#endif // OCTAVESERVER_H
