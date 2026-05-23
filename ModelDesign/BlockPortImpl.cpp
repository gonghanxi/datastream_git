#include "BlockPortImpl.h"
#include "Buffer.h"
#include "BufferReader.h"

using namespace SystemVueModelBuilder;

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, SystemVueModelBuilder::CircularBufferBus &externalPort, size_t readSize, DataType dataType)
{
    std::ignore = externalPort;
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<double> &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    //时域buffer设置初始化时间步长
    externalPort.SetTimeStep(1.0);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<int> &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    //时域buffer设置初始化时间步长
    externalPort.SetTimeStep(1.0);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<float> &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);


    //时域buffer设置初始化时间步长
    externalPort.SetTimeStep(1.0);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<bool> &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    //时域buffer设置初始化时间步长
    externalPort.SetTimeStep(1.0);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<std::complex<float> > &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    //时域buffer设置初始化时间步长
    externalPort.SetTimeStep(1.0);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<std::complex<double> > &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    //时域buffer设置初始化时间步长
    externalPort.SetTimeStep(1.0);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, IntCircularBuffer &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, FloatCircularBuffer &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, FComplexCircularBuffer &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, DComplexCircularBuffer &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }
    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, SystemVueModelBuilder::CircularBuffer<double> &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }
    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, BoolCircularBuffer &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, SystemVueModelBuilder::EnvelopeCircularBuffer &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }
    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    //时域Buffer初始化时间步长
    externalPort.SetTimeStep(1.0);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, int &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }
    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, double &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }
    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;


    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, float &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }
    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, bool &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }
    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, std::complex<float> &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }
    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, std::complex<double> &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    //检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    //加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, IntMatrixCircularBuffer &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    // 检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '"
                 << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    // 加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, DoubleMatrixCircularBuffer &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    // 检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '"
                 << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    // 加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, FloatMatrixCircularBuffer &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    // 检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    // 加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, BoolMatrixCircularBuffer &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    // 检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    // 加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, FComplexMatrixCircularBuffer &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    // 检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    // 加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, DComplexMatrixCircularBuffer &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    // 检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    // 加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, EnvelopeMatrixCircularBuffer &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    // 检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    // 加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<IntMatrix> &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    // 检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    // 加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<DoubleMatrix> &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    // 检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    // 加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<FloatMatrix> &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    // 检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    // 加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<BoolMatrix> &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    // 检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    // 加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<FComplexMatrix> &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    // 检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    // 加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

