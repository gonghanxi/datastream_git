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
    delete[] m_pdBuffer;
}

bool SinkCx_Block::Setup()
{
    Block::Setup();

    // 计算要收集的样本总数
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

    // Windows 2GB 文件大小限制
    unsigned long maxSamples = (static_cast<unsigned long>(1) << 31) / sizeof(std::complex<double>);

    if (totalSamples > static_cast<long long>(maxSamples)) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg),
                 "Data collection range too large. Maximum samples allowed: %lu (%.2f GB)",
                 maxSamples,
                 (maxSamples * sizeof(double)) / (1024.0 * 1024.0 * 1024.0));
        LOG_ERROR(errorMsg, sizeof(errorMsg),"Data collection range too large. Maximum samples allowed: %lu (%.2f GB)"
                  ,maxSamples,(maxSamples * sizeof(double)) / (1024.0 * 1024.0 * 1024.0));
        return false;
    }

    // 创建完整路径
    QString outputPath = QString::fromStdString(getOutPutPath());
    QString folderPath = outputPath + "/02";

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

    if (!subsystemName.isEmpty()) {
        // 有子系统名称：链路名_子系统名_实例名_Id.json
        if(!UserId.isEmpty()) {
            fileName = QString("%1_%2_%3_%4.json")
                    .arg(linkName)
                    .arg(subsystemName)
                    .arg(instanceName)
                    .arg(UserId);
        }
        else {
            fileName = QString("%1_%2_%3.json")
                    .arg(linkName)
                    .arg(subsystemName)
                    .arg(instanceName);
        }

        //        qDebug() << "use subsystem name:" << fileName;
    } else {
        // 没有子系统名称：链路名_实例名_Id.json
        if(!UserId.isEmpty()) {
            fileName = QString("%1_%2_%3.json")
                    .arg(linkName)
                    .arg(instanceName)
                    .arg(UserId);
        }
        else {
            fileName = QString("%1_%2.json")
                    .arg(linkName)
                    .arg(instanceName);
        }

        //        qDebug() << "not use subsystem name:" << fileName;
    }

    QString fullPath = folderPath + "/" + fileName;

    //后端存储路径
    m_WritePath = "/02/" + fileName;

    // 保存路径（转换为char*给原有代码使用）
    QByteArray pathBytes = fullPath.toUtf8();
    FileName = new char[pathBytes.size() + 1];
    strcpy(FileName, pathBytes.constData());

    // 初始化缓冲区
    m_pdBuffer = new std::complex<double>[FILEWRITER_BUFFER_SIZE];
    m_iBuffer = 0;

    m_fullPath = fullPath;
    //    // 创建并初始化文件
    //    m_qfile.setFileName(m_fullPath);
    //    if (!m_qfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    //        LOG_INFO("无法创建文件:",m_qfile.errorString().toStdString());
    //        return false;
    //    }

    //    m_stream.setDevice(&m_qfile);
    //    m_stream.setCodec("UTF-8");
    //    m_stream << "[" << "\r\n";
    //    m_stream.flush();

    // 暂时不关闭文件，保持打开状态用于追加
    // m_qfile.close();

    //更新sink的读取速率
    BufferReader* inputReader = GetInputPort(GetInputPortName(0));
    qDebug() << "SinkCx_Block::Setup inputReader: " << inputReader->GetReadSize();
    Buffer* outputBuffer = inputReader->GetConnectedBuffer();

    size_t WriteSize = outputBuffer->GetWriteSize();

    size_t ReadSize = inputReader->GetReadSize();

    if(WriteSize != ReadSize) {
        inputReader->SetReadSize(outputBuffer->GetWriteSize());
//        outputBuffer->UpdateBufferSize();
    }

    return true;
}

bool SinkCx_Block::Run()
{
    if(!CanProcess()) {
        return false;
    }

    // 获取输入端口名称
    std::string inputPortName = GetInputPortName(0);
    BufferReader* inputReader = GetInputPort(inputPortName);

    if(inputReader->GetConnectedBuffer()->GetDataType() == DataType::COMPLEX_DOUBLE) {
        std::complex<double> inputData;
        if(!inputReader->ReadData(inputData)) {
            LOG_ERROR("Sink '" , GetName(), "' read Double data Failed!");
            return false;
        }
        m_pdBuffer[m_iBuffer++] = inputData;
        // 缓冲区满了，立即写入文件
        RunDealData();
        return true;
    }
    else {

    }
    // 读取数据
    auto inputData = ReadInputData<std::complex<double>>(inputPortName);
    if(inputData.empty()) {
        return false;
    }

    // 处理数据
    for(size_t i = 0; i < inputData.size(); i++) {
        m_pdBuffer[m_iBuffer++] = inputData[i];

        // 缓冲区满了，立即写入文件
        RunDealData();
    }

    return true;
}

