#include "UDP_Source_Block.h"
#include "UDP_Sink_Block.h"
#include <QtEndian>
#include <QHostInfo>
#include <QThread>
#include <string.h>
#include <QTextCodec>

UDP_Source_Block::UDP_Source_Block(const std::string &name, size_t itemsize, size_t veclen,
                                   int port, int header_type,
                                   int payload_size,
                                   bool notify_missed,
                                   bool source_zeros,
                                   bool ipv6)
    : Block(name),
    m_port(port),
    m_payload_size(payload_size),
    m_notify_missed(notify_missed),
    m_source_zeros(source_zeros),
    m_ipv6(ipv6),
    m_running(false),
    m_itemsize(itemsize),
    m_veclen(veclen),
    m_header_type(header_type),
    m_header_size(0),
    m_seq_num(0),
    m_expected_seq_num(0),
    m_missed_packets(0),
    m_udp_socket(nullptr),
    m_send_socket(nullptr),  // 初始化发送socket
    m_send_port(0),         // 初始化发送端口
    m_total_bytes_received(0),
    m_total_packets_received(0),
    m_total_packets_dropped(0),
    m_radarControl(nullptr),
    m_radarDataReceiver(nullptr),
    m_radarSignalProcessor(nullptr),
    m_servoControl(nullptr),
    m_hostCommunication(nullptr),
    m_systemInitialized(false),
    m_controlThreadRunning(false)
{
    SetBlockType(Block::BlockType::SOURCE);

    // 计算块大小
    m_block_size = m_itemsize * m_veclen;

    // 初始化包头大小
    switch(m_header_type) {
    case SOURCEHEADERTYPE_SEQNUM:
        m_header_size = sizeof(SourceHeaderSeqNum);
        break;
    case SOURCEHEADERTYPE_SEQPLUSSIZE:
        m_header_size = sizeof(SourceHeaderSeqPlusSize);
        break;
    case SOURCEHEADERTYPE_NONE:
        m_header_size = 0;
        break;
    default:
        std::cerr << "UDP Source: Unkown header type, using NONE" << std::endl;
        m_header_type = HEADERTYPE_NONE;
        m_header_size = 0;
        break;
    }

    // 验证负载大小
    if (!ValidatePayloadSize()) {
        std::cerr << "UDP Source: Invalid payload size, setting to minimum" << std::endl;
        m_payload_size = m_block_size + m_header_size;
        if (m_payload_size < 8) m_payload_size = 8;
    }

    Initialize();

    m_udp_socket = new QUdpSocket();
    m_receive_buffer.reserve(m_payload_size * 2);

    memset((void*)&m_localData, 0, sizeof(LocalData));


}

UDP_Source_Block::~UDP_Source_Block()
{
    cleanup();
}

