#include "Sink_Block.h"
#include <QDir>

#define FILEWRITER_BUFFER_SIZE 1000000

Sink_Block::Sink_Block(const std::string &name)
    :Block(name)
{
    Index = 1;
    m_sampleRate = 0;
    m_pdBuffer = nullptr;
    m_iBuffer = 0;
}

Sink_Block::~Sink_Block()
{
    cleanup();
}

bool Sink_Block::Setup()
{
    Block::Setup();

    // 检查收集范围是否超出文件大小限制（2GB）
    long long totalSamples = 0;
    switch (m_StartStopOption) {
    case Sink::Auto:
    case Sink::Time:
        if (m_sampleRate > 0) {
            int calculatedStart = static_cast<int>(m_TimeStart * m_sampleRate);
            int calculatedStop  = static_cast<int>(m_TimeStop * m_sampleRate);
            totalSamples = calculatedStop - calculatedStart + 1;
        }
        break;
    case Sink::Samples:
        totalSamples = m_SampleStop - m_SampleStart + 1;
        break;
    }

    unsigned long maxSamples = (static_cast<unsigned long>(1) << 31) / sizeof(double);
    if (totalSamples > static_cast<long long>(maxSamples)) {
        char errorMsg[256];
        LOG_ERROR(errorMsg, sizeof(errorMsg),
                  "Data collection range too large. Maximum samples allowed: %lu (%.2f GB)",
                  maxSamples, (maxSamples * sizeof(double)) / (1024.0 * 1024.0 * 1024.0));
    }

    // 创建输出目录
    QString outputPath = QString::fromStdString(getOutPutPath());
    QString folderPath = outputPath + "/01";
    QDir dir(folderPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 根据链路名、子系统名、实例名等构造唯一文件名，最终路径存入 FileName
    QString fileName;
    QString linkName = QString::fromStdString(getSimu().linkName);
    QString subsystemName = QString::fromStdString(getSubsystemName());
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

    QString fullPath = folderPath + "/" + fileName;
    m_WritePath = "/01/" + fileName;

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
        qDebug() << "[Sink_Block] 检测到时间驱动模式，启用定期刷新, 间隔:" << m_flushInterval;
    }
    qDebug() << "Sink::Setup - m_isTimeDrivenMode: " << (m_isTimeDrivenMode ? "true" : "false");

    return true;
}