BufferReader *BlockPortImpl::AddInputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<DComplexMatrix> &externalPort, size_t readSize, BlockPortImpl::DataType dataType)
{
    std::ignore = externalPort;
    // 检查是否添加过相同端口
    if(m_block->m_inputPorts.find(portName) != m_block->m_inputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Input port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_inputPorts[portName];
    }

    // 加入instanceName以防重名
    std::string name = m_block->getInstanceName() + "_" + portName;

    BufferReader* reader = new BufferReader(name, readSize, dataType);
    m_block->m_inputPorts[portName] = reader;
    m_block->m_inputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_inputPortNames.push_back(portName);
    m_block->m_inputPortNameToIndex[portName] = m_block->m_inputPortNames.size() - 1;

    reader->SetDataType(dataType);

    return reader;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, CircularBufferBus &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }

    // 设置到外部端口
    Buffer* buffer = new Buffer(portName, writeSize, dataType);
    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;


    //存储外部端口到Buffer中
    buffer->SetExternalCircularBufferBus(&externalPort);

    // 外部端口：存储引用，标记为外部端口，不创建 DFPort
    buffer->SetExternalBasicTypeOutputPorts(portName, externalPort);
    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, IntCircularBuffer &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;
    int* newBuffer = new int[INIT_BUFFER_SIZE];
    externalPort.SetBuffer(newBuffer, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] newBuffer;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, FloatCircularBuffer &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;
    float* newBuffer = new float[INIT_BUFFER_SIZE];
    externalPort.SetBuffer(newBuffer, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] newBuffer;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, BoolCircularBuffer &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;
    bool* newBuffer = new bool[INIT_BUFFER_SIZE];
    externalPort.SetBuffer(newBuffer, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] newBuffer;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, FComplexCircularBuffer &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;
    std::complex<float>* newBuffer = new std::complex<float>[INIT_BUFFER_SIZE];
    externalPort.SetBuffer(newBuffer, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] newBuffer;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}
Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, SystemVueModelBuilder::DComplexCircularBuffer &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;
    std::complex<double>* newBuffer = new std::complex<double>[INIT_BUFFER_SIZE];
    externalPort.SetBuffer(newBuffer, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        //delete[] newBuffer;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, SystemVueModelBuilder::DoubleCircularBuffer &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;
    double* newBuffer = new double[INIT_BUFFER_SIZE];
    externalPort.SetBuffer(newBuffer, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] newBuffer;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<int> &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;

    // 设置到外部端口
    int* bufferMemory = new int[INIT_BUFFER_SIZE];

    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);


    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureTimedCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<float> &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;
    // 设置到外部端口
    float* bufferMemory = new float[INIT_BUFFER_SIZE];

    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);


    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureTimedCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<double> &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;

    // 设置到外部端口
    double* bufferMemory = new double[INIT_BUFFER_SIZE];

    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);


    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureTimedCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<bool> &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;
    // 设置到外部端口
    bool* bufferMemory = new bool[INIT_BUFFER_SIZE];

    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);


    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureTimedCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<std::complex<float> > &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;

    // 设置到外部端口
    std::complex<float>* bufferMemory = new std::complex<float>[INIT_BUFFER_SIZE];

    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);


    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureTimedCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<std::complex<double> > &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;

    // 设置到外部端口
    std::complex<double>* bufferMemory = new std::complex<double>[INIT_BUFFER_SIZE];

    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);


    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureTimedCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, double &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }

    // 设置到外部端口
    Buffer* buffer = new Buffer(portName, writeSize, dataType);
    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    // 外部端口：存储引用，标记为外部端口，不创建 DFPort
    buffer->SetExternalDoublePort(portName, externalPort);
    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, std::complex<float> &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }

    // 设置到外部端口
    Buffer* buffer = new Buffer(portName, writeSize, dataType);
    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    // 外部端口：存储引用，标记为外部端口，不创建 DFPort
    buffer->SetExternalFComplexPort(portName, externalPort);
    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, std::complex<double> &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }

    // 设置到外部端口
    Buffer* buffer = new Buffer(portName, writeSize, dataType);
    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    // 外部端口：存储引用，标记为外部端口，不创建 DFPort
    buffer->SetExternalDComplexPort(portName, externalPort);
    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, SystemVueModelBuilder::EnvelopeCircularBuffer &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;

    // 设置到外部端口
    SystemVueModelBuilder::EnvelopeSignal* external = new SystemVueModelBuilder::EnvelopeSignal[INIT_BUFFER_SIZE];
    externalPort.SetBuffer(external, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();


    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        //delete[] external;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureTimedCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, int &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }

    // 设置到外部端口
    Buffer* buffer = new Buffer(portName, writeSize, dataType);
    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    // 外部端口：存储引用，标记为外部端口，不创建 DFPort
    buffer->SetExternalIntPort(portName, externalPort);
    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, float &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }

    // 设置到外部端口
    Buffer* buffer = new Buffer(portName, writeSize, dataType);
    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    // 外部端口：存储引用，标记为外部端口，不创建 DFPort
    buffer->SetExternalFloatPort(portName, externalPort);
    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, bool &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }

    // 设置到外部端口
    Buffer* buffer = new Buffer(portName, writeSize, dataType);
    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    // 外部端口：存储引用，标记为外部端口，不创建 DFPort
    buffer->SetExternalBoolPort(portName, externalPort);
    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, IntMatrixCircularBuffer &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    // 检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '"
                 << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }

    const size_t INIT_BUFFER_SIZE = 1024; // 初始缓冲区大小，可以根据需要调整

    // 创建矩阵缓冲区内存
    SystemVueModelBuilder::IntMatrix* bufferMemory = new SystemVueModelBuilder::IntMatrix[INIT_BUFFER_SIZE];

    // 初始化矩阵（创建默认大小的矩阵，或者可以先创建空矩阵）
    for (size_t i = 0; i < INIT_BUFFER_SIZE; i++) {
        // 这里可以设置默认的矩阵大小，或者先创建空矩阵
        // bufferMemory[i].Resize(1, 1); // 如果需要默认大小
    }

    // 设置外部端口的缓冲区
    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer for matrix port";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }

    buffer->EnsureCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, FloatMatrixCircularBuffer &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    // 检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '"
                 << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }

    const size_t INIT_BUFFER_SIZE = 1024; // 初始缓冲区大小，可以根据需要调整

    // 创建矩阵缓冲区内存
    SystemVueModelBuilder::FloatMatrix* bufferMemory = new SystemVueModelBuilder::FloatMatrix[INIT_BUFFER_SIZE];

    // 初始化矩阵（创建默认大小的矩阵，或者可以先创建空矩阵）
    for (size_t i = 0; i < INIT_BUFFER_SIZE; i++) {
        // 这里可以设置默认的矩阵大小，或者先创建空矩阵
        // bufferMemory[i].Resize(1, 1); // 如果需要默认大小
    }

    // 设置外部端口的缓冲区
    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer for matrix port";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }

    buffer->EnsureCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, BoolMatrixCircularBuffer &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    // 检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '"
                 << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }

    const size_t INIT_BUFFER_SIZE = 1024; // 初始缓冲区大小，可以根据需要调整

    // 创建矩阵缓冲区内存
    SystemVueModelBuilder::BoolMatrix* bufferMemory = new SystemVueModelBuilder::BoolMatrix[INIT_BUFFER_SIZE];

    // 初始化矩阵（创建默认大小的矩阵，或者可以先创建空矩阵）
    for (size_t i = 0; i < INIT_BUFFER_SIZE; i++) {
        // 这里可以设置默认的矩阵大小，或者先创建空矩阵
        // bufferMemory[i].Resize(1, 1); // 如果需要默认大小
    }

    // 设置外部端口的缓冲区
    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer for matrix port";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }

    buffer->EnsureCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, DoubleMatrixCircularBuffer &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    // 检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '"
                 << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }

    const size_t INIT_BUFFER_SIZE = 1024; // 初始缓冲区大小，可以根据需要调整

    // 创建矩阵缓冲区内存
    SystemVueModelBuilder::DoubleMatrix* bufferMemory = new SystemVueModelBuilder::DoubleMatrix[INIT_BUFFER_SIZE];

    // 初始化矩阵（创建默认大小的矩阵，或者可以先创建空矩阵）
    for (size_t i = 0; i < INIT_BUFFER_SIZE; i++) {
        // 这里可以设置默认的矩阵大小，或者先创建空矩阵
        // bufferMemory[i].Resize(1, 1); // 如果需要默认大小
    }

    // 设置外部端口的缓冲区
    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer for matrix port";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }

    buffer->EnsureCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, FComplexMatrixCircularBuffer &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    // 检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '"
                 << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }

    const size_t INIT_BUFFER_SIZE = 1024; // 初始缓冲区大小，可以根据需要调整

    // 创建矩阵缓冲区内存
    SystemVueModelBuilder::FComplexMatrix* bufferMemory = new SystemVueModelBuilder::FComplexMatrix[INIT_BUFFER_SIZE];

    // 初始化矩阵（创建默认大小的矩阵，或者可以先创建空矩阵）
    for (size_t i = 0; i < INIT_BUFFER_SIZE; i++) {
        // 这里可以设置默认的矩阵大小，或者先创建空矩阵
        // bufferMemory[i].Resize(1, 1); // 如果需要默认大小
    }

    // 设置外部端口的缓冲区
    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer for matrix port";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }

    buffer->EnsureCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, DComplexMatrixCircularBuffer &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    // 检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '"
                 << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }

    const size_t INIT_BUFFER_SIZE = 1024; // 初始缓冲区大小，可以根据需要调整

    // 创建矩阵缓冲区内存
    SystemVueModelBuilder::DComplexMatrix* bufferMemory = new SystemVueModelBuilder::DComplexMatrix[INIT_BUFFER_SIZE];

    // 初始化矩阵（创建默认大小的矩阵，或者可以先创建空矩阵）
    for (size_t i = 0; i < INIT_BUFFER_SIZE; i++) {
        // 这里可以设置默认的矩阵大小，或者先创建空矩阵
        // bufferMemory[i].Resize(1, 1); // 如果需要默认大小
    }

    // 设置外部端口的缓冲区
    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer for matrix port";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }

    buffer->EnsureCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, EnvelopeMatrixCircularBuffer &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    // 检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '"
                 << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }

    const size_t INIT_BUFFER_SIZE = 1024; // 初始缓冲区大小，可以根据需要调整

    // 创建矩阵缓冲区内存
    SystemVueModelBuilder::EnvelopeMatrix* bufferMemory = new SystemVueModelBuilder::EnvelopeMatrix[INIT_BUFFER_SIZE];

    // 初始化矩阵（创建默认大小的矩阵，或者可以先创建空矩阵）
    for (size_t i = 0; i < INIT_BUFFER_SIZE; i++) {
        // 这里可以设置默认的矩阵大小，或者先创建空矩阵
        // bufferMemory[i].Resize(1, 1); // 如果需要默认大小
    }

    // 设置外部端口的缓冲区
    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();

    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer for matrix port";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }

    buffer->EnsureCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;

    // 添加到端口名称容器
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<IntMatrix> &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;
    SystemVueModelBuilder::IntMatrix* bufferMemory = new SystemVueModelBuilder::IntMatrix[INIT_BUFFER_SIZE];
    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();
    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureTimedCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<DoubleMatrix> &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;
    SystemVueModelBuilder::DoubleMatrix* bufferMemory = new SystemVueModelBuilder::DoubleMatrix[INIT_BUFFER_SIZE];
    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();
    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureTimedCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<FloatMatrix> &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;
    SystemVueModelBuilder::FloatMatrix* bufferMemory = new SystemVueModelBuilder::FloatMatrix[INIT_BUFFER_SIZE];
    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();
    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureTimedCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<BoolMatrix> &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;
    SystemVueModelBuilder::BoolMatrix* bufferMemory = new SystemVueModelBuilder::BoolMatrix[INIT_BUFFER_SIZE];
    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();
    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureTimedCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<FComplexMatrix> &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;
    SystemVueModelBuilder::FComplexMatrix* bufferMemory = new SystemVueModelBuilder::FComplexMatrix[INIT_BUFFER_SIZE];
    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();
    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureTimedCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}

