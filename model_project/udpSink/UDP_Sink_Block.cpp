#include "UDP_Sink_Block.h"
#include <QHostInfo>
#include <algorithm>
#include <QByteArray>
#include <QtEndian>

// #ifdef _WIN32
// #include <winsock2.h>
// #else
// #include <arpa/inet.h>
// #endif

UDP_Sink_Block::UDP_Sink_Block(const std::string &name,
                               const std::string &host,
                               int port,
                               int header_type,
                               int payload_size,
                               bool send_eof)
    :Block(name),
    m_host(host),
    m_port(port),
    m_payload_size(payload_size),
    m_send_eof(send_eof),
    m_running(false),
    m_is_ipv6(false),
    m_header_type(header_type),
    m_header_size(0),
    m_seq_num(0),
    m_udp_socket(nullptr)
{
    Initialize();

}

UDP_Sink_Block::~UDP_Sink_Block()
{
    cleanup();
}

// uint64_t UDP_Sink_Block::hostToNetwork64(uint64_t value) {
// #ifdef _WIN32
//     return htonll(value);
// #else
//     return htobe64(value);
// #endif
// }

// uint32_t UDP_Sink_Block::hostToNetwork32(uint32_t value) {
// #ifdef _WIN32
//     return htonl(value);
// #else
//     return htobe32(value);
// #endif
// }

void UDP_Sink_Block::Initialize()
{
    SetBlockType(Block::BlockType::SINK);

    switch(m_header_type) {
    case HEADERTYPE_NONE:
        m_header_size = 0;
        break;
    case HEADERTYPE_SEQNUM:
        m_header_size = sizeof(HeaderSeqNum);
        break;
    case HEADERTYPE_SEQPLUSSIZE:
        m_header_size = sizeof(HeaderSeqPlusSize);
        break;
    default:
        std::cerr << "UDP Sink: Unkown header type, using NONE" << std::endl;
        m_header_type = HEADERTYPE_NONE;
        m_header_size = 0;
        break;
    }

    if(m_payload_size < 8) {
        std::cerr << "UDP Sink: Payload is too small, setting to minimum 8 bytes" << std::endl;
        m_payload_size = 8;
    }
    m_udp_socket = new QUdpSocket();
    m_send_buffer.reserve(m_payload_size);
    AddInputPort("udp_input", 1, Block::DataType::ENVELOPE_SIGNAL);
}

void UDP_Sink_Block::Setup()
{
    Block::Setup();

    if(ConnectSocket()) {
        m_running = true;
        std::cout << "UDP Sink Block started -> " << m_host << ":" << m_port << std::endl;
    }
    else {
        std::cerr << "UDP Sink Block failed to connected to " << m_host << ":" << m_port << std::endl;
    }

}

bool UDP_Sink_Block::ConnectSocket()
{
    m_dest_address = QHostAddress(QString::fromStdString(m_host));
    if(m_dest_address.isNull()) {
        QHostInfo info = QHostInfo::fromName(QString::fromStdString(m_host));
        if(!info.addresses().isEmpty()) {
            m_dest_address = info.addresses().first();
        }
        else {
            std::cerr << "Failed to resolve host: " << m_host << std::endl;
            return false;
        }
    }

    DetermineIPVersion();

    if(m_udp_socket->isOpen()) {
        m_udp_socket->close();
    }

    delete m_udp_socket;
    m_udp_socket = new QUdpSocket();

    //绑定
    bool bind_success = false;
    if(m_is_ipv6) {
        bind_success = m_udp_socket->bind(QHostAddress(QHostAddress::AnyIPv6), 0);
        if(!bind_success) {
            std::cerr << "UDP Sink: Failed to bind IPv6 socket" << std::endl;
            bind_success = m_udp_socket->bind(QHostAddress(QHostAddress::AnyIPv4), 0);
            if(bind_success) {
                std::cout << "UDP Sink: Fallback to IPv4 binding successful" << std::endl;
                m_is_ipv6 = false;
            }
        }
    }
    else {
        bind_success = m_udp_socket->bind(QHostAddress(QHostAddress::AnyIPv4), 0);
        if(!bind_success) {
            std::cerr << "UDP Sink: Failed to bind IPv4 socket" << std::endl;
        }
    }

    if(!bind_success) {
        std::cerr << "UDP Sink: Failed to bind socket to any address" << std::endl;
        return false;
    }

    QHostAddress local_address = m_udp_socket->localAddress();
    quint16 local_port = m_udp_socket->localPort();
    std::cout << "UDP Sink: Bound to local address " << local_address.toString().toStdString()
              << ":" << local_port << std::endl;

    return true;

}

