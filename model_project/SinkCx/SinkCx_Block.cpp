#include "SinkCx_Block.h"

#include <QDir>

#define FILEWRITER_BUFFER_SIZE 1000000


SinkCx_Block::SinkCx_Block(const std::string &name)
    :Block(name)
{
    Index = 1;
    m_sampleRate = 0;
    m_pdBuffer = 0;
    m_iBuffer = 0;
}

SinkCx_Block::~SinkCx_Block()
{
    cleanup();
}

bool SinkCx_Block::Setup()
{
    Block::Setup();

    // 检查收集范围是否超出文件大小限制（2GB）
    long long totalSamples = 0;
    switch (m_StartStopOption) {
    case SinkCx::Auto:
    case SinkCx::Time:
        // 计算一下以确保符合限制
        if (m_sampleRate > 0) {
            int calculatedStart = static_cast<int>(m_TimeStart * m_sampleRate);
            int calculatedStop = static_cast<int>(m_TimeStop * m_sampleRate);
            totalSamples = calculatedStop - calculatedStart + 1;
        }
        break;

    case SinkCx::Samples:
        totalSamples = m_SampleStop - m_SampleStart + 1;
        break;
    }

    unsigned long maxSamples = (static_cast<unsigned long>(1) << 31) / sizeof(std::complex<double>);
    if (totalSamples > static_cast<long long>(maxSamples)) {
        char errorMsg[256];
        LOG_ERROR(errorMsg, sizeof(errorMsg),
                  "Data collection range too large. Maximum samples allowed: %lu (%.2f GB)",
                  maxSamples, (maxSamples * sizeof(double)) / (1024.0 * 1024.0 * 1024.0));
    }

    // 创建完整路径
    QString outputPath = QString::fromStdString(getOutPutPath());
    QString folderPath = outputPath + "/02";
    QDir dir(folderPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 根据链路名、子系统名、实例名等构造唯一文件名，最终路径存入 FileName
    QString fileName;
    QString linkName = QString::fromStdString(getSimu().linkName);
    QString subsystemName = QString::fromStdString(getSubsystemName());  // 获取子系统名称
    QString instanceName = QString::fromStdString(getInstanceName());
    m_UserId = getUserId();
    QString UserId = QString::fromStdString(m_UserId);

    if (!subsystemName.isEmpty()) {
        if (!UserId.isEmpty())
            fileName = QString("%1_%2_%3_%4.json").arg(linkName, subsystemName, instanceName, UserId);
        else
            fileName = QString("%1_%2_%3.json").arg(linkName, subsystemName, instanceName);
    } else {
        if (!UserId.isEmpty())
            fileName = QString("%1_%2_%3.json").arg(linkName, instanceName, UserId);
        else
            fileName = QString("%1_%2.json").arg(linkName, instanceName);
    }

    //后端存储路径
    QString fullPath = folderPath + "/" + fileName;    
    m_WritePath = "/02/" + fileName;

    // 保存路径（转换为char*给原有代码使用）
    QByteArray pathBytes = fullPath.toUtf8();
    FileName = new char[pathBytes.size() + 1];
    strcpy(FileName, pathBytes.constData());

    // 分配 DataPoint 缓冲区（存储时间和数值）
    m_pdBuffer = new DataPoint[FILEWRITER_BUFFER_SIZE];
    m_iBuffer = 0;
    m_fullPath = fullPath;


    // 匹配上游读写大小（数据流模式）
    if (!IsVariableStepMode()) {
        BufferReader* inputReader = GetInputPort(GetInputPortName(0));
        Buffer* outputBuffer = inputReader->GetConnectedBuffer();
        size_t WriteSize = outputBuffer->GetWriteSize();
        size_t ReadSize  = inputReader->GetReadSize();
        if (WriteSize != ReadSize) {
            inputReader->SetReadSize(outputBuffer->GetWriteSize());
        }
    }

    // 检测并启用时间驱动模式
    if (IsVariableStepMode()) {
        m_isTimeDrivenMode = true;
        m_flushInterval = 100;
        qDebug() << "[SinkCx_Block] 检测到时间驱动模式，启用定期刷新, 间隔:" << m_flushInterval;
    }
    qDebug() << "SinkCx_Block::Setup - m_isTimeDrivenMode: " << (m_isTimeDrivenMode ? "true" : "false");

    return true;
}

bool SinkCx_Block::Run()
{

    // 获取输入端口名称
    std::string inputPortName = GetInputPortName(0);
    BufferReader* inputReader = GetInputPort(inputPortName);

    // 处理变长数据（TIMED_COMPLEX_DOUBLE）或普通 COMPLEX_DOUBLE
    // 读取一个或多个数据点
    // 对于每个数据点：
    //   1. 确定时间戳（时间驱动用真实时间，数据流模式根据采样率计算）
    //   2. 存入缓冲区（DataPoint{time, value}）
    //   3. 缓冲区满则调用 RunDealData() 批量写入
    if (inputReader->GetConnectedBuffer()->GetDataType() != DataType::COMPLEX_DOUBLE) {
        auto inputData = ReadInputData<std::complex<double>>(inputPortName);
        if (inputData.empty()) {
            return true;  // 无数据，静默跳过
        }
        qDebug() << "SinkCx_Block::Run - Index: " << Index;

        for (size_t i = 0; i < inputData.size(); ++i) {
            // 计算当前数据点的时间戳（数据流模式）或使用真实时间
            double timeVal = 0.0;
            if (m_isTimeDrivenMode) {
                timeVal = m_currentSimulationTime;
            } else {
                switch (m_StartStopOption) {
                case SinkCx::Auto:
                    timeVal = (Index - 1) / m_sampleRate;
                    break;
                case SinkCx::Time:
                    timeVal = m_TimeStart + (Index - 1) / m_sampleRate;
                    break;
                case SinkCx::Samples:
                    timeVal = 0.0;  // 不使用
                    break;
                }
            }

            m_pdBuffer[m_iBuffer].time  = timeVal;
            m_pdBuffer[m_iBuffer].value = inputData[i];
            ++m_iBuffer;
            ++m_flushCounter;
            ++Index;  // 全局序号递增

            // 缓冲区满则写入
            RunDealData();
        }
    }
    // 处理单个 COMPLEX_DOUBLE 数据
    else {
        std::complex<double> inputData;
        if (!inputReader->ReadData(inputData)) {
            return true;  // 无数据，静默跳过
        }

        double timeVal = 0.0;
        if (m_isTimeDrivenMode) {
            timeVal = m_currentSimulationTime;
        } else {
            switch (m_StartStopOption) {
            case SinkCx::Auto:
                timeVal = (Index - 1) / m_sampleRate;
                break;
            case SinkCx::Time:
                timeVal = m_TimeStart + (Index - 1) / m_sampleRate;
                break;
            case SinkCx::Samples:
                timeVal = 0.0;
                break;
            }
        }

        m_pdBuffer[m_iBuffer].time  = timeVal;
        m_pdBuffer[m_iBuffer].value = inputData;
        ++m_iBuffer;
        ++m_flushCounter;
        ++Index;

        RunDealData();
    }
    qDebug() << "SinkCx_Block::Run - m_iBuffer: " << m_iBuffer;
    qDebug() << "SinkCx_Block::Run - m_pdBuffer[0] time: " << m_pdBuffer[0].time;
    qDebug() << "SinkCx_Block::Run - m_pdBuffer[0] value: " << m_pdBuffer[0].value.real() << "," << m_pdBuffer[0].value.imag();

    return true;
}

bool SinkCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::SINK);

    m_sinkcx = std::make_unique<SinkCx>();

    AddInputPort("input", m_sinkcx->input, 1, DataType::TIMED_DCOMPLEX);

    SetDefaultParameters();
    std::string File = getSimu().linkName + "_" + getInstanceName();
    m_StartStopOption = ConvertStringToSelected(getParameter("StartStopOption").Value);
    m_fileName = CopyStringToCharPtr(getParameter("FileName").Value);
    FileName = CopyStringToCharPtr(File);
    m_sampleRate = getSimu().samplingRate;

    if (m_StartStopOption == SinkCx::Auto) {
        Block::SetTerminalMode(TerminalMode::AUTO);
        m_TimeStart = getSimu().startTime;
        m_TimeStop  = getSimu().stopTime;
    } else if (m_StartStopOption == SinkCx::Samples) {
        Block::SetTerminalMode(TerminalMode::SAMPLES);
        m_SampleStart = std::stoi(getParameter("SampleStart").Value);
        m_SampleStop  = std::stoi(getParameter("SampleStop").Value);
    } else if (m_StartStopOption == SinkCx::Time) {
        Block::SetTerminalMode(TerminalMode::TIME);
        m_TimeStart = std::stod(getParameter("TimeStart").Value);
        m_TimeStop  = std::stod(getParameter("TimeStop").Value);
    }

    SetParameters(m_SampleStart, m_SampleStop, m_TimeStart, m_TimeStop,
                  m_StartStopOption, m_fileName);
    return true;
}