bool Sink_Block::Run()
{
    // 获取当前仿真时间（时间驱动有效，数据流返回0但不使用）
    if (m_isTimeDrivenMode) {
        m_currentSimulationTime = GetCurrentTime();
    }

    // 事件驱动模式：ZeroCross触发时跳过数据输出，只推进时间（由迭代计数器处理）
    if (IsEventDrivenMode() && ShouldSkipDataOutput()) {
        return true;
    }

    std::string inputPortName = GetInputPortName(0);
    BufferReader* inputReader = GetInputPort(inputPortName);

    // 处理变长数据（TIMED_DOUBLE）或普通 DOUBLE
    if (inputReader->GetConnectedBuffer()->GetDataType() != DataType::DOUBLE) {
        auto inputData = ReadInputData<double>(inputPortName);
        if (inputData.empty()) {
            // 事件驱动模式下无数据时仍然返回true（时间由迭代计数器推进）
            return true;
        }

        for (size_t i = 0; i < inputData.size(); ++i) {
            // 跳过 SampleStart/TimeStart 之前的数据点
            if (m_sinkSkipSamples > 0) {
                --m_sinkSkipSamples;
                continue;
            }
            // 达到目标采样点后跳过写入
            if (Index - 1 >= m_sinkTargetSamples) {
                continue;
            }
            double timeVal = 0.0;
            if (m_isTimeDrivenMode) {
                timeVal = m_currentSimulationTime;
            } else if (IsEventDrivenMode()) {
                // 事件驱动模式：用迭代计数计算时间
                timeVal = (GetCurrentIteration() - 1) / m_sampleRate;
            } else {
                switch (m_StartStopOption) {
                case Sink::Auto:
                    timeVal = (Index - 1) / m_sampleRate;
                    break;
                case Sink::Time:
                    timeVal = m_TimeStart + (Index - 1) / m_sampleRate;
                    break;
                case Sink::Samples:
                    timeVal = 0.0;
                    break;
                }
            }

            m_pdBuffer[m_iBuffer].time  = timeVal;
            m_pdBuffer[m_iBuffer].value = inputData[i];
            ++m_iBuffer;
            ++m_flushCounter;
            ++Index;

            RunDealData();
        }
    }
    // 处理单个 DOUBLE 数据
    else {
        double inputData;
        if (!inputReader->ReadData(inputData)) {
            // 事件驱动模式下无数据时仍然返回true
            return true;
        }

        // 跳过 SampleStart/TimeStart 之前的数据点
        if (m_sinkSkipSamples > 0) {
            --m_sinkSkipSamples;
            return true;
        }

        // 达到目标采样点后跳过写入
        if (Index - 1 >= m_sinkTargetSamples) {
            return true;
        }

        double timeVal = 0.0;
        if (m_isTimeDrivenMode) {
            timeVal = m_currentSimulationTime;
        } else if (IsEventDrivenMode()) {
            // 事件驱动模式：用迭代计数计算时间
            timeVal = (GetCurrentIteration() - 1) / m_sampleRate;
        } else {
            switch (m_StartStopOption) {
            case Sink::Auto:
                timeVal = (Index - 1) / m_sampleRate;
                break;
            case Sink::Time:
                timeVal = m_TimeStart + (Index - 1) / m_sampleRate;
                break;
            case Sink::Samples:
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

    return true;
}

bool Sink_Block::Initialize()
{
    SetBlockType(Block::BlockType::SINK);
    m_sink = std::make_unique<Sink>();
    AddInputPort("input", m_sink->input, 1, DataType::TIMED_DOUBLE);

    SetDefaultParameters();
    std::string File = getSimu().linkName + "_" + getInstanceName();
    m_StartStopOption = ConvertStringToSelected(getParameter("StartStopOption").Value);
    m_fileName = CopyStringToCharPtr(getParameter("FileName").Value);
    FileName = CopyStringToCharPtr(File);
    m_sampleRate = getSimu().samplingRate;

    if (m_StartStopOption == Sink::Auto) {
        Block::SetTerminalMode(TerminalMode::AUTO);
        m_TimeStart = getSimu().startTime;
        m_TimeStop  = getSimu().stopTime;
    } else if (m_StartStopOption == Sink::Samples) {
        Block::SetTerminalMode(TerminalMode::SAMPLES);
        m_SampleStart = std::stoi(getParameter("SampleStart").Value);
        m_SampleStop  = std::stoi(getParameter("SampleStop").Value);
    } else if (m_StartStopOption == Sink::Time) {
        Block::SetTerminalMode(TerminalMode::TIME);
        m_TimeStart = std::stod(getParameter("TimeStart").Value);
        m_TimeStop  = std::stod(getParameter("TimeStop").Value);
    }

    SetParameters(m_SampleStart, m_SampleStop, m_TimeStart, m_TimeStop,
                  m_StartStopOption, m_fileName);

    // 计算 SINK 目标采样点数（仅在 Stop_Condition == "按数据收集器" 时生效）
    m_sinkTargetSamples = ULLONG_MAX;
    m_sinkSkipSamples = 0;
    if (getSimu().stopCondition == "按数据收集器") {
        size_t sim_total_samples = getSimu().num_Samples;
        unsigned long long sink_target = 0;
        switch (m_StartStopOption) {
        case Sink::Auto:
            sink_target = sim_total_samples;
            m_sinkSkipSamples = 0;
            break;
        case Sink::Samples:
            sink_target = (m_SampleStop >= m_SampleStart) ? static_cast<unsigned long long>(m_SampleStop - m_SampleStart + 1) : 0;
            m_sinkSkipSamples = (m_SampleStart > 0) ? static_cast<unsigned long long>(m_SampleStart) : 0;
            break;
        case Sink::Time:
            if (getSimu().time_Interval > 0) sink_target = static_cast<unsigned long long>(m_TimeStop / getSimu().time_Interval);
            else if (m_sampleRate > 0) sink_target = static_cast<unsigned long long>(m_TimeStop * m_sampleRate);
            if (m_TimeStart > 0) {
                if (getSimu().time_Interval > 0) m_sinkSkipSamples = static_cast<unsigned long long>(m_TimeStart / getSimu().time_Interval);
                else if (m_sampleRate > 0) m_sinkSkipSamples = static_cast<unsigned long long>(m_TimeStart * m_sampleRate);
            }
            break;
        }
        m_sinkTargetSamples = (sink_target < sim_total_samples) ? sink_target : sim_total_samples;
        qDebug() << "[Sink_Block] sink_target=" << sink_target << "sim_total=" << sim_total_samples << "m_sinkTargetSamples=" << m_sinkTargetSamples << "m_sinkSkipSamples=" << m_sinkSkipSamples;
    }
    return true;
}

bool Sink_Block::Done()
{
    qDebug() << "[Sink_Block] Done - 处理剩余数据"
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

bool Sink_Block::Flush()
{
    if (m_isTimeDrivenMode && m_flushCounter >= m_flushInterval) {
        flushToFile();
        m_flushCounter = 0;
        return true;
    }
    return false;
}

bool Sink_Block::IsCollectionComplete()
{
    // 事件驱动模式：基于迭代计数判断完成
    if (IsEventDrivenMode()) {
        switch (m_StartStopOption) {
        case Sink::Auto:
            return GetCurrentIteration() >= static_cast<unsigned long long>(getSimu().num_Samples);
        case Sink::Samples:
            return (Index - 1) >= static_cast<unsigned long long>(m_SampleStop - m_SampleStart + 1);
        case Sink::Time:
            return GetCurrentIteration() >= static_cast<unsigned long long>(m_TimeStop * m_sampleRate);
        default:
            return false;
        }
    }

    unsigned long long collected = Index - 1;   // 实际已记录点数

    switch (m_StartStopOption) {
    case Sink::Auto:
        return collected >= getSimu().num_Samples;
    case Sink::Samples:
        return collected >= static_cast<unsigned long long>(m_SampleStop - m_SampleStart + 1);
    case Sink::Time:
        // 时间模式下当仿真时间达到或超过 TimeStop 时完成
        return m_currentSimulationTime >= m_TimeStop;
    default:
        return false;
    }
}

void Sink_Block::SetParameters(int SampleStart, int SampleStop,
                               double TimeStart, double TimeStop,
                               Sink::SelectedStartStopOption StartStopOption,
                               char *Filename)
{
    m_SampleStart = SampleStart;
    m_SampleStop  = SampleStop;
    m_TimeStart   = TimeStart;
    m_TimeStop    = TimeStop;
    m_StartStopOption = StartStopOption;
    m_fileName    = Filename;

    if (m_sink) {
        m_sink->SampleStart = m_SampleStart;
        m_sink->SampleStop  = m_SampleStop;
        m_sink->TimeStart   = m_TimeStart;
        m_sink->TimeStop    = m_TimeStop;
        m_sink->FileName    = m_fileName;
    }
}

// ---------- 辅助函数（无改动）----------
Sink::SelectedStartStopOption Sink_Block::ConvertStringToSelected(const std::string &value)
{
    std::string trimmedValue = value;
    trimmedValue.erase(0, trimmedValue.find_first_not_of(" \t\n\r"));
    trimmedValue.erase(trimmedValue.find_last_not_of(" \t\n\r") + 1);
    std::string lowerValue = trimmedValue;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

    if (lowerValue == "auto" || lowerValue == "0") return Sink::Auto;
    if (lowerValue == "samples" || lowerValue == "1") return Sink::Samples;
    if (lowerValue == "time" || lowerValue == "2") return Sink::Time;
    return Sink::Auto;
}

void Sink_Block::CopyStringToCharPtr(const std::string &src, char *&dest)
{
    delete[] dest;
    if (!src.empty()) {
        dest = new char[src.length() + 1];
        strcpy(dest, src.c_str());
    } else {
        dest = nullptr;
    }
}

char* Sink_Block::CopyStringToCharPtr(const std::string& src)
{
    size_t bufSize = src.length() + 1;
    char* dest = new (std::nothrow) char[bufSize];
    for (size_t i = 0; i < src.length(); ++i)
        dest[i] = src[i];
    dest[src.length()] = '\0';
    return dest;
}

void Sink_Block::SetDefaultParameters()
{
    m_StartStopOption = Sink::Auto;
    m_SampleStart = 0;
    m_SampleStop  = 1;
    m_TimeStart   = 0.0;
    m_TimeStop    = 1.0;
    m_fileName    = nullptr;
}

// ---------- 文件操作 ----------
bool Sink_Block::openFileForWrite()
{
    m_qfile.setFileName(m_fullPath);
    if (!m_qfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_INFO("[Sink_Block] 无法创建文件:", m_qfile.errorString().toStdString());
        return false;
    }

    m_stream.setDevice(&m_qfile);
    m_stream.setCodec("UTF-8");
    m_stream << "[" << "\r\n";
    m_stream.flush();

    m_fileOpenedForAppend = true;
    qDebug() << "[Sink_Block] 文件已创建:" << m_fullPath;
    return true;
}

bool Sink_Block::openFileForAppend()
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

void Sink_Block::closeFileProperly()
{
    if (!m_qfile.isOpen()) return;
    m_stream << "\r\n]";
    m_stream.flush();
    m_qfile.close();
    m_fileOpenedForAppend = false;
}

void Sink_Block::cleanup()
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
void Sink_Block::writeDataPointToStream(size_t bufferIndex, unsigned long long dataIndex)
{
    const DataPoint& pt = m_pdBuffer[bufferIndex];

    // JSON 分隔符
    if (dataIndex > 1) {
        m_stream << ",\r\n";
    }

    m_stream << "\t{\r\n";
    m_stream << "\t\t\"Index\": " << dataIndex << ",\r\n";

    switch (m_StartStopOption) {
    case Sink::Auto:
    case Sink::Time:
        // 输出时间字段，值已在 Run 中根据模式计算好
        m_stream << "\t\t\"Sink_Time\": " << pt.time << ",\r\n";
        break;
    case Sink::Samples:
        // 输出序号字段
        m_stream << "\t\t\"Sink_Index\": " << (m_SampleStart + dataIndex - 1) << ",\r\n";
        break;
    }

    m_stream << "\t\t\"Sink_Data\": " << pt.value << "\r\n";
    m_stream << "\t}";
}

// ---------- 缓冲区满时批量写入 ----------
void Sink_Block::RunDealData()
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
void Sink_Block::flushToFile()
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

bool Sink_Block::isTimeDrivenMode() const { return m_isTimeDrivenMode; }
void Sink_Block::setTimeDrivenMode(bool enabled) { m_isTimeDrivenMode = enabled; }
double Sink_Block::GetCurrentSimulationTime() const { return m_currentSimulationTime; }
