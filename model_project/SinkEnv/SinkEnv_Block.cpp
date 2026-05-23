#include "SinkEnv_Block.h"
#include <QDir>

#define FILEWRITER_BUFFER_SIZE 1000000


SinkEnv_Block::SinkEnv_Block(const std::string &name)
    :Block(name)
{
    Index = 1;
    m_sampleRate = 0;
    m_pdBuffer = 0;
    m_iBuffer = 0;
}

SinkEnv_Block::~SinkEnv_Block()
{
    delete[] m_pdBuffer;
}

bool SinkEnv_Block::Setup()
{
    Block::Setup();

    // 计算要收集的样本总数
    long long totalSamples = 0;

    switch (m_StartStopOption) {
    case SinkEnv::Auto:
    case SinkEnv::Time:
        // 计算一下以确保符合限制
        if (m_sampleRate > 0) {
            int calculatedStart = static_cast<int>(m_TimeStart * m_sampleRate);
            int calculatedStop = static_cast<int>(m_TimeStop * m_sampleRate);
            totalSamples = calculatedStop - calculatedStart + 1;
        }
        break;

    case SinkEnv::Samples:
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

    // 暂时不关闭文件，保持打开状态用于追加
    // m_qfile.close();

    //更新sink的读取速率
    BufferReader* inputReader = GetInputPort(GetInputPortName(0));
    Buffer* outputBuffer = inputReader->GetConnectedBuffer();
    size_t WriteSize = outputBuffer->GetWriteSize();
    size_t ReadSize = inputReader->GetReadSize();
    if(WriteSize != ReadSize) {
        inputReader->SetReadSize(outputBuffer->GetWriteSize());
        outputBuffer->UpdateBufferSize();
    }

    return true;
}

bool SinkEnv_Block::Run()
{
    if(!CanProcess()) {
        return false;
    }

    // 获取输入端口名称
    std::string inputPortName = GetInputPortName(0);

    // 读取数据
    auto inputData = ReadInputData<SystemVueModelBuilder::EnvelopeSignal>(inputPortName);

    if(inputData.empty()) {
        return false;
    }

//    qDebug() << "inputData: " << "[" << inputData[0].real() << "," << inputData[0].imag() << "]" << inputData.size();

    // 处理数据
    for(size_t i = 0; i < inputData.size(); i++) {
        m_pdBuffer[m_iBuffer++] = inputData[i].complex();
        RunDealData();
    }

    return true;
}

bool SinkEnv_Block::Initialize()
{
    SetBlockType(Block::BlockType::SINK);

    m_sinkenv = std::make_unique<SinkEnv>();

    AddInputPort("input", m_sinkenv->input, 1, DataType::ENVELOPE_SIGNAL);

    SetDefaultParameters();

    m_StartStopOption = ConvertStringToSelected(getParameter("StartStopOption").Value);
    //    CopyStringToCharPtr(getParameter("FileName").Value, m_fileName);
    m_fileName=CopyStringToCharPtr(getParameter("FileName").Value);
    //    m_fileName=getParameter("FileName").Value.data();

    m_sampleRate = getSimu().samplingRate;
    if(m_StartStopOption == SinkEnv::Auto) {
        Block::SetTerminalMode(TerminalMode::AUTO);
        m_TimeStart = getSimu().startTime;
        m_TimeStop = getSimu().stopTime;
    }
    else if(m_StartStopOption == SinkEnv::Samples) {
        Block::SetTerminalMode(TerminalMode::SAMPLES);
        m_SampleStart = std::stoi(getParameter("SampleStart").Value);
        m_SampleStop = std::stoi(getParameter("SampleStop").Value);
    }
    else if(m_StartStopOption == SinkEnv::Time) {
        Block::SetTerminalMode(TerminalMode::TIME);
        m_TimeStart = std::stod(getParameter("TimeStart").Value);
        m_TimeStop = std::stod(getParameter("TimeStop").Value);
    }

    SetParameters(m_SampleStart, m_SampleStop, m_TimeStart, m_TimeStop, m_StartStopOption, m_fileName);

    return true;
}

bool SinkEnv_Block::Done()
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
            m_stream << "\r\n]";
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

    BufferReader* reader = GetInputPort(GetInputPortName(0));
    double fc = reader->getCharacterizationFrequency();

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
            if(Index == 1) {
                m_stream << "\t\t" << R"("Fc": )" << fc << ",\r\n";
            }
            m_stream << "\t\t\"Index\": " << Index << ",\r\n";

            switch (m_StartStopOption) {
            case SinkEnv::Auto: {
                double timeValue = (Index - 1) / m_sampleRate;
//                m_stream << "\t\t\"Sink_Time\": " << formatSinkTime(timeValue) << ",\r\n";
                m_stream << "\t\t\"Sink_Time\": " << timeValue << ",\r\n";
                break;
            }
            case SinkEnv::Samples:
                m_stream << "\t\t\"Sink_Index\": " << m_SampleStart + Index - 1 << ",\r\n";
                break;
            case SinkEnv::Time: {
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
    m_stream << "\r"
                ""
                ""
                ""
                "\n]";
    m_stream.flush();

    // 关闭文件
    m_qfile.close();

    // 验证文件
    QString filePath = QString::fromUtf8(FileName);
    qDebug() << "SinkEnv Done - filePath: " << filePath;
//    LOG_INFO("结果写入文件路径: ", m_WritePath.toStdString());
    std::cout << "[RESULT]结果写入文件路径: " << m_WritePath.toStdString() << std::endl;
    QFile checkFile(filePath);
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

void SinkEnv_Block::SetParameters(int SampleStart, int SampleStop, double TimeStart, double TimeStop, SinkEnv::SelectedStartStopOption StartStopOption, char *Filename)
{
    m_SampleStart = SampleStart;
    m_SampleStop = SampleStop;
    m_TimeStart = TimeStart;
    m_TimeStop = TimeStop;
    m_StartStopOption = StartStopOption;
    m_fileName = Filename;

    if(m_sinkenv) {
        m_sinkenv->SampleStart = m_SampleStart;
        m_sinkenv->SampleStop = m_SampleStop;
        m_sinkenv->TimeStart = m_TimeStart;
        m_sinkenv->TimeStop = m_TimeStop;
        m_sinkenv->FileName = m_fileName;
    }
}

SinkEnv::SelectedStartStopOption SinkEnv_Block::ConvertStringToSelected(const std::string &value)
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
        return SinkEnv::Auto;
    } else if (lowerValue == "samples" || lowerValue == "1") {
        return SinkEnv::Samples;
    } else if (lowerValue == "time" || lowerValue == "2") {
        return SinkEnv::Time;
    }
}

void SinkEnv_Block::CopyStringToCharPtr(const std::string &src, char *&dest)
{
    delete[] dest;
    if (!src.empty()) {
        dest = new char[src.length() + 1];
        strcpy(dest, src.c_str());
    } else {
        dest = nullptr;
    }
}

char *SinkEnv_Block::CopyStringToCharPtr(const std::string &src)
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
char *SinkEnv_Block::combinePathWithJsonSuffix(const fs::path &linkKeyFolder, const char *fileName)
{
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

void SinkEnv_Block::SetDefaultParameters()
{
    m_StartStopOption = SinkEnv::Auto;
    m_SampleStart = 0;
    m_SampleStop = 1;
    m_TimeStart = 0.0;
    m_TimeStop = 1.0;
    m_fileName = nullptr;
}

bool SinkEnv_Block::openFileForAppend()
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

void SinkEnv_Block::cleanup()
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

void SinkEnv_Block::RunDealData()
{
    // 缓冲区满了，立即写入文件
    if(m_iBuffer == FILEWRITER_BUFFER_SIZE) {
        // 创建并初始化文件
        m_qfile.setFileName(m_fullPath);
        if (!m_qfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            LOG_INFO("无法创建文件:",m_qfile.errorString().toStdString());
            return;
        }

        m_stream.setDevice(&m_qfile);
        m_stream.setCodec("UTF-8");
        m_stream << "[" << "\r\n";
        m_stream.flush();

        // 确保文件打开
        if (!m_qfile.isOpen()) {
            if (!openFileForAppend()) {
                return;
            }
        }

        BufferReader* reader = GetInputPort(GetInputPortName(0));
        double fc = reader->getCharacterizationFrequency();


        // 写入缓冲区数据
        for(int j = 0; j < FILEWRITER_BUFFER_SIZE; j++) {
            if(IsBitShiftRegister()) {
//                WriteBitShiftRegisterData(j);
            }
            else {
                // 如果不是第一条数据，需要加逗号
                if (Index > 1 || j > 0) {
                    m_stream << ",\n";
                }
                m_stream << "\t{\n";
                if(Index == 1) {
                    m_stream << "\t\t" << R"("Fc": )" << fc << ",\r\n";
                }
                m_stream << "\t\t\"Index\": " << Index << ",\r\n";

                switch (m_StartStopOption) {
                case SinkEnv::Auto: {
                    double timeValue = (Index - 1) / m_sampleRate;
//                    m_stream << "\t\t\"Sink_Time\": " << formatSinkTime(timeValue) << ",\r\n";
                    m_stream << "\t\t\"Sink_Time\": " << timeValue << ",\r\n";
                    break;
                }
                case SinkEnv::Samples:
                    m_stream << "\t\t\"Sink_Index\": " << m_SampleStart + Index - 1 << ",\r\n";
                    break;
                case SinkEnv::Time: {
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

void SinkEnv_Block::WriteBitShiftRegisterData(int i)
{
    BufferReader* reader = GetInputPort(GetInputPortName(0));
    double fc = reader->getCharacterizationFrequency();
    int NumBits = GetBitShiftRegisterNumBits();
    // 如果不是第一条数据，需要加逗号
    if (Index > 1 || i > 0) {
        m_stream << ",\n";
    }
    m_stream << "\t{\n";
    if(Index == 1) {
        m_stream << "\t\t" << R"("Fc": )" << fc << ",\r\n";
    }
    m_stream << "\t\t\"Index\": " << Index << ",\r\n";

    switch (m_StartStopOption) {
    case SinkEnv::Auto: {
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
    case SinkEnv::Samples:
        m_stream << "\t\t\"Sink_Index\": " << m_SampleStart + Index - 1 << ",\r\n";
        break;
    case SinkEnv::Time: {
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

QString SinkEnv_Block::formatSinkTime(double timeValue) const
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

double SinkEnv_Block::roundToPrecision(double value, int decimals) const
{
    double factor = std::pow(10.0, decimals);
    return std::round(value * factor) / factor;
}
