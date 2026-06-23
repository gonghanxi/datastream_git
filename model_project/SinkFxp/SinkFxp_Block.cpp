#include "SinkFxp_Block.h"
#include <QDir>
#include <cmath>

#define FILEWRITER_BUFFER_SIZE 1000000

SinkFxp_Block::SinkFxp_Block(const std::string &name)
    :Block(name)
{
    Index = 1;
    m_sampleRate = 0;
    m_pdBuffer = nullptr;
    m_iBuffer = 0;
    m_fxpPos = 4;
    m_fxpFactor = 1.0;
}

SinkFxp_Block::~SinkFxp_Block()
{
    cleanup();
}

bool SinkFxp_Block::Setup()
{
    Block::Setup();

    long long totalSamples = 0;
    switch (m_StartStopOption) {
    case SinkFxp::Auto:
    case SinkFxp::Time:
        if (m_sampleRate > 0) {
            int calculatedStart = static_cast<int>(m_TimeStart * m_sampleRate);
            int calculatedStop  = static_cast<int>(m_TimeStop * m_sampleRate);
            totalSamples = calculatedStop - calculatedStart + 1;
        }
        break;
    case SinkFxp::Samples:
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

    QString outputPath = QString::fromStdString(getOutPutPath());
    QString folderPath = outputPath + "/01";
    QDir dir(folderPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

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

    m_pdBuffer = new DataPoint[FILEWRITER_BUFFER_SIZE];
    m_iBuffer = 0;
    m_fullPath = fullPath;

    if (!IsVariableStepMode()) {
        BufferReader* inputReader = GetInputPort(GetInputPortName(0));
        Buffer* outputBuffer = inputReader->GetConnectedBuffer();
        size_t WriteSize = outputBuffer->GetWriteSize();
        size_t ReadSize  = inputReader->GetReadSize();
        if (WriteSize != ReadSize) {
            inputReader->SetReadSize(outputBuffer->GetWriteSize());
        }
    }

    if (IsVariableStepMode()) {
        m_isTimeDrivenMode = true;
        m_flushInterval = 100;
        qDebug() << "[SinkFxp_Block] time-driven mode, flush interval:" << m_flushInterval;
    }
    qDebug() << "SinkFxp_Block::Setup - m_isTimeDrivenMode: " << (m_isTimeDrivenMode ? "true" : "false");

    return true;
}

bool SinkFxp_Block::Run()
{
    if (m_isTimeDrivenMode) {
        m_currentSimulationTime = GetCurrentTime();
    }

    if (IsEventDrivenMode() && ShouldSkipDataOutput()) {
        return true;
    }

    std::string inputPortName = GetInputPortName(0);
    BufferReader* inputReader = GetInputPort(inputPortName);

    if (inputReader->GetConnectedBuffer()->GetDataType() != DataType::DOUBLE) {
        auto inputData = ReadInputData<double>(inputPortName);
        if (inputData.empty()) {
            return true;
        }

        for (size_t i = 0; i < inputData.size(); ++i) {
            double timeVal = 0.0;
            if (m_isTimeDrivenMode) {
                timeVal = m_currentSimulationTime;
            } else if (IsEventDrivenMode()) {
                timeVal = (GetCurrentIteration() - 1) / m_sampleRate;
            } else {
                switch (m_StartStopOption) {
                case SinkFxp::Auto:
                    timeVal = (Index - 1) / m_sampleRate;
                    break;
                case SinkFxp::Time:
                    timeVal = m_TimeStart + (Index - 1) / m_sampleRate;
                    break;
                case SinkFxp::Samples:
                    timeVal = 0.0;
                    break;
                }
            }

            // FxpPos truncation
            double truncatedValue = std::trunc(inputData[i] * m_fxpFactor) / m_fxpFactor;

            m_pdBuffer[m_iBuffer].time  = timeVal;
            m_pdBuffer[m_iBuffer].value = truncatedValue;
            ++m_iBuffer;
            ++m_flushCounter;
            ++Index;

            RunDealData();
        }
    }
    else {
        double inputData;
        if (!inputReader->ReadData(inputData)) {
            return true;
        }

        double timeVal = 0.0;
        if (m_isTimeDrivenMode) {
            timeVal = m_currentSimulationTime;
        } else if (IsEventDrivenMode()) {
            timeVal = (GetCurrentIteration() - 1) / m_sampleRate;
        } else {
            switch (m_StartStopOption) {
            case SinkFxp::Auto:
                timeVal = (Index - 1) / m_sampleRate;
                break;
            case SinkFxp::Time:
                timeVal = m_TimeStart + (Index - 1) / m_sampleRate;
                break;
            case SinkFxp::Samples:
                timeVal = 0.0;
                break;
            }
        }

        // FxpPos truncation
        double truncatedValue = std::trunc(inputData * m_fxpFactor) / m_fxpFactor;

        m_pdBuffer[m_iBuffer].time  = timeVal;
        m_pdBuffer[m_iBuffer].value = truncatedValue;
        ++m_iBuffer;
        ++m_flushCounter;
        ++Index;

        RunDealData();
    }

    return true;
}

bool SinkFxp_Block::Initialize()
{
    SetBlockType(Block::BlockType::SINK);
    m_sinkFxp = std::make_unique<SinkFxp>();
    AddInputPort("input", m_sinkFxp->input, 1, DataType::TIMED_DOUBLE);

    SetDefaultParameters();
    std::string File = getSimu().linkName + "_" + getInstanceName();
    m_StartStopOption = ConvertStringToSelected(getParameter("StartStopOption").Value);
    m_fileName = CopyStringToCharPtr(getParameter("FileName").Value);
    FileName = CopyStringToCharPtr(File);
    m_sampleRate = getSimu().samplingRate;

    try { m_fxpPos = std::stoi(getParameter("FxpPos").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FxpPos', using default value."); }
    m_fxpFactor = std::pow(10.0, m_fxpPos);

    if (m_StartStopOption == SinkFxp::Auto) {
        Block::SetTerminalMode(TerminalMode::AUTO);
        m_TimeStart = getSimu().startTime;
        m_TimeStop  = getSimu().stopTime;
    } else if (m_StartStopOption == SinkFxp::Samples) {
        Block::SetTerminalMode(TerminalMode::SAMPLES);
        m_SampleStart = std::stoi(getParameter("SampleStart").Value);
        m_SampleStop  = std::stoi(getParameter("SampleStop").Value);
    } else if (m_StartStopOption == SinkFxp::Time) {
        Block::SetTerminalMode(TerminalMode::TIME);
        m_TimeStart = std::stod(getParameter("TimeStart").Value);
        m_TimeStop  = std::stod(getParameter("TimeStop").Value);
    }

    SetParameters(m_SampleStart, m_SampleStop, m_TimeStart, m_TimeStop,
                  m_StartStopOption, m_fileName);
    return true;
}

bool SinkFxp_Block::Done()
{
    qDebug() << "[SinkFxp_Block] Done -"
             << (m_isTimeDrivenMode ? "time-driven" : "data-flow");

    if (m_iBuffer == 0) {
        if (m_fileOpenedForAppend) {
            closeFileProperly();
        }
        cleanup();
        return true;
    }

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

    unsigned long long startIndex = Index - m_iBuffer;

    for (size_t i = 0; i < m_iBuffer; ++i) {
        writeDataPointToStream(i, startIndex + i);
    }

    closeFileProperly();
    cleanup();
    std::cout << "[RESULT]" << m_WritePath.toStdString() << std::endl;
    return true;
}

bool SinkFxp_Block::Flush()
{
    if (m_isTimeDrivenMode && m_flushCounter >= m_flushInterval) {
        flushToFile();
        m_flushCounter = 0;
        return true;
    }
    return false;
}

bool SinkFxp_Block::IsCollectionComplete()
{
    if (IsEventDrivenMode()) {
        switch (m_StartStopOption) {
        case SinkFxp::Auto:
            return GetCurrentIteration() >= static_cast<unsigned long long>(getSimu().num_Samples);
        case SinkFxp::Samples:
            return (Index - 1) >= static_cast<unsigned long long>(m_SampleStop - m_SampleStart + 1);
        case SinkFxp::Time:
            return GetCurrentIteration() >= static_cast<unsigned long long>(m_TimeStop * m_sampleRate);
        default:
            return false;
        }
    }

    unsigned long long collected = Index - 1;

    switch (m_StartStopOption) {
    case SinkFxp::Auto:
        return collected >= getSimu().num_Samples;
    case SinkFxp::Samples:
        return collected >= static_cast<unsigned long long>(m_SampleStop - m_SampleStart + 1);
    case SinkFxp::Time:
        return m_currentSimulationTime >= m_TimeStop;
    default:
        return false;
    }
}

void SinkFxp_Block::SetParameters(int SampleStart, int SampleStop,
                               double TimeStart, double TimeStop,
                               SinkFxp::SelectedStartStopOption StartStopOption,
                               char *Filename)
{
    m_SampleStart = SampleStart;
    m_SampleStop  = SampleStop;
    m_TimeStart   = TimeStart;
    m_TimeStop    = TimeStop;
    m_StartStopOption = StartStopOption;
    m_fileName    = Filename;

    if (m_sinkFxp) {
        m_sinkFxp->SampleStart = m_SampleStart;
        m_sinkFxp->SampleStop  = m_SampleStop;
        m_sinkFxp->TimeStart   = m_TimeStart;
        m_sinkFxp->TimeStop    = m_TimeStop;
        m_sinkFxp->FileName    = m_fileName;
    }
}

SinkFxp::SelectedStartStopOption SinkFxp_Block::ConvertStringToSelected(const std::string &value)
{
    std::string trimmedValue = value;
    trimmedValue.erase(0, trimmedValue.find_first_not_of(" \t\n\r"));
    trimmedValue.erase(trimmedValue.find_last_not_of(" \t\n\r") + 1);
    std::string lowerValue = trimmedValue;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

    if (lowerValue == "auto" || lowerValue == "0") return SinkFxp::Auto;
    if (lowerValue == "samples" || lowerValue == "1") return SinkFxp::Samples;
    if (lowerValue == "time" || lowerValue == "2") return SinkFxp::Time;
    return SinkFxp::Auto;
}

void SinkFxp_Block::CopyStringToCharPtr(const std::string &src, char *&dest)
{
    delete[] dest;
    if (!src.empty()) {
        dest = new char[src.length() + 1];
        strcpy(dest, src.c_str());
    } else {
        dest = nullptr;
    }
}

char* SinkFxp_Block::CopyStringToCharPtr(const std::string& src)
{
    size_t bufSize = src.length() + 1;
    char* dest = new (std::nothrow) char[bufSize];
    for (size_t i = 0; i < src.length(); ++i)
        dest[i] = src[i];
    dest[src.length()] = '\0';
    return dest;
}

void SinkFxp_Block::SetDefaultParameters()
{
    m_StartStopOption = SinkFxp::Auto;
    m_SampleStart = 0;
    m_SampleStop  = 1;
    m_TimeStart   = 0.0;
    m_TimeStop    = 1.0;
    m_fileName    = nullptr;
    m_fxpPos      = 4;
    m_fxpFactor   = 10000.0;
}

// ---------- File operations ----------
bool SinkFxp_Block::openFileForWrite()
{
    m_qfile.setFileName(m_fullPath);
    if (!m_qfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_INFO("[SinkFxp_Block] file creation failed:", m_qfile.errorString().toStdString());
        return false;
    }

    m_stream.setDevice(&m_qfile);
    m_stream.setCodec("UTF-8");
    m_stream << "[" << "\r\n";
    m_stream.flush();

    m_fileOpenedForAppend = true;
    qDebug() << "[SinkFxp_Block] file created:" << m_fullPath;
    return true;
}

bool SinkFxp_Block::openFileForAppend()
{
    QString filePath = QString::fromUtf8(FileName);
    m_qfile.setFileName(filePath);
    if (!m_qfile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        LOG_INFO("open file failed:", m_qfile.errorString().toStdString());
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

void SinkFxp_Block::closeFileProperly()
{
    if (!m_qfile.isOpen()) return;
    m_stream << "\r\n]";
    m_stream.flush();
    m_qfile.close();
    m_fileOpenedForAppend = false;
}

void SinkFxp_Block::cleanup()
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

// ---------- Write data point to JSON stream ----------
void SinkFxp_Block::writeDataPointToStream(size_t bufferIndex, unsigned long long dataIndex)
{
    const DataPoint& pt = m_pdBuffer[bufferIndex];

    if (dataIndex > 1) {
        m_stream << ",\r\n";
    }

    m_stream << "\t{\r\n";
    m_stream << "\t\t\"Index\": " << dataIndex << ",\r\n";

    switch (m_StartStopOption) {
    case SinkFxp::Auto:
    case SinkFxp::Time:
        m_stream << "\t\t\"Sink_Time\": " << pt.time << ",\r\n";
        break;
    case SinkFxp::Samples:
        m_stream << "\t\t\"Sink_Index\": " << (m_SampleStart + dataIndex - 1) << ",\r\n";
        break;
    }

    m_stream << "\t\t\"Sink_Data\": " << pt.value << "\r\n";
    m_stream << "\t}";
}

// ---------- Buffer flush ----------
void SinkFxp_Block::RunDealData()
{
    if (m_iBuffer < FILEWRITER_BUFFER_SIZE)
        return;

    if (!m_fileOpenedForAppend) {
        if (!openFileForWrite()) return;
    } else if (!m_qfile.isOpen()) {
        if (!openFileForAppend()) return;
    }

    unsigned long long startIndex = Index - m_iBuffer;

    for (size_t j = 0; j < m_iBuffer; ++j) {
        writeDataPointToStream(j, startIndex + j);
    }
    m_stream.flush();
    m_iBuffer = 0;
}

void SinkFxp_Block::flushToFile()
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

bool SinkFxp_Block::isTimeDrivenMode() const { return m_isTimeDrivenMode; }
void SinkFxp_Block::setTimeDrivenMode(bool enabled) { m_isTimeDrivenMode = enabled; }
double SinkFxp_Block::GetCurrentSimulationTime() const { return m_currentSimulationTime; }