bool SinkCx_Block::Done()
{   
    qDebug() << "[SinkCx_Block] Done - 处理剩余数据"
             << (m_isTimeDrivenMode ? "时间驱动" : "数据流");

    if (m_iBuffer == 0) {
        if (m_fileOpenedForAppend) {
            closeFileProperly();
        }
        cleanup();
        return true;
    }

    // 确保文件可写
    if (!m_fileOpenedForAppend) {
        if (!openFileForWrite()) {
            cleanup();
            return false;
        }
    } else if (!m_qfile.isOpen()) {
        if (!openFileForAppend()) {
            cleanup();
            return false;
        }
    }

    // 计算起始序号 (Index 已指向下一个未分配的序号)
    unsigned long long startIndex = Index - m_iBuffer;

    for (size_t i = 0; i < m_iBuffer; ++i) {
        writeDataPointToStream(i, startIndex + i);
    }

    closeFileProperly();
    cleanup();
    std::cout << "[RESULT]结果写入文件路径: " << m_WritePath.toStdString() << std::endl;
    return true;
}

bool SinkCx_Block::Flush()
{
    if (m_isTimeDrivenMode && m_flushCounter >= m_flushInterval) {
        flushToFile();
        m_flushCounter = 0;
        return true;
    }
    return false;
}