bool SinkCx_Block::Initialize()
{
    SetBlockType(Block::BlockType::SINK);

    m_sinkcx = std::make_unique<SinkCx>();

    AddInputPort("input", m_sinkcx->input, 1, DataType::TIMED_DCOMPLEX);

    SetDefaultParameters();

    m_StartStopOption = ConvertStringToSelected(getParameter("StartStopOption").Value);
    //    CopyStringToCharPtr(getParameter("FileName").Value, m_fileName);
    m_fileName=CopyStringToCharPtr(getParameter("FileName").Value);
    //    m_fileName=getParameter("FileName").Value.data();

    m_sampleRate = getSimu().samplingRate;
    if(m_StartStopOption == SinkCx::Auto) {
        Block::SetTerminalMode(TerminalMode::AUTO);
        m_TimeStart = getSimu().startTime;
        m_TimeStop = getSimu().stopTime;
    }
    else if(m_StartStopOption == SinkCx::Samples) {
        Block::SetTerminalMode(TerminalMode::SAMPLES);
        m_SampleStart = std::stoi(getParameter("SampleStart").Value);
        m_SampleStop = std::stoi(getParameter("SampleStop").Value);
    }
    else if(m_StartStopOption == SinkCx::Time) {
        Block::SetTerminalMode(TerminalMode::TIME);
        m_TimeStart = std::stod(getParameter("TimeStart").Value);
        m_TimeStop = std::stod(getParameter("TimeStop").Value);
    }

    SetParameters(m_SampleStart, m_SampleStop, m_TimeStart, m_TimeStop, m_StartStopOption, m_fileName);

    return true;
}

