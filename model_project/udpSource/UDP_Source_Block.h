#ifndef UDP_SOURCE_BLOCK_H
#define UDP_SOURCE_BLOCK_H

#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QHostAddress>
#include <mutex>
#include <queue>
#include "Block.h"

#include "CommonDefines.h"
#include "RadarControl.h"
#include "RadarDataReceiver.h"
#include "RadarSignalProcessor.h"
#include "ServoControl.h"
#include "HostCommunication.h"



enum SourceHeaderType {
    SOURCEHEADERTYPE_NONE = 0,        //无头部
    SOURCEHEADERTYPE_SEQNUM = 1,      //序列号头部
    SOURCEHEADERTYPE_SEQPLUSSIZE = 2, //序列号+包大小头部
};

//序列号头部结构 8字节
#pragma pack(push, 1)
struct SourceHeaderSeqNum {
    uint64_t seqnum;  //序列号
};

//序列号+包大小头部结构
struct SourceHeaderSeqPlusSize {
    uint64_t seqnum;  //序列号
    uint32_t length;  //包大小
};
#pragma pack(pop)


class UDP_Source_Block : public Block
{
public:
    UDP_Source_Block(const std::string& name,
                     size_t itemsize,
                     size_t veclen,
                     int port,
                     int header_type = SOURCEHEADERTYPE_NONE,
                     int payload_size = 1472,
                     bool notify_missed = true,
                     bool source_zeros = false,
                     bool ipv6 = false);
    ~UDP_Source_Block();

    void Setup() override;
    void Stop() override;
    void Run() override;

    void SetPort(int port);
    void SetPayloadSize(int size);
    void SetNotifyMissed(bool notify_missed);
    void SetSourceZeros(bool source_zeros);
    void SetHeaderType(int header_type);
    void SetIPv6(bool ipv6);
    void SetItemsize(size_t itemsize);
    void SetVeclen(size_t veclen);

    // 设置发送目标
    void SetSendTarget(const std::string& host, int port);
private:
    // 网络配置
    int m_port;
    int m_payload_size;
    bool m_notify_missed;
    bool m_source_zeros;
    bool m_ipv6;
    bool m_running;

    // 数据格式配置
    size_t m_itemsize;
    size_t m_veclen;
    size_t m_block_size;

    // 包头处理
    int m_header_type;
    int m_header_size;
    uint64_t m_seq_num;
    uint64_t m_expected_seq_num;
    int m_missed_packets;

    // 网络组件
    QUdpSocket* m_udp_socket;
    QUdpSocket* m_send_socket;  // 新增：发送socket
    QHostAddress m_bind_address;
    QHostAddress m_send_address; // 新增：发送目标地址
    int m_send_port;            // 新增：发送目标端口

    // 缓冲区管理
    std::vector<uint8_t> m_receive_buffer;
    std::queue<std::vector<uint8_t>> m_data_queue;  // 原始字节队列
    std::mutex m_queue_mutex;
    std::mutex m_socket_mutex;

    // 统计信息
    size_t m_total_bytes_received;
    size_t m_total_packets_received;
    size_t m_total_packets_dropped;

    //雷达与伺服控制系统组件
    LocalData m_localData;
    RadarControl* m_radarControl;
    RadarDataReceiver* m_radarDataReceiver;
    RadarSignalProcessor* m_radarSignalProcessor;
    ServoControl* m_servoControl;
    HostCommunication* m_hostCommunication;
    bool m_systemInitialized;


    std::atomic<bool> m_controlThreadRunning;
    std::thread m_controlThread;

    void Initialize();
    bool BindSocket();
    bool InitSendSocket();  // 新增：初始化发送socket
    void ProcessDatagram(const QByteArray& datagram);
    void ParseHeader(const uint8_t* header_buffer, uint64_t& seq_num, uint32_t& payload_length);
    void HandleMissedPackets(uint64_t received_seq_num);
    void cleanup();
    void ProcessQueueData();

    void ProcessRadarDataAndControl();

    // 新增：发送数据方法
    void SendRadarData();
    QByteArray BuildRadarPacket();

    bool ValidatePayloadSize() const;

    void ControlThreadFunction();

    void CheckForUDPData();
};

#endif // UDP_SOURCE_BLOCK_H