bool SinkCx_Block::IsCollectionComplete()
{
    unsigned long long collected = Index - 1;   // 实际已记录点数

    switch (m_StartStopOption) {
    case SinkCx::Auto:
        return collected >= getSimu().num_Samples;
    case SinkCx::Samples:
        return collected >= static_cast<unsigned long long>(m_SampleStop - m_SampleStart + 1);
    case SinkCx::Time:
        // 时间模式下当仿真时间达到或超过 TimeStop 时完成
        return m_currentSimulationTime >= m_TimeStop;
    default:
        return false;
    }
}

void SinkCx_Block::SetParameters(int SampleStart, int SampleStop, double TimeStart, double TimeStop, SinkCx::SelectedStartStopOption StartStopOption, char *Filename)
{
    m_SampleStart = SampleStart;
    m_SampleStop = SampleStop;
    m_TimeStart = TimeStart;
    m_TimeStop = TimeStop;
    m_StartStopOption = StartStopOption;
    m_fileName = Filename;

    if(m_sinkcx) {
        m_sinkcx->SampleStart = m_SampleStart;
        m_sinkcx->SampleStop = m_SampleStop;
        m_sinkcx->TimeStart = m_TimeStart;
        m_sinkcx->TimeStop = m_TimeStop;
        m_sinkcx->FileName = m_fileName;
    }
}

int SinkCx_Block::GetBatchSize() const
{
    const int DEFAULT_BATCH = 100;

    return DEFAULT_BATCH;
}