bool SinkCx_Block::Done()
{   
    // 如果文件已经打开，直接写入剩余数据
    if (m_qfile.isOpen()) {
        // 文件已打开，跳过打开文件的步骤
    } else {
        // 文件未打开，才执行打开操作
        m_qfile.setFileName(m_fullPath);
        if (!m_qfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            LOG_INFO("无法创建文件:", m_qfile.errorString().toStdString());
            return false;
        }

        m_stream.setDevice(&m_qfile);
        m_stream.setCodec("UTF-8");
        m_stream << "[" << "\r\n";
        m_stream.flush();
    }

    if (m_iBuffer == 0) {
        // 直接结束JSON数组
        if (m_qfile.isOpen()) {
            m_stream << "\n]";
            m_stream.flush();
            m_qfile.close();
        }
        cleanup();
        return true;
    }

    // 确保文件打开
    if (!m_qfile.isOpen()) {
        if (!openFileForAppend()) {
            cleanup();
            return false;
        }
    }


    // 写入剩余数据
    for (size_t i = 0; i < m_iBuffer; i++) {
        if(IsBitShiftRegister()) {
//            WriteBitShiftRegisterData(i);
        }
        else {
            // 如果不是第一条数据，需要加逗号
            if (Index > 1) {
                m_stream << ",\r\n";
            } else if (i > 0) {
                m_stream << ",\r\n";
            }

            m_stream << "\t{\r\n";
            m_stream << "\t\t\"Index\": " << Index << ",\r\n";

            switch (m_StartStopOption) {
            case SinkCx::Auto: {
                double timeValue = (Index - 1) / m_sampleRate;
//                m_stream << "\t\t\"Sink_Time\": " << formatSinkTime(timeValue) << ",\r\n";
                m_stream << "\t\t\"Sink_Time\": " << timeValue << ",\r\n";
                break;
            }
            case SinkCx::Samples:
                m_stream << "\t\t\"Sink_Index\": " << m_SampleStart + Index - 1 << ",\r\n";
                break;
            case SinkCx::Time: {
                double timeValue = m_TimeStart + (Index - 1) / m_sampleRate;
//                m_stream << "\t\t\"Sink_Time\": " << formatSinkTime(timeValue) << ",\r\n";
                m_stream << "\t\t\"Sink_Time\": " << timeValue << ",\r\n";
                break;
            }
            default:
                break;
            }

            m_stream << "\t\t" << R"("(re)Sink_Data": )" << m_pdBuffer[i].real() << "," << "\r\n";
            m_stream << "\t\t" << R"("(im)Sink_Data": )" << m_pdBuffer[i].imag() << "\r\n";
            m_stream << "\t}";

            Index++;
        }
    }

    // 结束JSON数组
    m_stream << "\r\n]";
    m_stream.flush();

    // 关闭文件
    m_qfile.close();

    // 验证文件
    QString filePath = QString::fromUtf8(FileName);
    QFile checkFile(filePath);
//    LOG_INFO("结果写入文件路径: ", m_WritePath.toStdString());
    std::cout << "[RESULT]结果写入文件路径: " << m_WritePath.toStdString() << std::endl;
    if (checkFile.open(QIODevice::ReadOnly)) {

        // 读取并显示前200个字符
        QByteArray preview = checkFile.read(200);

        // 如果是空的或只有[
        if (checkFile.size() <= 2) {
        }

        checkFile.close();
    } else {
    }

    cleanup();

    return true;
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
        m_pdBuffer[m_iBuffer++] = data[i];

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

// 核心函数：拼接文件夹路径 + 文件名 + .json 后缀，返回 char*（需手动释放）
char* SinkCx_Block::combinePathWithJsonSuffix(const fs::path& linkKeyFolder, const char* fileName) {
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

void SinkCx_Block::SetDefaultParameters()
{
    m_StartStopOption = SinkCx::Auto;
    m_SampleStart = 0;
    m_SampleStop = 1;
    m_TimeStart = 0.0;
    m_TimeStop = 1.0;
    m_fileName = nullptr;
}

bool SinkCx_Block::openFileForAppend()
{
    QString filePath = QString::fromUtf8(FileName);
    m_qfile.setFileName(filePath);

    if (!m_qfile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        LOG_INFO("打开文件失败:",m_qfile.errorString().toStdString());
        return false;
    }

    // 移动到文件末尾
    m_qfile.seek(m_qfile.size());

    // 如果文件不是以[开头，说明有问题
    if (m_qfile.size() == 0) {
        m_stream << "[";
    }

    m_stream.setDevice(&m_qfile);
    m_stream.setCodec("UTF-8");

    return true;
}

void SinkCx_Block::cleanup()
{
    // 清理缓冲区
    if (m_pdBuffer) {
        delete[] m_pdBuffer;
        m_pdBuffer = nullptr;
    }

    m_iBuffer = 0;

    // 确保文件关闭
    if (m_qfile.isOpen()) {
        m_qfile.close();
    }

    // 清理FileName内存
    if (FileName) {
        delete[] FileName;
        FileName = nullptr;
    }
}

void SinkCx_Block::RunDealData()
{
    if(m_iBuffer == FILEWRITER_BUFFER_SIZE) {
        // 创建并初始化文件
        m_qfile.setFileName(m_fullPath);
        if (!m_qfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            LOG_INFO("无法创建文件:",m_qfile.errorString().toStdString());
            return;
        }

        m_stream.setDevice(&m_qfile);
        m_stream.setCodec("UTF-8");
        m_stream << "[" << "\n";
        m_stream.flush();

        // 确保文件打开
        if (!m_qfile.isOpen()) {
            if (!openFileForAppend()) {
                return;
            }
        }

        // 写入缓冲区数据
        for(int j = 0; j < FILEWRITER_BUFFER_SIZE; j++) {
            if(IsBitShiftRegister()) {
//                WriteBitShiftRegisterData(j);
            }
            else {
                // 如果不是第一条数据，需要加逗号
                if (Index > 1 || j > 0) {
                    m_stream << ",\r\n";
                }

                m_stream << "\t{\r\n";
                m_stream << "\t\t\"Index\": " << Index << ",\r\n";

                switch (m_StartStopOption) {
                case SinkCx::Auto: {
                    double timeValue = (Index - 1) / m_sampleRate;
//                    m_stream << "\t\t\"Sink_Time\": " << formatSinkTime(timeValue) << ",\r\n";
                    m_stream << "\t\t\"Sink_Time\": " << timeValue << ",\r\n";
                    break;
                }
                case SinkCx::Samples:
                    m_stream << "\t\t\"Sink_Index\": " << m_SampleStart + Index - 1 << ",\r\n";
                    break;
                case SinkCx::Time: {
                    double timeValue = m_TimeStart + (Index - 1) / m_sampleRate;
//                    m_stream << "\t\t\"Sink_Time\": " << formatSinkTime(timeValue) << ",\r\n";
                    m_stream << "\t\t\"Sink_Time\": " << timeValue << ",\r\n";
                    break;
                }
                default:
                    break;
                }

                m_stream << "\t\t" << R"("(re)Sink_Data": )" << m_pdBuffer[j].real() << "," << "\r\n";
                m_stream << "\t\t" << R"("(im)Sink_Data": )" << m_pdBuffer[j].imag() << "\r\n";
                m_stream << "\t}";

                Index++;
            }
        }
        m_stream.flush();
        m_iBuffer = 0; // 重置缓冲区
    }
}

void SinkCx_Block::WriteBitShiftRegisterData(int i)
{
    int NumBits = GetBitShiftRegisterNumBits();
    // 如果不是第一条数据，需要加逗号
    if (Index > 1) {
        m_stream << ",\r\n";
    } else if (i > 0) {
        m_stream << ",\r\n";
    }

    m_stream << "\t{\r\n";
    m_stream << "\t\t\"Index\": " << Index << ",\r\n";

    switch (m_StartStopOption) {
    case SinkCx::Auto: {
        // 计算在位移寄存器中的相对位置
        int bitPosition = (Index - 1) % NumBits;
        int dataIndex = (Index - 1) / NumBits;

        // 原来每个数据的时间间隔
        double originalInterval = 1.0 / m_sampleRate;
        // 平分后的时间间隔
        double dividedInterval = originalInterval / NumBits;

        // 计算新的时间值
        double timeValue = dataIndex / m_sampleRate + bitPosition * dividedInterval;
//        m_stream << "\t\t\"Sink_Time\": " << formatSinkTime(timeValue) << ",\r\n";
        m_stream << "\t\t\"Sink_Time\": " << timeValue << ",\r\n";
        break;
    }
    case SinkCx::Samples:
        m_stream << "\t\t\"Sink_Index\": " << m_SampleStart + Index - 1 << ",\r\n";
        break;
    case SinkCx::Time: {
        // 计算在位移寄存器中的相对位置
        int bitPosition = (Index - 1) % NumBits;
        int dataIndex = (Index - 1) / NumBits;

        // 原来每个数据的时间间隔
        double originalInterval = 1.0 / m_sampleRate;
        // 平分后的时间间隔
        double dividedInterval = originalInterval / NumBits;

        // 计算新的时间值
        double timeValue = m_TimeStart + dataIndex / m_sampleRate + bitPosition * dividedInterval;
//        m_stream << "\t\t\"Sink_Time\": " << formatSinkTime(timeValue) << ",\r\n";
        m_stream << "\t\t\"Sink_Time\": " << timeValue << ",\r\n";
        break;
    }
    default:
        break;
    }

    m_stream << "\t\t" << R"("(re)Sink_Data": )" << m_pdBuffer[i].real() << "," << "\r\n";
    m_stream << "\t\t" << R"("(im)Sink_Data": )" << m_pdBuffer[i].imag() << "\r\n";
    m_stream << "\t}";

    Index++;
}

QString SinkCx_Block::formatSinkTime(double timeValue) const
{
    if (timeValue == 0.0) {
        return "0";
    }

    double absValue = std::abs(timeValue);

    // 阈值：1e-6 (0.000001)
    const double SCIENTIFIC_THRESHOLD = 0.000001;

    if (absValue < SCIENTIFIC_THRESHOLD) {
        // 使用科学计数法
        // 先舍入到合适精度，避免过长的小数
        double roundedValue = timeValue;

        // 对于非常小的数，舍入到 12 位有效数字
        if (absValue < 1e-10) {
            roundedValue = std::round(timeValue * 1e12) / 1e12;
        }

        QString scientificStr = QString::number(roundedValue, 'e', 12);

        // 简化科学计数法格式
        int ePos = scientificStr.indexOf('e', Qt::CaseInsensitive);
        if (ePos != -1) {
            QString mantissa = scientificStr.left(ePos);
            QString exponent = scientificStr.mid(ePos);

            // 去掉尾数末尾的零
            while (mantissa.length() > 1 && mantissa.endsWith('0')) {
                mantissa.chop(1);
            }
            if (mantissa.endsWith('.')) {
                mantissa.chop(1);
            }

            // 简化指数：去掉前导零，如 e-07 -> e-7
            if (exponent.length() >= 4 && (exponent[1] == '-' || exponent[1] == '+')) {
                QString expNum = exponent.mid(2);
                // 去掉前导零
                while (expNum.length() > 1 && expNum.startsWith('0')) {
                    expNum.remove(0, 1);
                }
                exponent = exponent.left(2) + expNum;
            }

            return mantissa + exponent;
        }

        return scientificStr;
    } else {
        // 使用固定6位小数格式
        // 先舍入到6位小数，消除浮点误差
        double roundedValue = std::round(timeValue * 1000000.0) / 1000000.0;

        // 格式化为固定6位小数
        QString result = QString::number(roundedValue, 'f', 6);

        // 确保显示完整的6位小数（不删除末尾的零）
        // 但需要处理一个特殊情况：如果数值本来就是整数，比如 1.0
        // 但根据你的需求，0.0009 应该显示为 0.000900，所以保留所有零

        return result;
    }
}

double SinkCx_Block::roundToPrecision(double value, int decimals) const
{
    double factor = std::pow(10.0, decimals);
    return std::round(value * factor) / factor;
}