void UDP_Source_Block::Setup()
{
    Block::Setup();

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    qDebug() << "雷达与伺服控制系统启动";

    // std::cout << "=== 端口配置信息 ===" << std::endl;
    // std::cout << "receive port: " << m_port << " (listening)" << std::endl;
    // std::cout << "send target: " << m_send_address.toString().toStdString()
    //           << ":" << m_send_port << " (sending)" << std::endl;
    // std::cout << "===================" << std::endl;

    m_radarControl = new RadarControl();
    m_radarDataReceiver = new RadarDataReceiver();
    m_radarSignalProcessor = new RadarSignalProcessor();
    m_servoControl = new ServoControl();
    m_hostCommunication = new HostCommunication();

    try {

        qDebug() << "\n1. 接收初始化...";
        m_radarControl->radarInit(m_localData);

        qDebug() << "\n2. 雷达启停控制...";
        if(!m_radarControl->radarStartStop(m_localData)) {
            throw std::runtime_error("雷达启动失败");
        }
        qDebug() << "雷达启动成功";

        // 3. 数据接受协议设定
        qDebug() << "\n3. 数据接受协议设定...";
        if (m_radarControl->configureDCA1000()) {
            qDebug() << "数据接受协议设定成功";
            m_localData.DAC1000_cfg = 1;
        } else {
            qDebug() << "数据接受协议设定失败";
            return;
        }


        // // 4. 雷达回波数据接收
        // qDebug() << "\n4. 雷达回波数据接收...";
        // QVector<double> radarData = m_radarDataReceiver->receiveRadarData(m_localData);

        // if (radarData.isEmpty()) {
        //     qDebug() << "未接收到雷达数据";
        // } else {
        //     qDebug() << "接收到雷达数据，长度:" << radarData.size();

        //     // 5. 雷达信号处理
        //     qDebug() << "\n5. 雷达信号处理...";
        //     m_radarSignalProcessor->processRadarData(radarData, m_localData);

        //     qDebug() << "目标信息：";
        //     qDebug() << "  角度：" << m_localData.Target_Angle;
        //     qDebug() << "  距离：" << m_localData.Target_Range;
        // }

        // // 3. 初始化发送socket
        // std::cout << "\n3. 初始化UDP发送socket..." << std::endl;
        // if(!InitSendSocket()) {
        //     throw std::runtime_error("发送socket初始化失败");
        // }

        // // 4. 设置发送目标
        // SetSendTarget("127.0.0.1", 9000);
        // std::cout << "send target is : 127.0.0.1:9000" << std::endl;

        // qDebug() << "\n4. 启动UDP数据接收...";
        // if(!BindSocket()) {
        //     throw std::runtime_error("UDP Socket绑定失败");
        // }



        // m_systemInitialized = true;
        // std::cout << "UDP Source Block started on port: " << m_port << std::endl;
        // qDebug() << "雷达与伺服控制系统初始化完成 开始接收数据...";

        // // 使用 Lambda 表达式
        // QAbstractSocket::connect(m_udp_socket, &QUdpSocket::readyRead, [this]() {
        //     if(!m_running || !m_udp_socket) {
        //         return;
        //     }

        //     std::cout << "----------UDP Socket Data has been detected to have arrived..." << std::endl;

        //     while(m_udp_socket->hasPendingDatagrams()) {
        //         QByteArray datagram;
        //         datagram.resize(m_udp_socket->pendingDatagramSize());

        //         QHostAddress sender_address;
        //         quint16 sender_port;

        //         qint64 bytes_read = m_udp_socket->readDatagram(
        //             datagram.data(), datagram.size(),
        //             &sender_address, &sender_port);

        //         if(bytes_read > 0) {
        //             std::cout << "从 " << sender_address.toString().toStdString()
        //                       << ":" << sender_port << " 接收到数据" << std::endl;
        //             ProcessDatagram(datagram);
        //         }
        //     }

        //     ProcessQueueData();
        // });

        // 启动控制线程
        qDebug() << "\n6. 启动控制线程...";
        m_running = true;
        m_controlThreadRunning = true;
        m_systemInitialized = true;
        // m_controlThread = std::thread([this]() {
        //     ControlThreadFunction();
        // });
        ControlThreadFunction();
        // qDebug() << "控制线程启动成功";
    }catch(const std::exception& e) {
        qDebug() << "系统启动失败:" << e.what();
        cleanup();
        throw;
    }
}

void UDP_Source_Block::Stop()
{
    cleanup();
    Block::Stop();
}

void UDP_Source_Block::Run()
{
    CheckForUDPData();
    // 处理队列中的数据并发送
    ProcessQueueData();

    // 发送雷达数据
    SendRadarData();
}

void UDP_Source_Block::SetPort(int port)
{
    std::lock_guard<std::mutex> lock(m_socket_mutex);

    m_port = port;

    if(m_running) {
        BindSocket();
    }
}

void UDP_Source_Block::SetPayloadSize(int size)
{
    std::lock_guard<std::mutex> lock(m_socket_mutex);

    if(size >= 8 + m_header_size) {
        m_payload_size = size;
        m_receive_buffer.reserve(m_payload_size);
    }
    else {
        std::cerr << "Payload is too small, must to be at least 8 bytes" << std::endl;
    }
}

void UDP_Source_Block::SetNotifyMissed(bool notify_missed)
{
    m_notify_missed = notify_missed;
}