int SinkCx_Block::RunBatch(int maxCount)
{
    if (GetInputPortCount() == 0) return 0;

    std::string inputPortName = GetInputPortName(0);
    BufferReader* reader = GetInputPort(inputPortName);

    if (!reader) return 0;

    // ========== 动态计算实际批量 ==========
    size_t available = reader->GetAvailableDataCount();
    if (available == 0) return 0;

    // 收集器可以批量处理，但不要超过可用数据量
    int batchSize = std::min(maxCount, GetBatchSize());
    batchSize = std::min(batchSize, (int)available);

    // 如果可用数据很少，就全部处理
    if (available < 100) {
        batchSize = (int)available;
    }

    if (batchSize <= 0) return 0;

    // ========== 批量读取和处理 ==========
    std::vector<std::complex<double>> data;
    data.reserve(batchSize);
//    qDebug() << "SinkCx_Block::RunBatch-- 最终执行次数: " << batchSize;

    if (reader->GetConnectedBuffer()->GetDataType() == DataType::COMPLEX_DOUBLE) {
        // 单个数据读取模式
        data.reserve(batchSize);
        for (int i = 0; i < batchSize; i++) {
            double value;
            if (reader->ReadData(value)) {
                data.push_back(value);
            } else {
                break;
            }
        }
    } else {
        // 批量读取模式
        for (int i = 0; i < batchSize; i++) {
            // 每次读取1个数据
            auto ReadData = ReadInputData<std::complex<double>>(inputPortName);
            if (ReadData.empty()) {
                // 数据不足，回滚已读取的数据
                // 注意：这里已经读取的数据无法回滚，需要实现回滚机制
                // 简化处理：返回已处理的数量
                qDebug() << "SinkCx_Block: data insufficient at" << i;
                return i;  // 返回实际处理的数量
            }
            data.push_back(ReadData[0]);
        }
    }

    // 批量写入
    // 处理数据
    for(size_t i = 0; i < data.size(); i++) {
//        m_pdBuffer[m_iBuffer++] = data[i];

        // 缓冲区满了，立即写入文件
        RunDealData();
    }
    return data.size();
}

SinkCx::SelectedStartStopOption SinkCx_Block::ConvertStringToSelected(const std::string &value)
{
    // 去除字符串前后的空格
    std::string trimmedValue = value;
    trimmedValue.erase(0, trimmedValue.find_first_not_of(" \t\n\r"));
    trimmedValue.erase(trimmedValue.find_last_not_of(" \t\n\r") + 1);

    // 转换为小写以便不区分大小写比较
    std::string lowerValue = trimmedValue;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

    // 字符串匹配
    if (lowerValue == "auto" || lowerValue == "0") {
        return SinkCx::Auto;
    } else if (lowerValue == "samples" || lowerValue == "1") {
        return SinkCx::Samples;
    } else if (lowerValue == "time" || lowerValue == "2") {
        return SinkCx::Time;
    }
    return SinkCx::Auto;
}

void SinkCx_Block::CopyStringToCharPtr(const std::string &src, char *&dest)
{
    delete[] dest;
    if (!src.empty()) {
        dest = new char[src.length() + 1];
        strcpy(dest, src.c_str());
    } else {
        dest = nullptr;
    }
}

char *SinkCx_Block::CopyStringToCharPtr(const std::string &src)
{
    // 1. 分配内存：长度+1（预留'\0'的位置）
    size_t bufSize = src.length() + 1;
    char* dest = new (std::nothrow) char[bufSize]; // nothrow避免内存不足直接崩溃

    for (size_t i = 0; i < src.length(); i++) {
        dest[i] = src[i]; // 直接取string的字符，不依赖c_str()
    }
    dest[src.length()] = '\0'; // 手动加终止符（关键！）

    return dest;
}



void SinkCx_Block::SetDefaultParameters()
{
    m_StartStopOption = SinkCx::Auto;
    m_SampleStart = 0;
    m_SampleStop = 1;
    m_TimeStart = 0.0;
    m_TimeStop = 1.0;
    m_fileName = nullptr;
}

// ---------- 文件操作 ----------
bool SinkCx_Block::openFileForWrite()
{
    m_qfile.setFileName(m_fullPath);
    if (!m_qfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_INFO("[SinkCx_Block] 无法创建文件:", m_qfile.errorString().toStdString());
        return false;
    }

    m_stream.setDevice(&m_qfile);
    m_stream.setCodec("UTF-8");
    m_stream << "[" << "\r\n";
    m_stream.flush();

    m_fileOpenedForAppend = true;
    qDebug() << "[SinkCx_Block] 文件已创建:" << m_fullPath;
    return true;
}

