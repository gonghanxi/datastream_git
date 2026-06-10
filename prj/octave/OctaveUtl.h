#ifndef OCTAVEUTL_H
#define OCTAVEUTL_H
#include "DataInterface.h"
#include "SharedMemory.h"
//#include "SafeList.h"

//#include "OctaveRun.h"

class OctaveUtl
{
public:
    OctaveUtl();

   static bool readHead(bool isServer, ShareMInfo& shmInfo, std::vector<char> &headBuffer, uint64_t sizeHead)
    {
        bool isReadHead = shmInfo.read(isServer, headBuffer,sizeHead);
        if (isReadHead)
        {
            HeadData head;
            std::memcpy(&head, headBuffer.data(), sizeof(HeadData));
            shmInfo.head=head;
            if (head.dataType == DataTypeCmd)
            {
                std::cout<<"执行命令："<<head.cmdName<<std::endl;
                return isReadHead;
            }else {
                std::cout<<"参数："<<head.paramName<<std::endl;
            }
            shmInfo.pSet.addParamHead(head);

        }
        return isReadHead;
    }

   static bool readParamData(bool isServer,ShareMInfo& shmInfo)
    {
        std::vector<char> &dataBuffer = shmInfo.pSet.getData(shmInfo.head.getParamName());
        uint64_t totalSize = shmInfo.head.getBytes();
        bool isReadData = shmInfo.read(isServer, dataBuffer,totalSize);
        if (isReadData)
        {
//            std::cout<<dataBuffer.data()<<std::endl;
            shmInfo.head.operate = CmdReadHead;
        }
        return isReadData;
    }
};

#endif // OCTAVEUTL_H