Buffer *BlockPortImpl::AddOutputPort(const std::string &portName, SystemVueModelBuilder::TimedCircularBuffer<DComplexMatrix> &externalPort, size_t writeSize, BlockPortImpl::DataType dataType)
{
    //检查是否添加过相同端口
    if(m_block->m_outputPorts.find(portName) != m_block->m_outputPorts.end()) {
        qDebug() << QString::fromStdString(m_block->m_name) << ": Output port '" << QString::fromStdString(portName) << "' already exists";
        return m_block->m_outputPorts[portName];
    }
    size_t INIT_BUFFER_SIZE = 1024;
    SystemVueModelBuilder::DComplexMatrix* bufferMemory = new SystemVueModelBuilder::DComplexMatrix[INIT_BUFFER_SIZE];
    externalPort.SetBuffer(bufferMemory, INIT_BUFFER_SIZE, 1);
    externalPort.Initialize();
    Buffer* buffer = new Buffer(portName, writeSize, dataType);

    bool success = buffer->SetExternalCircularBuffer(&externalPort);
    if (!success) {
        qDebug() << "ERROR: Failed to set external circular buffer";
        delete[] bufferMemory;
        delete buffer;
        return nullptr;
    }
    buffer->EnsureTimedCircularBuffer();

    m_block->m_outputPorts[portName] = buffer;
    m_block->m_outputPortDataTypes[portName] = dataType;
    m_block->m_outputPortNames.push_back(portName);
    m_block->m_outputPortNameToIndex[portName] = m_block->m_outputPortNames.size() - 1;

    return buffer;
}
