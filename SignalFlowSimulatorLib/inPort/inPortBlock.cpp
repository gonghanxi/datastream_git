#include "inPortBlock.h"
#include <QDebug>
#include <QFileInfo>
#include <utility>

#include "inPortModelInfo.h"

// 包含各种Buffer类型
#include "CircularBuffer.h"

namespace SystemVueModelBuilder {

inPortBlock::inPortBlock(const std::string &name)
    :Block(name)
{
    SetBlockType(BlockType::SOURCE);
    while(!m_intQueue.empty()) m_intQueue.pop();
    while(!m_doubleQueue.empty()) m_doubleQueue.pop();
    while(!m_dcomplexQueue.empty()) m_dcomplexQueue.pop();
}

inPortBlock::~inPortBlock()
{
    qDebug() << "inPortBlock::~inPortBlock - 实例:" << m_instanceName;
}

void inPortBlock::addPortInfo(const PortMsg& port)
{
    qDebug() << "addPortInfo - port: " << port.name;
    qDebug() << "addPortInfo - port putType: " << port.putType;
    PortInfo info;
    info.id = port.id;
    info.name = port.name;
    info.putType = port.putType;
    info.dataType = port.dataType;
    info.portRate = port.portRate;
    info.isOptional = port.isOptional;
    info.topProtId = port.topProtId;

    m_portInfos[port.id] = info;

    if (port.putType == "in") {
        m_inputPortNames.append(port.name);
    } else if (port.putType == "out") {
        m_outputPortNames.append(port.name);
    }
}

void inPortBlock::addParameterInfo(const QString& paramName, const QString& value)
{
    ParamInfo info;
    info.name = paramName;
    info.value = value;
    m_paramInfos[paramName] = info;
}

bool inPortBlock::createPorts()
{
    for(auto& blockInfos : AlgorithmManager::createInstance()->getBlocksInfo()) {
        for(auto& blockinfo : blockInfos) {
            if(blockinfo.cmpType == "inPort") {
                for(auto& port : blockinfo.portsMsg) {
                    IntCircularBuffer* buffer = new IntCircularBuffer();
                    blockinfo.block->AddOutputPort(port.name.toStdString(), *buffer, 1, DataType::CIRCULAR_BUFFER_INT);
                    m_block = blockinfo.block;
                }

            }
        }
    }
    m_outputPortNames.push_back(QString::fromStdString(m_block->GetOutputPortName(0)));
    return true;
}

bool inPortBlock::Initialize()
{
    qDebug() << "inPortBlock::Initialize - 实例:" << m_instanceName;

    // 创建输入输出端口
    if (!createPorts()) {
        LOG_ERROR("inPort创建端口失败:", m_instanceName.toStdString());
        return false;
    }

    m_isInitialized = true;
    return true;
}

bool inPortBlock::Setup()
{
    std::cout << "inPortBlock::Setup - 实例:" << m_instanceName.toStdString() << std::endl;

    if (!m_isInitialized) {
        LOG_ERROR("inPortBlock未初始化:", m_instanceName.toStdString());
        return false;
    }
    // 从事件处 获取参数

    // 最大允许的输出数量
//    const int MAX_OUTPUT_SIZE = 1827;
    const int TARGET_BYTE_COUNT = 14656;
    const int BITES_PER_BYTE = 8;

    // 需要对于Sink收集器进行判断
    Block::DataType SinkDataType = Block::DataType::CIRCULAR_BUFFER_INT;
    for(auto& blockInfos : AlgorithmManager::createInstance()->getBlocksInfo()) {
        for(const auto& blockinfo : blockInfos) {
            if(blockinfo.cmpCategory == "Sinks") {
                SinkDataType = blockinfo.block->GetInputPort(blockinfo.block->GetInputPortName(0))->GetDataType();
            }
        }
    }


    Block::DataType datatype = GetOutputPort(GetOutputPortName(0))->GetDataType();
    // 确定每个值占用的比特数
    int bitsPerValue = 0;
    if(datatype == Block::DataType::CIRCULAR_BUFFER_DOUBLE) {
        if(SinkDataType == Block::DataType::TIMED_INT) {
            bitsPerValue = 1;
        }
        else if(SinkDataType == Block::DataType::TIMED_DOUBLE) {
            bitsPerValue = 64;
        }
        else if(SinkDataType == Block::DataType::TIMED_DCOMPLEX || SinkDataType == Block::DataType::ENVELOPE_SIGNAL ) {
            bitsPerValue = 128;
        }
    }
    // 以 int 类型进来，就是bit 流，不需要转换为int
    else if(datatype == Block::DataType::CIRCULAR_BUFFER_INT) {
        if(SinkDataType == Block::DataType::TIMED_INT) {
            bitsPerValue = 1;
        }
        else if(SinkDataType == Block::DataType::TIMED_DOUBLE) {
            bitsPerValue = 64;
        }
        else if(SinkDataType == Block::DataType::TIMED_DCOMPLEX || SinkDataType == Block::DataType::ENVELOPE_SIGNAL ) {
            bitsPerValue = 128;
        }
    }
    else if(datatype == Block::DataType::CIRCULAR_BUFFER_DCOMPLEX) {
        if(SinkDataType == Block::DataType::TIMED_INT) {
            bitsPerValue = 1;
        }
        else if(SinkDataType == Block::DataType::TIMED_DOUBLE) {
            bitsPerValue = 64;
        }
        else if(SinkDataType == Block::DataType::TIMED_DCOMPLEX || SinkDataType == Block::DataType::ENVELOPE_SIGNAL ) {
            bitsPerValue = 128;
        }
    }
    else {
        bitsPerValue = 32;//默认为int
    }

    // 需要生成的数值个数
    int totalBitsNeeded = TARGET_BYTE_COUNT * BITES_PER_BYTE;
    int numValues = totalBitsNeeded / bitsPerValue;

    //读取bits流
    std::string PORT_value = getParameter("PORT").Value;

    if(PORT_value.empty()) {
        return true;
    }

    qDebug() << "inPortBlock::Setup - PORT_value is empty or not:" << (PORT_value.empty() ? "true" : "false");
    qDebug() << "inPortBlock::Setup - PORT_value size:" << PORT_value.size();
    QString port_value = QString::fromStdString(PORT_value);
    qDebug() << "inPortBlock::Setup - port_value:" << port_value;
    QStringList groups = port_value.split(' ');

    qDebug() << "inPortBlock::Setup - groups is empty or not:" << (groups.empty() ? "true" : "false");

    std::vector<int> bits;
    // 遍历所有的 bit 组
//    for(int i=0; i < groups.size(); i++) {
//        QChar ch = groups
//        int bit = groups[i].toInt();
//        qDebug() << "inPortBlock::Setup - dealing with groups[i]:" << groups[i];
//        qDebug() << "inPortBlock::Setup - dealing with bit:" << bit;
//        if(bit != 0 && bit != 1) bit = 0;
//        bits.push_back(bit);
//    }
    for(int i = 0; i < port_value.size(); i++) {
        QChar ch = port_value[i];
        if(ch.isSpace()) continue;
        int bit = (ch == '1') ? 1 : 0;
        bits.push_back(bit);
    }

    //特殊处理：当接收到14696个字节时，即为 处理衰减链路 14656个字节为原始数据， 40个字节为 5个衰减double数据
    // 获取连接的下游 处理衰减信号的模型，截取后40个字符数据直接调用写入函数 发给模型，其余14656个字节原始数据照常处理
    if(GetOutputPort(GetOutputPortName(0))->GetReaderCount() > 1) {
        for(const auto& reader : GetOutputPort(GetOutputPortName(0))->GetReaders()) {
            if(reader->GetName() == "Complex" && reader->GetDataType() == DataType::CIRCULAR_BUFFER_DCOMPLEX) {

            }
        }
    }

    qDebug() << "inPortBlock::Setup - deal with before bits:" << bits;


    //补零至所需比特数

    if(bits.size() < totalBitsNeeded) {
        bits.resize(totalBitsNeeded, 0);
    }
    else if(bits.size() > totalBitsNeeded) {
        bits.resize(totalBitsNeeded);
    }

    qDebug() << "inPortBlock::Setup - deal with after bits:" << bits;
    qDebug() << "inPortBlock::Setup - bitsPerValue:" << bitsPerValue;
    qDebug() << "inPortBlock::Setup - numValues:" << numValues;

    if(datatype == Block::DataType::CIRCULAR_BUFFER_INT) {
        std::queue<int> &outputQueue = m_intQueue;
        for(int i = 0; i < numValues; i++) {
            unsigned int value = 0;
//            for(int j = 0; j < bitsPerValue; j++) {
//                unsigned int bit = bits[i * bitsPerValue + j];
//                value = (value << 1) | bit;
//            }
            value = bits[i];
            outputQueue.push(value);
            qDebug() << QString("int value %1 - [%2]").arg(i).arg(value);
        }
        qDebug() << "inPortBlock::Setup - int outputQueue size: " << outputQueue.size();
    }
    else if(datatype == Block::DataType::CIRCULAR_BUFFER_DCOMPLEX) {
        std::queue<std::complex<double>> &outputQueue = m_dcomplexQueue;
        for(int i = 0; i < numValues; i++) {
            uint64_t realRaw = 0;
            uint64_t imagRaw = 0;

            //bits 流读取顺序 需要反转 将 从高到低 改为 从低到高
            int baseIdx = i * 128;
            //实部
            //逆序读取
            for(int j = 63; j >= 0; --j) {
                realRaw = (realRaw << 1) | bits[baseIdx + j];
            }
//            for(int j = 0; j < 64; ++j) {
//                int bit = bits[i * bitsPerValue + j];
//                realRaw = (realRaw << 1) | bit;
//            }
            //虚部
            //逆序读取
            for(int j = 63; j >= 0; --j) {
                imagRaw = (imagRaw << 1) | bits[baseIdx + 64 + j];
            }
//            for(int j = 64; j < 128; ++j) {
//                int bit = bits[i * bitsPerValue + j];
//                imagRaw = (imagRaw << 1) | bit;
//            }
            double real, imag;
            memcpy(&real, &realRaw, sizeof(double));
            memcpy(&imag, &imagRaw, sizeof(double));
            outputQueue.push(std::complex<double>(real, imag));
            qDebug() << "realRaw hex: " << QString::number(realRaw, 16);
            qDebug() << "imagRaw hex: " << QString::number(imagRaw, 16);
            qDebug() << QString("complex value %1 - [%2,%3]").arg(i).arg(real).arg(imag);
        }
        qDebug() << "inPortBlock::Setup - dcomplex outputQueue size: " << outputQueue.size();
    }
    else if(datatype == Block::DataType::CIRCULAR_BUFFER_DOUBLE) {
        std::queue<double> &outputQueue = m_doubleQueue;
        for(int i = 0; i < numValues; i++) {
            uint64_t raw = 0;
            for(int j = 0; j < bitsPerValue; j++) {
                int bit = bits[i * bitsPerValue + j];
                raw = (raw << 1) | bit;
            }
            double value;
            memcpy(&value, &raw, sizeof(double));
            outputQueue.push(value);
            qDebug() << QString("double value %1 - [%2]").arg(i).arg(value);
        }
        qDebug() << "inPortBlock::Setup - double outputQueue size: " << outputQueue.size();
    }

    if(datatype == Block::DataType::CIRCULAR_BUFFER_DOUBLE) {
        std::cout << "inPortBlock::Setup - m_doubleQueue size: " << m_doubleQueue.size() << std::endl;
        setEventSize(m_doubleQueue.size());
    }
    else if(datatype == Block::DataType::CIRCULAR_BUFFER_INT)  {
        std::cout << "inPortBlock::Setup - m_intQueue size: " << m_intQueue.size() << std::endl;
        setEventSize(m_intQueue.size());
    }
    else if(datatype == Block::DataType::CIRCULAR_BUFFER_DCOMPLEX) {
        std::cout << "inPortBlock::Setup - m_dcomplexQueue size: " << m_dcomplexQueue.size() << std::endl;
        setEventSize(m_dcomplexQueue.size());
    }

    // 重置当前步数和时间
    m_currentStep = 0;
    m_currentTime = m_startTime;
    m_startTime = getSimu().startTime;
    m_stopTime = getSimu().stopTime;
    m_timeInterval = getSimu().time_Interval;
    m_numSamples = getSimu().num_Samples;
    m_samplingRate = getSimu().samplingRate;
    qDebug() << "inPortBlock::Setup - startTime: " << getSimu().startTime;
    qDebug() << "inPortBlock::Setup - stopTime: " << getSimu().stopTime;
    qDebug() << "inPortBlock::Setup - time_Interval: " << getSimu().time_Interval;
    qDebug() << "inPortBlock::Setup - num_Samples: " << getSimu().num_Samples;
    qDebug() << "inPortBlock::Setup - samplingRate: " << getSimu().samplingRate;

    Block::Setup();

    m_isSetup = true;
    return true;
}

bool inPortBlock::Run()
{
    if (!m_isSetup) {
        LOG_ERROR("inPortBlock未Setup:", m_instanceName.toStdString());
        return false;
    }

    // 3. 从inPort读取输出并写入输出Buffer
    qDebug() << "inPortBlock::readOutputsAndWrite - run begin";
    qDebug() << "m_outputPortNames size: " << m_outputPortNames.size();

    // 将获取到的值写入输出Buffer
//    size_t intIndex = 0;
    //    size_t boolIndex = 0;
    //    size_t stringIndex = 0;

    for (const QString& portName : m_outputPortNames) {
        // 获取输出端口信息
        // 获取输出Buffer
        Buffer* buffer = GetOutputPort(portName.toStdString());
        if (!buffer) {
            qDebug() << "inPort输出端口未连接:" << portName;
            continue;
        }
        if(buffer->GetDataType() == DataType::CIRCULAR_BUFFER_INT) {
            int outputValue = m_intQueue.front();
            m_intQueue.pop();
            std::vector<int> data;
            data.push_back(outputValue);
            buffer->WriteData(data);
            std::cout << "写入输出(Int):" << portName.toStdString() << "值:" << data[0] << std::endl;
        }
        else if(buffer->GetDataType() == DataType::CIRCULAR_BUFFER_DOUBLE) {
            double outputValue = m_doubleQueue.front();
            m_doubleQueue.pop();
            std::vector<double> data;
            data.push_back(outputValue);
            buffer->WriteData(data);
            std::cout << "写入输出(double):" << portName.toStdString() << "值:" << data[0] << std::endl;
        }
        else if(buffer->GetDataType() == DataType::CIRCULAR_BUFFER_DCOMPLEX) {
            std::complex<double> outputValue = m_dcomplexQueue.front();
            m_dcomplexQueue.pop();
            std::vector<std::complex<double>> data;
            data.push_back(outputValue);
            buffer->WriteData(data);
            std::cout << "写入输出(std::complex<double>):" << portName.toStdString() << "值:" << data[0].real() << "," << data[0].imag() << std::endl;
        }

        // 根据数据类型写入Buffer

        std::cout << "buffer: " << std::endl;
        std::cout << "buffer TotalWritten: " << buffer->GetTotalWritten() << std::endl;
        std::cout << "buffer BufferFreeSpace: " << buffer->GetBufferFreeSpace() << std::endl;
        std::cout << "buffer UsedSpace: " << buffer->GetUsedSpace() << std::endl;
        std::cout << "buffer ReaderCount: " << buffer->GetReaderCount() << std::endl;


        break;

    }

    return true;
    m_currentStep++;

    return true;
}

bool inPortBlock::Done()
{
    qDebug() << "inPortBlock::Done - 实例:" << m_instanceName;
    return true;
}

bool inPortBlock::Stop()
{
    qDebug() << "inPortBlock::Stop - 实例:" << m_instanceName;
    Block::Stop();
    return true;
}

} // namespace SystemVueModelBuilder