void UDP_Source_Block::SetSourceZeros(bool source_zeros)
{
    m_source_zeros = source_zeros;
}

void UDP_Source_Block::SetHeaderType(int header_type)
{
    std::lock_guard<std::mutex> lock(m_socket_mutex);

    int old_header_type = m_header_type;
    switch(header_type) {
    case SOURCEHEADERTYPE_SEQNUM:
        m_header_size = sizeof(SourceHeaderSeqNum);
        break;
    case SOURCEHEADERTYPE_SEQPLUSSIZE:
        m_header_size = sizeof(SourceHeaderSeqPlusSize);
        break;
    case SOURCEHEADERTYPE_NONE:
        m_header_size = 0;
        break;
    default:
        std::cerr << "UDP Source: Unkown header type, using NONE" << std::endl;
        m_header_type = HEADERTYPE_NONE;
        m_header_size = 0;
        break;
    }

    if(m_header_size != old_header_type && m_payload_size < 8 + m_header_size) {
        std::cout << "UDP Source: Adjusting payload size due to header change" << std::endl;
        m_payload_size = 8 + m_header_size;
        m_receive_buffer.reserve(m_payload_size);
    }
}

void UDP_Source_Block::SetIPv6(bool ipv6)
{
    std::lock_guard<std::mutex> lock(m_socket_mutex);

    m_ipv6 = ipv6;

    if(m_running) {
        BindSocket();
    }
}

void UDP_Source_Block::SetItemsize(size_t itemsize)
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);

    m_itemsize = itemsize;
    m_block_size = m_itemsize * m_veclen;

    if(!ValidatePayloadSize()) {
        std::cerr << "UDP Source: Payload size may be invalid with new itemsize" << std::endl;
    }
}

void UDP_Source_Block::SetVeclen(size_t veclen)
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_veclen = veclen;
    m_block_size = m_itemsize * m_veclen;

    if(!ValidatePayloadSize()) {
        std::cerr << "UDP Source: Payload size may be invalid with new veclen" << std::endl;
    }
}

void UDP_Source_Block::Initialize()
{
    AddOutputPort("target_out", 1, Block::DataType::DOUBLE);
    AddOutputPort("radar_out", 1, Block::DataType::DOUBLE);
}


bool UDP_Source_Block::BindSocket()
{
    // 先关闭之前的socket
    if(m_udp_socket) {
        if(m_udp_socket->isOpen()) {
            m_udp_socket->close();
        }
        delete m_udp_socket;
    }

    // 创建新的socket
    m_udp_socket = new QUdpSocket();

    m_udp_socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 1024 * 1024);

    bool bind_success = m_udp_socket->bind(QHostAddress("127.0.0.1"), m_port, QUdpSocket::ShareAddress);

    if(!bind_success) {
        std::cout << "绑定失败: " << m_udp_socket->errorString().toStdString() << std::endl;
        return false;
    }

    QHostAddress local_address = m_udp_socket->localAddress();
    quint16 local_port = m_udp_socket->localPort();

    std::cout << "绑定成功! 地址: " << local_address.toString().toStdString()
              << ":" << local_port << std::endl;


    // 详细检查socket状态
    std::cout << "Socket详细状态:" << std::endl;
    std::cout << "  isOpen: " << m_udp_socket->isOpen() << std::endl;
    std::cout << "  isValid: " << m_udp_socket->isValid() << std::endl;
    std::cout << "  state: " << m_udp_socket->state() << std::endl;
    std::cout << "  localPort: " << m_udp_socket->localPort() << std::endl;
    std::cout << "  localAddress: " << m_udp_socket->localAddress().toString().toStdString() << std::endl;

    return true;

//     if(m_udp_socket->isOpen()) {
//         m_udp_socket->close();
//     }

//     delete m_udp_socket;
//     m_udp_socket = new QUdpSocket();