void UDP_Sink_Block::BuildHeader(uint8_t *header_buffer)
{
    switch(m_header_type) {
    case HEADERTYPE_SEQNUM: {
        m_seq_num++;
        HeaderSeqNum seq_header;
        seq_header.seqnum = qToBigEndian(m_seq_num);
        std::memcpy(header_buffer, &seq_header, m_header_size);
        break;
    }
    case HEADERTYPE_SEQPLUSSIZE: {
        m_seq_num++;
        HeaderSeqPlusSize seq_header_plus_size;
        seq_header_plus_size.seqnum = qToBigEndian(m_seq_num);
        seq_header_plus_size.length = qToBigEndian(static_cast<uint32_t>(m_payload_size));
        // seq_header_plus_size.length = static_cast<uint32_t>(m_send_buffer.size());
        std::memcpy(header_buffer, &seq_header_plus_size, m_header_size);
        break;
    }
    case HEADERTYPE_NONE: {
        break;
    }
    }
}

void UDP_Sink_Block::DetermineIPVersion()
{
    if(m_host.find(":") != std::string::npos) {
        m_is_ipv6 = true;
        std::cout << "UDP Sink: Detected IPv6 address from host string" << std::endl;
        return;
    }
    if(m_dest_address.protocol() == QAbstractSocket::IPv6Protocol) {
        m_is_ipv6 = true;
    }
    else {
        m_is_ipv6 = false;
    }
}

void UDP_Sink_Block::SendData(const std::vector<SystemVueModelBuilder::EnvelopeSignal> &data)
{
    if(!m_running || !m_udp_socket || data.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_socket_mutex);

    //计算每个样本的大小
    const size_t sample_size = sizeof(double) * 2;

    //计算有效载荷的大小
    const size_t effective_payload_size = m_payload_size - m_header_size;
    const size_t max_samples_per_socket = (effective_payload_size) / sample_size;

    if(max_samples_per_socket == 0) {
        std::cerr << "Payload size too small for even one sample" << std::endl;
        return;
    }

    int total_packets = 0;
    int total_samples = 0;

    for(size_t start_idx = 0; start_idx < data.size(); start_idx += max_samples_per_socket) {
        size_t end_idx = (std::min)(start_idx + max_samples_per_socket, data.size());
        size_t samples_in_this_packet = end_idx - start_idx;

        size_t data_size = samples_in_this_packet * sample_size;
        size_t total_packet_size = m_header_size + data_size;

        if(total_packet_size > m_payload_size) {
            continue;
        }

        m_send_buffer.resize(total_packet_size);

        // 构建头部
        if (m_header_type != HEADERTYPE_NONE) {
            BuildHeader(m_send_buffer.data());
        }

        //复制数据
        uint8_t* data_buffer = m_send_buffer.data() + m_header_size;
        for(size_t i = 0; i < samples_in_this_packet; ++i) {
            const auto& sample = data[start_idx + i];
            std::complex<double> complex_val = sample.complex();

            // 直接复制 double 数据，不进行转换（保持 IEEE 754 格式）
            double real = complex_val.real();
            double imag = complex_val.imag();

            std::memcpy(data_buffer + i * sample_size, &real, sizeof(double));
            std::memcpy(data_buffer + i * sample_size + sizeof(double), &imag, sizeof(double));
        }

        //发送UDP数据包
        qint64 bytes_sent = m_udp_socket->writeDatagram(
            reinterpret_cast<const char*>(m_send_buffer.data()),
            m_send_buffer.size(),
            m_dest_address,
            m_port);

        if(bytes_sent == -1) {
            std::cerr << "UDP Send error - " << m_udp_socket->errorString().toStdString()
                      << " (seq: " << m_seq_num << " )" << std::endl;
        }
        else {
            total_packets++;
            total_samples += samples_in_this_packet;
        }
    }
    if (total_packets > 0) {
        std::cout << "UDP Sink: sent " << total_samples << " samples in " << total_packets
                  << " packets to " << m_host << ":" << m_port
                  << " (seq: " << m_seq_num << ")" << std::endl;
    }
}

