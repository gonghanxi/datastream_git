#ifndef UDP_SINK_BLOCK_H
#define UDP_SINK_BLOCK_H

#include <mutex>
#include <cstdint>
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QHostAddress>

#include "Block.h"

enum HeaderType {
    HEADERTYPE_NONE = 0,        //无头部
    HEADERTYPE_SEQNUM = 1,      //序列号头部
    HEADERTYPE_SEQPLUSSIZE = 2, //序列号+包大小头部
};

//序列号头部结构 8字节
#pragma pack(push, 1)
struct HeaderSeqNum {
    uint64_t seqnum;  //序列号
};

//序列号+包大小头部结构
struct HeaderSeqPlusSize {
    uint64_t seqnum;  //序列号
    uint32_t length;  //包大小
};
#pragma pack(pop)

class UDP_Sink_Block : public Block
{
public:
    UDP_Sink_Block(const std::string& name,
                   const std::string& host,
                   int port,
                   int header_type = HEADERTYPE_NONE,
                   int payload_size = 1472,
                   bool send_eof = true
                   );
    ~UDP_Sink_Block();

    void Setup() override;
    void Stop() override;
    void Run() override;

    void SetDestination(const std::string& host, int port);
    void SetPayloadSize(int size);
    void SetSendEof(bool send_eof);
    void SetHeaderType(int header_type);

private:
    std::string m_host;
    int m_port;
    int m_payload_size;
    bool m_send_eof;
    bool m_running;
    bool m_is_ipv6;
    int m_header_type;
    int m_header_size;
    uint64_t m_seq_num;

    QUdpSocket* m_udp_socket;
    QHostAddress m_dest_address;

    std::vector<uint8_t> m_send_buffer;
    std::vector<uint8_t> m_head_buffer;
    std::mutex m_socket_mutex;

    void Initialize();
    bool ConnectSocket();
    void BuildHeader(uint8_t* header_buffer);
    void DetermineIPVersion();
    void SendData(const std::vector<SystemVueModelBuilder::EnvelopeSignal>& data);
    void SendEof();
    void cleanup();

};

#endif // UDP_SINK_BLOCK_H