//     bool bind_success = false;
//     if(m_ipv6) {
//         bind_success = m_udp_socket->bind(QHostAddress(QHostAddress::AnyIPv6), m_port);
//         if(!bind_success) {
//             std::cerr << "UDP Source: Failed to bind IPv6 socket on port " << m_port << std::endl;
//             bind_success = m_udp_socket->bind(QHostAddress(QHostAddress::AnyIPv4), m_port);
//             if(bind_success) {
//                 std::cout << "UDP Source: Fallback to IPv4 binding successful" << std::endl;
//                 m_ipv6 = false;
//             }
//         }
//     }
//     else {
//         // bind_success = m_udp_socket->bind(QHostAddress(QHostAddress::AnyIPv4), m_port);
//         bind_success = m_udp_socket->bind(QHostAddress("127.0.0.1"), m_port);
//         if(!bind_success) {
//             std::cerr << "UDP Source: Failed to bind IPv4 socket on port " << m_port << std::endl;
//         }
//     }

//     if(!bind_success) {
//         std::cerr << "UDP Source: Failed to bind socket on port " << m_port << std::endl;
//         return false;
//     }

//     QHostAddress local_address = m_udp_socket->localAddress();
//     quint16 local_port = m_udp_socket->localPort();
//     std::cout << "UDP Source: Bound to local address " << local_address.toString().toStdString()
//               << ":" << local_port << std::endl;

//     return true;
}

void UDP_Source_Block::ParseHeader(const uint8_t* header_buffer, uint64_t& seq_num, uint32_t& payload_length)
{
    switch(m_header_type) {

    case SOURCEHEADERTYPE_SEQNUM: {
        SourceHeaderSeqNum seq_header;
        std::memcpy(&seq_header, header_buffer, m_header_size);
        m_seq_num = qFromBigEndian(seq_header.seqnum);
        payload_length = 0;
        HandleMissedPackets(m_seq_num);
        break;
    }
    case SOURCEHEADERTYPE_SEQPLUSSIZE: {
        SourceHeaderSeqPlusSize seq_header_plus_size;
        std::memcpy(&seq_header_plus_size, header_buffer, m_header_size);
        m_seq_num = qFromBigEndian(seq_header_plus_size.seqnum);
        payload_length = qFromBigEndian(seq_header_plus_size.length);
        HandleMissedPackets(m_seq_num);
        break;
    default:
        break;
    }
    }
}

void UDP_Source_Block::HandleMissedPackets(uint64_t received_seq_num)
{
    if(m_expected_seq_num == 0) {
        //第一个包，初始化期望序列号
        m_expected_seq_num = received_seq_num + 1;
        return;
    }

    if(received_seq_num > m_expected_seq_num) {
        int missed = received_seq_num - m_expected_seq_num;
        m_missed_packets += missed;
        m_total_packets_dropped += missed;

        if(m_notify_missed) {
            std::cerr << "UDP Source: Missed " << missed << " packets. Total missed: "
                      << m_missed_packets << std::endl;
        }
    }

    m_expected_seq_num = received_seq_num + 1;
}

void UDP_Source_Block::cleanup()
{
    m_running = false;
    m_systemInitialized = false;

    // 停止控制线程
    if (m_controlThreadRunning) {
        std::cout << "停止控制线程..." << std::endl;
        m_controlThreadRunning = false;

        if (m_controlThread.joinable()) {
            m_controlThread.join();
            std::cout << "控制线程已停止" << std::endl;
        }
    }

    if(m_udp_socket) {
        if(m_udp_socket->isOpen()) {
            m_udp_socket->close();
        }
        delete m_udp_socket;
        m_udp_socket = nullptr;
    }

    if(m_send_socket) {
        if(m_send_socket->isOpen()) {
            m_send_socket->close();
        }
        delete m_send_socket;
        m_send_socket = nullptr;
    }

    //清理系统组件
    delete m_radarControl;
    delete m_radarDataReceiver;
    delete m_radarSignalProcessor;
    delete m_servoControl;
    delete m_hostCommunication;

    m_radarControl = nullptr;
    m_radarDataReceiver = nullptr;
    m_radarSignalProcessor = nullptr;
    m_servoControl = nullptr;
    m_hostCommunication = nullptr;

    std::lock_guard<std::mutex> lock(m_queue_mutex);
    while(!m_data_queue.empty()) {
        m_data_queue.pop();
    }
}

