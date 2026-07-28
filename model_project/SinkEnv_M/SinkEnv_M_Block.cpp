#include "SinkEnv_M_Block.h"
#include <QDir>

#define FILEWRITER_BUFFER_SIZE 1000000

SinkEnv_M_Block::SinkEnv_M_Block(const std::string &name)
    :Block(name)
{

}
SinkEnv_M_Block::~SinkEnv_M_Block()
{
    cleanup();
}

bool SinkEnv_M_Block::Setup()
{
    Block::Setup();


    // 检查收集范围是否超出文件大小限制（2GB）
    long long totalSamples = 0;

    switch (m_StartStopOption) {
    case SinkEnv_M::Auto:
    case SinkEnv_M::Time:
        // 计算一下以确保符合限制
        if (m_sampleRate > 0) {
            int calculatedStart = static_cast<int>(m_TimeStart * m_sampleRate);
            int calculatedStop = static_cast<int>(m_TimeStop * m_sampleRate);
            totalSamples = calculatedStop - calculatedStart + 1;
        }
        break;

    case SinkEnv_M::Samples:
        totalSamples = m_SampleStop - m_SampleStart + 1;
        break;
    }

    // Windows 2GB 文件大小限制
    unsigned long maxSamples = (static_cast<unsigned long>(1) << 31) / sizeof(double);

    if (totalSamples > static_cast<long long>(maxSamples)) {
        char errorMsg[256];
        LOG_ERROR(errorMsg, sizeof(errorMsg),"Data collection range too large. Maximum samples allowed: %lu (%.2f GB)"
                  ,maxSamples,(maxSamples * sizeof(double)) / (1024.0 * 1024.0 * 1024.0));
        return false;
    }

    // 创建完整路径
    QString outputPath = QString::fromStdString(getOutPutPath());
    QString folderPath = outputPath + "/08";

    QDir dir(folderPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 构造文件名 - 修改这里，加入子系统名称
    QString fileName;
    QString linkName = QString::fromStdString(getSimu().linkName);
    QString subsystemName = QString::fromStdString(getSubsystemName());  // 获取子系统名称
    QString instanceName = QString::fromStdString(getInstanceName());
    m_UserId = getUserId();
    QString UserId = QString::fromStdString(m_UserId);
    qDebug() << "UserId: " << UserId;

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

    //后端存储路径
    m_WritePath = "/08/" + fileName;

    // 保存路径（转换为char*给原有代码使用）
    QByteArray pathBytes = fullPath.toUtf8();
    FileName = new char[pathBytes.size() + 1];
    strcpy(FileName, pathBytes.constData());

    // 初始化缓冲区
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

    return true;
}

bool SinkEnv_M_Block::Run()
{
    // 获取当前仿真时间（时间驱动有效，数据流返回0但不使用）
    if (m_isTimeDrivenMode) {
        m_currentSimulationTime = GetCurrentTime();
    }

    std::string inputPortName = GetInputPortName(0);
    BufferReader* inputReader = GetInputPort(inputPortName);

    // 处理变长数据（TIMED_ENVELOPE_MATRIX）或普通 DOUBLE
    // 读取一个或多个数据点
    // 对于每个数据点：
    //   1. 确定时间戳（时间驱动用真实时间，数据流模式根据采样率计算）
    //   2. 存入缓冲区（DataPoint{time, value}）
    //   3. 缓冲区满则调用 RunDealData() 批量写入
    if (inputReader->GetConnectedBuffer()->GetDataType() != DataType::DOUBLE) {
        auto inputData = ReadInputData<EnvelopeMatrix>(inputPortName);
        if (inputData.empty()) {
            return true;  // 无数据，静默跳过
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
            // 计算当前数据点的时间戳（数据流模式）或使用真实时间
            double timeVal = 0.0;
            if (m_isTimeDrivenMode) {
                timeVal = m_currentSimulationTime;
            } else {
                switch (m_StartStopOption) {
                case SinkEnv_M::Auto:
                    timeVal = (Index - 1) / m_sampleRate;
                    break;
                case SinkEnv_M::Time:
                    timeVal = m_TimeStart + (Index - 1) / m_sampleRate;
                    break;
                case SinkEnv_M::Samples:
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
    // 处理单个 ENVELOPE_MATRIX 数据
    else {
//        EnvelopeMatrix inputData;
//        if (!inputReader->ReadData(inputData)) {
//            return true;  // 无数据，静默跳过
//        }

//        double timeVal = 0.0;
//        if (m_isTimeDrivenMode) {
//            timeVal = m_currentSimulationTime;
//        } else {
//            switch (m_StartStopOption) {
//            case Sink_M::Auto:
//                timeVal = (Index - 1) / m_sampleRate;
//                break;
//            case Sink_M::Time:
//                timeVal = m_TimeStart + (Index - 1) / m_sampleRate;
//                break;
//            case Sink_M::Samples:
//                timeVal = 0.0;
//                break;
//            }
//        }

//        m_pdBuffer[m_iBuffer].time  = timeVal;
//        m_pdBuffer[m_iBuffer].value = inputData;
//        ++m_iBuffer;
//        ++m_flushCounter;
//        ++Index;

//        RunDealData();
    }

    return true;
}

bool SinkEnv_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::SINK);

    m_sink = std::make_unique<SinkEnv_M>();

    AddInputPort("input", m_sink->input, 1, DataType::MATRIX_ENVELOPE);

    SetDefaultParameters();
    std::string File = getSimu().linkName + "_" + getInstanceName();

    m_StartStopOption = ConvertStringToSelected(getParameter("StartStopOption").Value);
    m_fileName=CopyStringToCharPtr(getParameter("FileName").Value);
    //存储数据的json文件名
    FileName = CopyStringToCharPtr(File);

    m_sampleRate = getSimu().samplingRate;
    if(m_StartStopOption == SinkEnv_M::Auto) {
        Block::SetTerminalMode(TerminalMode::AUTO);
        m_TimeStart = getSimu().startTime;
        m_TimeStop = getSimu().stopTime;
    }
    else if(m_StartStopOption == SinkEnv_M::Samples) {
        Block::SetTerminalMode(TerminalMode::SAMPLES);
        m_SampleStart = std::stoi(getParameter("SampleStart").Value);
        m_SampleStop = std::stoi(getParameter("SampleStop").Value);
    }
    else if(m_StartStopOption == SinkEnv_M::Time) {
        Block::SetTerminalMode(TerminalMode::TIME);
        m_TimeStart = std::stod(getParameter("TimeStart").Value);
        m_TimeStop = std::stod(getParameter("TimeStop").Value);
    }

    SetParameters();

    // 计算 SINK 目标采样点数（仅在 Stop_Condition == "按数据收集器" 时生效）
    m_sinkTargetSamples = ULLONG_MAX;
    m_sinkSkipSamples = 0;
    if (getSimu().stopCondition == "按数据收集器") {
        size_t sim_total_samples = getSimu().num_Samples;
        unsigned long long sink_target = 0;
        switch (m_StartStopOption) {
        case SinkEnv_M::Auto:
            sink_target = sim_total_samples;
            break;
        case SinkEnv_M::Samples:
            sink_target = (m_SampleStop >= m_SampleStart) ? static_cast<unsigned long long>(m_SampleStop - m_SampleStart + 1) : 0;
            m_sinkSkipSamples = (m_SampleStart > 0) ? static_cast<unsigned long long>(m_SampleStart) : 0;
            break;
        case SinkEnv_M::Time:
            if (getSimu().time_Interval > 0) sink_target = static_cast<unsigned long long>(m_TimeStop / getSimu().time_Interval);
            else if (m_sampleRate > 0) sink_target = static_cast<unsigned long long>(m_TimeStop * m_sampleRate);
            if (m_TimeStart > 0) {
                if (getSimu().time_Interval > 0) m_sinkSkipSamples = static_cast<unsigned long long>(m_TimeStart / getSimu().time_Interval);
                else if (m_sampleRate > 0) m_sinkSkipSamples = static_cast<unsigned long long>(m_TimeStart * m_sampleRate);
            }
            break;
        }
        m_sinkTargetSamples = (sink_target < sim_total_samples) ? sink_target : sim_total_samples;
    }
    return true;
}

bool SinkEnv_M_Block::Done()
{
    qDebug() << "[SinkEnv_M_Block] Done - 处理剩余数据"
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

bool SinkEnv_M_Block::Flush()
{
    if (m_isTimeDrivenMode && m_flushCounter >= m_flushInterval) {
        flushToFile();
        m_flushCounter = 0;
        return true;
    }
    return false;
}

bool SinkEnv_M_Block::IsCollectionComplete()
{
    unsigned long long collected = Index - 1;   // 实际已记录点数

    switch (m_StartStopOption) {
    case SinkEnv_M::Auto:
        return collected >= getSimu().num_Samples;
    case SinkEnv_M::Samples:
        return collected >= static_cast<unsigned long long>(m_SampleStop - m_SampleStart + 1);
    case SinkEnv_M::Time:
        // 时间模式下当仿真时间达到或超过 TimeStop 时完成
        return m_currentSimulationTime >= m_TimeStop;
    default:
        return false;
    }
}
void SinkEnv_M_Block::SetParameters()
{
    if(m_sink) {
        m_sink->SampleStart = m_SampleStart;
        m_sink->SampleStop = m_SampleStop;
        m_sink->TimeStart = m_TimeStart;
        m_sink->TimeStop = m_TimeStop;
        m_sink->FileName = m_fileName;
    }
}

SinkEnv_M::SelectedStartStopOption SinkEnv_M_Block::ConvertStringToSelected(const std::string &value)
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
        return SinkEnv_M::Auto;
    } else if (lowerValue == "samples" || lowerValue == "1") {
        return SinkEnv_M::Samples;
    } else if (lowerValue == "time" || lowerValue == "2") {
        return SinkEnv_M::Time;
    }
}

void SinkEnv_M_Block::CopyStringToCharPtr(const std::string &src, char *&dest)
{
    delete[] dest;
    if (!src.empty()) {
        dest = new char[src.length() + 1];
        strcpy(dest, src.c_str());
    } else {
        dest = nullptr;
    }
}

char* SinkEnv_M_Block::CopyStringToCharPtr(const std::string& src) {
    // 1. 分配内存：长度+1（预留'\0'的位置）
    size_t bufSize = src.length() + 1;
    char* dest = new (std::nothrow) char[bufSize]; // nothrow避免内存不足直接崩溃

    //    // 2. 检查内存分配是否成功
    //    if (dest == nullptr) {
    //        throw std::bad_alloc(); // 抛出异常，让调用方处理
    //    }

    //    // 3. 安全拷贝（替代不安全的strcpy）
    //    memcpy(dest, src.data(), bufSize); // 直接拷贝所有字节（含'\0'）

    for (size_t i = 0; i < src.length(); i++) {
        dest[i] = src[i]; // 直接取string的字符，不依赖c_str()
    }
    dest[src.length()] = '\0'; // 手动加终止符（关键！）

    return dest;
}

char* SinkEnv_M_Block::combinePathWithJsonSuffix(const fs::path& linkKeyFolder, const char* fileName) {
    // 1. 将fs::path转换为UTF-8字符串
    std::string folderPath;
    try {
        folderPath = linkKeyFolder.u8string();
    } catch (...) {
        // 如果u8string失败，使用string()
        folderPath = linkKeyFolder.string();
    }

    // 2. 处理文件名
    std::string fileNameStr;
    if (fileName == nullptr || strlen(fileName) == 0) {
        fileNameStr = "unknown";
    } else {
        // 假设fileName是UTF-8编码，直接使用
        fileNameStr = fileName;
    }

    // 3. 确保有.json后缀
    const std::string jsonSuffix = ".json";
    if (fileNameStr.size() < jsonSuffix.size() ||
            fileNameStr.substr(fileNameStr.size() - jsonSuffix.size()) != jsonSuffix) {
        fileNameStr += jsonSuffix;
    }

    // 4. 拼接路径
    std::string fullPath;
    if (!folderPath.empty() && folderPath.back() != '\\' && folderPath.back() != '/') {
        fullPath = folderPath + "\\" + fileNameStr;
    } else {
        fullPath = folderPath + fileNameStr;
    }

    // 5. 调试输出

    // 6. 分配内存
    char* result = new char[fullPath.size() + 1];
    strcpy(result, fullPath.c_str());

    return result;
}

void SinkEnv_M_Block::SetDefaultParameters()
{
    m_StartStopOption = SinkEnv_M::Auto;
    m_SampleStart = 0;
    m_SampleStop = 1;
    m_TimeStart = 0.0;
    m_TimeStop = 1.0;
    m_fileName = nullptr;
}

// ---------- 文件操作 ----------
bool SinkEnv_M_Block::openFileForWrite()
{
    m_qfile.setFileName(m_fullPath);
    if (!m_qfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_INFO("[SinkEnv_M_Block] 无法创建文件:", m_qfile.errorString().toStdString());
        return false;
    }

    m_stream.setDevice(&m_qfile);
    m_stream.setCodec("UTF-8");
    m_stream << "[" << "\r\n";
    m_stream.flush();

    m_fileOpenedForAppend = true;
    qDebug() << "[SinkEnv_M_Block] 文件已创建:" << m_fullPath;
    return true;
}

bool SinkEnv_M_Block::openFileForAppend()
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

void SinkEnv_M_Block::closeFileProperly()
{
    if (!m_qfile.isOpen()) return;
    m_stream << "\r\n]";
    m_stream.flush();
    m_qfile.close();
    m_fileOpenedForAppend = false;
}

void SinkEnv_M_Block::cleanup()
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
void SinkEnv_M_Block::writeDataPointToStream(size_t bufferIndex, unsigned long long dataIndex)
{
    const DataPoint& pt = m_pdBuffer[bufferIndex];

    // JSON 分隔符
    if (dataIndex > 1) {
        m_stream << ",\r\n";
    }
    numCols = pt.value.NumColumns();
    numRows = pt.value.NumRows();

    m_stream << "\t{\r\n";
    m_stream << "\t\t\"Index\": " << dataIndex << ",\r\n";

    switch (m_StartStopOption) {
    case SinkEnv_M::Auto:
    case SinkEnv_M::Time:
        // 输出时间字段，值已在 Run 中根据模式计算好
        m_stream << "\t\t\"Sink_Time\": " << pt.time << ",\r\n";
        break;
    case SinkEnv_M::Samples:
        // 输出序号字段
        m_stream << "\t\t\"Sink_Index\": " << (m_SampleStart + dataIndex - 1) << ",\r\n";
        break;
    }

    for (int m = 1; m <= numRows; m++)
    {
        for (int n = 1; n <= numCols; n++)
        {
            // 按"Sink_Data_[行][列]":[数据]的格式写json字段
            if (m == numRows && n == numCols)
            {
                // 最后一行不加逗号
                m_stream << "\t\t" << R"("(re)Sink_Data_)" << m << n << R"(":)" << pt.value(m - 1, n - 1).real() << "," << "\r\n";
                m_stream << "\t\t" << R"("(im)Sink_Data_)" << m << n << R"(":)" << pt.value(m - 1, n - 1).imag() << "\r\n";
            }
            else
            {
                m_stream << "\t\t" << R"("(re)Sink_Data_)" << m << n << R"(":)" << pt.value(m - 1, n - 1).real() << "," << "\r\n";
                m_stream << "\t\t" << R"("(im)Sink_Data_)" << m << n << R"(":)" << pt.value(m - 1, n - 1).imag() << "," << "\r\n";
            }
        }
    }
    m_stream << "\t}";
}

void SinkEnv_M_Block::RunDealData()
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
void SinkEnv_M_Block::flushToFile()
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

bool SinkEnv_M_Block::isTimeDrivenMode() const { return m_isTimeDrivenMode; }
void SinkEnv_M_Block::setTimeDrivenMode(bool enabled) { m_isTimeDrivenMode = enabled; }
double SinkEnv_M_Block::GetCurrentSimulationTime() const { return m_currentSimulationTime; }