void UDP_Sink_Block::Stop()
{
    cleanup();
    Block::Stop();
}

void UDP_Sink_Block::Run()
{
    BufferReader* reader = GetInputPort(GetInputPortName(0));
    if(!reader || !reader->HasDataAvailable()) {
        return;
    }

    std::vector<SystemVueModelBuilder::EnvelopeSignal> input_data;
    if(!reader->ReadData(input_data) || input_data.empty()) {
        return;
    }

    SendData(input_data);
}

void UDP_Sink_Block::SendEof()
{
    std::lock_guard<std::mutex> lock(m_socket_mutex);

    //发送3个空包作为EOF信号
    for(int i = 0; i < 3; i++) {
        m_udp_socket->writeDatagram("", 0, m_dest_address, m_port);
    }

    std::cout << "UDP Sink: Sent EOF packets" << std::endl;
}

void UDP_Sink_Block::SetDestination(const std::string &host, int port)
{
    std::lock_guard<std::mutex> lock(m_socket_mutex);

    m_host = host;
    m_port = port;

    m_dest_address = QHostAddress(QString::fromStdString(m_host));
    if(m_dest_address.isNull()) {
        QHostInfo info = QHostInfo::fromName(QString::fromStdString(m_host));
        if(!info.addresses().isEmpty()) {
            m_dest_address = info.addresses().first();
        }
    }

    DetermineIPVersion();

    if(m_running) {
        //重新连接
        ConnectSocket();
    }
}

void UDP_Sink_Block::SetPayloadSize(int size)
{
    std::lock_guard<std::mutex> lock(m_socket_mutex);

    if(size >= 8 + m_header_size) { //至少需要容纳一个复数样本
        m_payload_size = size;
        m_send_buffer.reserve(m_payload_size);
    }
    else {
        std::cerr << "Payload size is too small, must to be at least 8 bytes" << std::endl;
    }
}

void UDP_Sink_Block::SetSendEof(bool send_eof)
{
    m_send_eof = send_eof;
}

void UDP_Sink_Block::SetHeaderType(int header_type)
{
    std::lock_guard<std::mutex> lock(m_socket_mutex);

    int old_header_size = m_header_size;

    switch(header_type) {
    case HEADERTYPE_SEQNUM:
        m_header_type = HEADERTYPE_SEQNUM;
        m_header_size = sizeof(HeaderSeqNum);
        break;
    case HEADERTYPE_SEQPLUSSIZE:
        m_header_type = HEADERTYPE_SEQPLUSSIZE;
        m_header_size = sizeof(HeaderSeqPlusSize);
        break;
    case HEADERTYPE_NONE:
        m_header_type = HEADERTYPE_NONE;
        m_header_size = 0;
        break;
    default:
        std::cerr << "UDP Sink: Unkown header type, using NONE" << std::endl;
        m_header_type = HEADERTYPE_NONE;
        m_header_size = 0;
        break;
    }

    //如果头部大小变化，重新验证载荷大小
    if(m_header_size != old_header_size && m_payload_size < 8 + m_header_size) {
        std::cout << "UDP Sink: Adjusting payload size due to header change" << std::endl;
        m_payload_size = 8 + m_header_size;
        m_send_buffer.reserve(m_payload_size);
    }
}

void UDP_Sink_Block::cleanup()
{
    m_running = false;

    if(m_send_eof && m_udp_socket && m_udp_socket->isOpen()) {
        SendEof();
    }
    if(m_udp_socket) {
        if(m_udp_socket->isOpen()) {
            m_udp_socket->close();
        }
        delete m_udp_socket;
        m_udp_socket = nullptr;
    }
}