void UDP_Source_Block::ProcessDatagram(const QByteArray &datagram)
{
    const int MIN_EXPECTED_SIZE = 58;

    std::cout << "\n=== 接收到UDP数据 ===" << std::endl;
    std::cout << "数据包大小: " << datagram.size() << " 字节" << std::endl;

    if(datagram.size() < MIN_EXPECTED_SIZE) {
        std::cout << "!!! 数据包太小，期望至少 " << MIN_EXPECTED_SIZE << " 字节" << std::endl;
        // 但还是处理小数据包
    }

    std::lock_guard<std::mutex> lock(m_queue_mutex);

    // 解析包头
    uint64_t seq_num = 0;
    uint32_t payload_length = 0;

    // 需要自定义包头时解析头部
    if(m_header_type != HEADERTYPE_NONE && datagram.size() >= static_cast<int>(m_header_size)) {
        ParseHeader(reinterpret_cast<const uint8_t*>(datagram.data()), seq_num, payload_length);
        std::cout << "解析到包头 - 序列号: " << seq_num << ", 负载长度: " << payload_length << std::endl;
    }

    // 计算数据起始位置
    size_t header_offset = (m_header_type != HEADERTYPE_NONE) ? m_header_size : 0;
    size_t data_size = datagram.size() - header_offset;
    const uint8_t* data_start = reinterpret_cast<const uint8_t*>(datagram.data()) + header_offset;

    std::cout << "数据部分大小: " << data_size << " 字节" << std::endl;

    // 打印原始字节数据（前32字节）
    std::cout << "原始数据(前32字节): ";
    for(int i = 0; i < std::min(32, datagram.size()); i++) {
        printf("%02X ", static_cast<unsigned char>(datagram[i]));
    }
    std::cout << std::endl;

    // 尝试解析为标准格式
    if(datagram.size() >= 58) {
        std::cout << "尝试解析为标准雷达数据格式..." << std::endl;
        const uint8_t* data_ptr = reinterpret_cast<const uint8_t*>(datagram.data());

        // 检查数据头
        if(data_ptr[0] == 90 && data_ptr[1] == 92) {
            std::cout << "检测到标准数据头 [90 92]" << std::endl;
            data_ptr += 2;

            // 提取目标数据
            double target_angle, target_range;
            std::memcpy(&target_angle, data_ptr, sizeof(double));
            data_ptr += sizeof(double);
            std::memcpy(&target_range, data_ptr, sizeof(double));
            data_ptr += sizeof(double);

            std::cout << "解析结果:" << std::endl;
            std::cout << "  目标角度: " << target_angle << " 度" << std::endl;
            std::cout << "  目标距离: " << target_range << " 米" << std::endl;

            // 提取雷达参数
            std::cout << "  雷达参数: ";
            for(int i = 0; i < 10 && data_ptr < reinterpret_cast<const uint8_t*>(datagram.data()) + datagram.size(); i++) {
                double param;
                if(data_ptr + sizeof(double) <= reinterpret_cast<const uint8_t*>(datagram.data()) + datagram.size()) {
                    std::memcpy(&param, data_ptr, sizeof(double));
                    std::cout << param << " ";
                    data_ptr += sizeof(double);
                }
            }
            std::cout << std::endl;
        } else {
            std::cout << "非标准数据头，按原始数据处理" << std::endl;
            // 尝试按字符串显示
            std::string received_str = datagram.toStdString();
            if(received_str.length() > 0) {
                std::cout << "字符串内容: " << received_str << std::endl;
            }
        }
    } else {
        // 小数据包，直接显示内容
        std::string received_str = datagram.toStdString();
        std::cout << "数据内容: " << received_str << std::endl;
    }

    // 将数据放入队列(原始字节)
    std::vector<uint8_t> raw_data(data_start, data_start + data_size);
    m_data_queue.push(std::move(raw_data));

    m_total_bytes_received += data_size;
    m_total_packets_received++;

    std::cout << "数据已存入队列，总接收包数: " << m_total_packets_received << std::endl;
    std::cout << "=== 数据处理完成 ===\n" << std::endl;
}