bool SinkCx_Block::openFileForAppend()
{
    QString filePath = QString::fromUtf8(FileName);
    m_qfile.setFileName(filePath);
    if (!m_qfile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        LOG_INFO("打开文件失败:", m_qfile.errorString().toStdString());
        return false;
    }
    m_qfile.seek(m_qfile.size());
    if (m_qfile.size() == 0) {
        m_stream << "[";
    }
    m_stream.setDevice(&m_qfile);
    m_stream.setCodec("UTF-8");
    return true;
}

void SinkCx_Block::closeFileProperly()
{
    if (!m_qfile.isOpen()) return;
    m_stream << "\r\n]";
    m_stream.flush();
    m_qfile.close();
    m_fileOpenedForAppend = false;
}

void SinkCx_Block::cleanup()
{
    if (m_pdBuffer) {
        delete[] m_pdBuffer;
        m_pdBuffer = nullptr;
    }
    m_iBuffer = 0;

    if (m_qfile.isOpen()) {
        m_qfile.close();
    }

    if (FileName) {
        delete[] FileName;
        FileName = nullptr;
    }
}

// ---------- 核心写入（根据 StartStopOption 选择字段）----------
void SinkCx_Block::writeDataPointToStream(size_t bufferIndex, unsigned long long dataIndex)
{
    const DataPoint& pt = m_pdBuffer[bufferIndex];

    // JSON 分隔符
    if (dataIndex > 1) {
        m_stream << ",\r\n";
    }

    m_stream << "\t{\r\n";
    m_stream << "\t\t\"Index\": " << dataIndex << ",\r\n";

    switch (m_StartStopOption) {
    case SinkCx::Auto:
    case SinkCx::Time:
        // 输出时间字段，值已在 Run 中根据模式计算好
        m_stream << "\t\t\"Sink_Time\": " << pt.time << ",\r\n";
        break;
    case SinkCx::Samples:
        // 输出序号字段
        m_stream << "\t\t\"Sink_Index\": " << (m_SampleStart + dataIndex - 1) << ",\r\n";
        break;
    }

    m_stream << "\t\t" << R"("(re)Sink_Data": )" << pt.value.real() << "," << "\r\n";
    m_stream << "\t\t" << R"("(im)Sink_Data": )" << pt.value.real() << "\r\n";
    m_stream << "\t}";
}

void SinkCx_Block::RunDealData()
{
    if (m_iBuffer < FILEWRITER_BUFFER_SIZE)
        return;

    // 确保文件处于写入状态
    if (!m_fileOpenedForAppend) {
        if (!openFileForWrite()) return;
    } else if (!m_qfile.isOpen()) {
        if (!openFileForAppend()) return;
    }

    // 起始序号 = 下一个序号 - 缓冲区数量
    unsigned long long startIndex = Index - m_iBuffer;

    for (size_t j = 0; j < m_iBuffer; ++j) {
        writeDataPointToStream(j, startIndex + j);
    }
    m_stream.flush();
    m_iBuffer = 0;
}

// ---------- 时间驱动中途刷新 ----------
void SinkCx_Block::flushToFile()
{
    if (m_iBuffer == 0) return;

    if (!m_fileOpenedForAppend) {
        if (!openFileForWrite()) return;
    } else if (!m_qfile.isOpen()) {
        if (!openFileForAppend()) return;
    }

    unsigned long long startIndex = Index - m_iBuffer;

    for (size_t i = 0; i < m_iBuffer; ++i) {
        writeDataPointToStream(i, startIndex + i);
    }
    m_stream.flush();
    m_iBuffer = 0;
}

bool SinkCx_Block::isTimeDrivenMode() const { return m_isTimeDrivenMode; }
void SinkCx_Block::setTimeDrivenMode(bool enabled) { m_isTimeDrivenMode = enabled; }
double SinkCx_Block::GetCurrentSimulationTime() const { return m_currentSimulationTime; }