void UDP_Source_Block::ProcessQueueData()
{
    std::lock_guard<std::mutex> lock(m_queue_mutex);

    Buffer* TargetBuffer = GetOutputPort(GetOutputPortName(0));
    Buffer* RadarBuffer = GetOutputPort(GetOutputPortName(1));

    if(m_data_queue.empty()) {
        if(m_source_zeros) {
            //输出零值到两个端口
            if(TargetBuffer) {
                std::vector<double> zeros(2, 0.0);
                TargetBuffer->WriteData(zeros);
            }

            if(RadarBuffer) {
                std::vector<float> zeros(10, 0.0f);
                RadarBuffer->WriteData(zeros);
            }
        }
    }

    size_t processed_packets = 0;

    while(!m_data_queue.empty()) {
        const auto& packet = m_data_queue.front();

        if(packet.size() < 58) {
            std::cerr << "UDP Source: Packet too small for parsing. Size: "
                      << packet.size() << std::endl;
            m_data_queue.pop();
            continue;
        }

        bool target_success = false;
        bool radar_success = false;

        //解析数据包
        const uint8_t* data_ptr = packet.data();

        //跳过包头
        data_ptr += 2;

        //提取目标数据
        std::vector<double> target_data;
        target_data.reserve(2);

        double target_angle, target_range;
        std::memcpy(&target_angle, data_ptr, sizeof(double));
        data_ptr += sizeof(double);
        std::memcpy(&target_range, data_ptr, sizeof(double));
        data_ptr += sizeof(double);

        target_data.push_back(target_angle);
        target_data.push_back(target_range);

        //提取雷达数据
        std::vector<float> radar_param;
        radar_param.reserve(10);

        for(int i = 0; i < 10; i++) {
            float param;
            std::memcpy(&param, data_ptr, sizeof(float));
            data_ptr += sizeof(float);
            radar_param.push_back(param);
        }

        //写入目标数据到输出端口
        if(TargetBuffer && !target_data.empty()) {
            TargetBuffer->WriteData(target_data);
            target_success = true;

            std::cout << "UDP Source: Target Data - Angle: " << target_angle
                      << ", Range: " << target_range << std::endl;
        }

        //写入雷达数据到输出端口
        if(RadarBuffer && !radar_param.empty()) {
            RadarBuffer->WriteData(radar_param);
            radar_success = true;

            std::cout << "UDP Source: Radar Params - StartFreq: " << radar_param[0]
                      << ", FreqSlope: " << radar_param[1]
                      << ", SampleRate: " << radar_param[2] << std::endl;
        }

        if(target_success && radar_success) {
            processed_packets++;

            if(processed_packets % 10 == 0) {
                std::cout << "UDP Source: Successfully processed " << processed_packets
                          << " packets" << std::endl;
            }
        }
        else {
            std::cerr << "UDP Source: Failed to write data to output buffers" << std::endl;
        }
        // 移除已处理的数据包
        m_data_queue.pop();
    }

    if(processed_packets > 0) {
        std::cout << "UDP Source: Batch processed " << processed_packets << " packets" << std::endl;
    }


}

void UDP_Source_Block::ProcessRadarDataAndControl()
{
    if(!m_systemInitialized || !m_running) {
        return;
    }

    try {
        // 4. 雷达回波数据接收
        qDebug() << "\n4. 雷达回波数据接收...";
        QVector<double> radarData = m_radarDataReceiver->receiveRadarData(m_localData);

        if (radarData.isEmpty()) {
            qDebug() << "未接收到雷达数据";
        } else {
            qDebug() << "接收到雷达数据，长度:" << radarData.size();

            // 5. 雷达信号处理
            qDebug() << "\n5. 雷达信号处理...";
            m_radarSignalProcessor->processRadarData(radarData, m_localData);

            qDebug() << "目标信息：";
            qDebug() << "  角度：" << m_localData.Target_Angle;
            qDebug() << "  距离：" << m_localData.Target_Range;
        }

        // 6. 控制信号发送
        qDebug() << "\n6. 控制信号发送...";
        // 设置控制参数示例
           m_localData.Radar_mode_set = 1;      // 机扫模式
           m_localData.Work_mode = 2;           // 相对模式
           m_localData.Rotation_Angle = 10;     // 旋转角度10度
           m_localData.Rotation_Velocity = 5;   // 旋转速度5度/秒
           m_localData.Turntable_Connet = 1;    // 转台连接

        if (m_servoControl->sendControlSignal(m_localData)) {
            qDebug() << "控制信号发送成功";
        } else {
            qDebug() << "控制信号发送失败";
        }

        // 7. 接收伺服状态
        qDebug() << "\n7. 接收伺服状态...";
        m_servoControl->receiveServoState(m_localData);

        // 8. 读取雷达参数
        qDebug() << "\n8. 读取雷达参数...";
        RadarConfig radarConfig = m_hostCommunication->readRadarParameters(m_localData.configurationFileName);

        // 9. 发送数据到上位机
        qDebug() << "\n9. 发送数据到上位机...";
        if (m_hostCommunication->sendToHost(m_localData, radarConfig)) {
            qDebug() << "数据发送到上位机成功";
        } else {
            qDebug() << "数据发送到上位机失败";
        }

    } catch (const std::exception& e) {
        qDebug() << "数据处理与控制出错:" << e.what();
    }

    // if(!m_systemInitialized || !m_running) {
    //     return;
    // }

    // static int simulationCounter = 0;
    // simulationCounter++;

    // try {
    //     // 1. 生成模拟雷达数据
    //     m_localData.Target_Angle = 30.0 + 10.0 * sin(simulationCounter * 0.1);
    //     m_localData.Target_Range = 50.0 + 20.0 * sin(simulationCounter * 0.05);
    //     m_localData.Abs_Angle = static_cast<int>(m_localData.Target_Angle);
    //     m_localData.Angle_velocity = 5;

    //     // 2. 输出模拟数据信息
    //     std::cout << "生成模拟目标数据 - 周期[" << simulationCounter << "]: " << std::endl;
    //     std::cout << "  角度: " << m_localData.Target_Angle << " 度" << std::endl;
    //     std::cout << "  距离: " << m_localData.Target_Range << " 米" << std::endl;

    //     // 3. 设置模拟控制参数
    //     m_localData.Radar_mode_set = 1;
    //     m_localData.Work_mode = 2;
    //     m_localData.Rotation_Angle = 10;
    //     m_localData.Rotation_Velocity = 5;
    //     m_localData.Turntable_Connet = 1;

    //     // 4. 发送模拟控制信号（可选）
    //     std::cout << "发送模拟控制信号..." << std::endl;
    //     // 注释掉实际的硬件调用
    //     // if (m_servoControl->sendControlSignal(m_localData)) {
    //     //     std::cout << "控制信号发送成功" << std::endl;
    //     // }

    //     // 5. 发送数据到上位机
    //     std::cout << "发送数据到上位机..." << std::endl;
    //     SendRadarData();

    //     std::cout << "第 " << simulationCounter << " 个处理周期完成" << std::endl;

    // } catch (const std::exception& e) {
    //     std::cout << "数据处理出错:" << e.what() << std::endl;
    // }
}

bool UDP_Source_Block::ValidatePayloadSize() const
{
    if(m_payload_size < 8) {
        return false;
    }

    if(m_payload_size < static_cast<int>(m_header_size + m_block_size)) {
        return false;
    }

    return true;
}

void UDP_Source_Block::SetSendTarget(const std::string& host, int port)
{
    std::lock_guard<std::mutex> lock(m_socket_mutex);

    m_send_address = QHostAddress(QString::fromStdString(host));
    if(m_send_address.isNull()) {
        QHostInfo info = QHostInfo::fromName(QString::fromStdString(host));
        if(!info.addresses().isEmpty()) {
            m_send_address = info.addresses().first();
        }
    }
    m_send_port = port;

    std::cout << "UDP Source: Set send target to " << host << ":" << port << std::endl;
}

bool UDP_Source_Block::InitSendSocket()
{
    if(!m_send_socket) {
        m_send_socket = new QUdpSocket();
    }

    if(m_send_socket->isOpen()) {
        m_send_socket->close();
    }

    // 绑定发送socket到任意端口
    bool bind_success = m_send_socket->bind(QHostAddress(QHostAddress::AnyIPv4), 0);
    if(!bind_success) {
        std::cerr << "UDP Source: Failed to bind send socket" << std::endl;
        return false;
    }

    std::cout << "UDP Source: Send socket bound to port " << m_send_socket->localPort() << std::endl;
    return true;
}

QByteArray UDP_Source_Block::BuildRadarPacket()
{
    QByteArray byteStream;

    // 数据头 (2字节)
    qint8 data_head[] = {90, 92}; // 'Z' and '\'
    byteStream.append(reinterpret_cast<const char*>(data_head), sizeof(data_head));

    // 目标数据 (2个double, 16字节)
    double data_target[] = {m_localData.Target_Angle, m_localData.Target_Range};
    byteStream.append(reinterpret_cast<const char*>(data_target), sizeof(data_target));

    // 雷达参数 (10个double, 80字节) - 将float改为double
    double data_para[] = {
        m_localData.Target_Angle,      // 使用目标角度
        m_localData.Target_Range,      // 使用目标距离
        static_cast<double>(m_localData.Abs_Angle),     // 绝对角度
        static_cast<double>(m_localData.Angle_velocity), // 角速度
        77.0,           // 模拟起始频率
        15.2,           // 模拟频率斜率
        10.0,           // 模拟采样率
        200.0,          // 模拟最大距离
        0.5,            // 模拟距离分辨率
        50.0            // 模拟最大速度
    };
    byteStream.append(reinterpret_cast<const char*>(data_para), sizeof(data_para));

    return byteStream;
}

void UDP_Source_Block::SendRadarData()
{
    if(!m_running || !m_send_socket || m_send_address.isNull() || m_send_port == 0) {
        return;
    }

    // 构建雷达数据包
    QByteArray radarPacket = BuildRadarPacket();

    // 发送数据包
    qint64 bytes_sent = m_send_socket->writeDatagram(
        radarPacket,
        m_send_address,
        m_send_port);

    if(bytes_sent == -1) {
        std::cerr << "UDP Source: Send error - " << m_send_socket->errorString().toStdString() << std::endl;
    } else {
    }
}


void UDP_Source_Block::ControlThreadFunction()
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    qDebug() << "控制线程开始运行";

    int cycleCount = 0;
    const int PROCESS_INTERVAL_MS = 10000; // 10秒间隔，专注于接收测试

    while (m_controlThreadRunning && m_running) {
        try {
            cycleCount++;

            if (cycleCount % 5 == 0) {
                qDebug() << "\n=== 发送模拟数据（周期 " << cycleCount << "）===";
                ProcessRadarDataAndControl();
            } else {
                // 主要时间用于接收数据
                qDebug() << "等待接收数据... (" << cycleCount << ")";
            }

            // 休眠指定时间
            std::this_thread::sleep_for(std::chrono::milliseconds(PROCESS_INTERVAL_MS));

        } catch (const std::exception& e) {
            qDebug() << "控制线程出错: " << e.what();
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        }
    }

    qDebug() << "控制线程退出";
}

void UDP_Source_Block::CheckForUDPData()
{
    if(!m_running || !m_udp_socket) {
        return;
    }

    // 手动检查是否有数据
    if(m_udp_socket->hasPendingDatagrams()) {
        std::cout << "!!! 发现UDP数据包 !!!" << std::endl;

        QByteArray datagram;
        qint64 datagramSize = m_udp_socket->pendingDatagramSize();
        datagram.resize(datagramSize);

        QHostAddress sender_address;
        quint16 sender_port;

        qint64 bytes_read = m_udp_socket->readDatagram(
            datagram.data(), datagram.size(),
            &sender_address, &sender_port);

        if(bytes_read > 0) {
            std::cout << "读取到UDP数据: " << bytes_read << " 字节" << std::endl;
            std::cout << "来自: " << sender_address.toString().toStdString()
                      << ":" << sender_port << std::endl;
            ProcessDatagram(datagram);
        }
    }
}

















