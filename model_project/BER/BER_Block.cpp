#include "BER_Block.h"
#include <QDir>

#define FILEWRITER_BUFFER_SIZE 1000000

BER_Block::BER_Block(const std::string &name)
    :Block(name)
{

}

bool BER_Block::Setup()
{
    Block::Setup();


    // 计算要收集的样本总数
    long long totalSamples = 0;

    switch (m_StartStopOption) {
    case BER::Auto:
    case BER::Time:
        // 计算一下以确保符合限制
        if (m_sampleRate > 0) {
            int calculatedStart = static_cast<int>(m_TimeStart * m_sampleRate);
            int calculatedStop = static_cast<int>(m_TimeStop * m_sampleRate);
            totalSamples = calculatedStop - calculatedStart + 1;
        }
        break;

    case BER::Samples:
        totalSamples = m_SampleStop - m_SampleStart + 1;
        break;
    }

    // Windows 2GB 文件大小限制
    unsigned long maxSamples = (static_cast<unsigned long>(1) << 31) / sizeof(double);

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
    QString folderPath = outputPath + "/01";

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
    m_WritePath = "/01/" + fileName;

    // 保存路径（转换为char*给原有代码使用）
    QByteArray pathBytes = fullPath.toUtf8();
    FileName = new char[pathBytes.size() + 1];
    strcpy(FileName, pathBytes.constData());

    m_fullPath = fullPath;

    return true;
}

bool BER_Block::Run()
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
    // 确保文件打开
    if (!m_qfile.isOpen()) {
        if (!openFileForAppend()) {
            cleanup();
            return false;
        }
    }

    if (m_control.CollectData()) // Check if we should still collect data
    {
        auto testData = ReadInputData<int>(GetInputPortName(0));
        auto refData = ReadInputData<int>(GetInputPortName(1));

        // 异或运算统计误码
        if (refData[0] ^ testData[0])
        {
            m_ber->BitErrorCount++;
        }
        m_ber->PeriodIndex++;

        // 收集满检测周期个点后输出
        if (m_ber->PeriodIndex >= StatusUpdatePeriod)
        {
            if (m_ber->SinkIndex == 0)
            {

            }
            m_stream << "\t{" << "\r\n";
            m_stream << "\t\t" << R"("Index": )" << m_ber->ResultIndex << "," << "\r\n";
            m_stream << "\t\t" << R"("BER":)" << 1.0*m_ber->BitErrorCount / StatusUpdatePeriod << "\r\n";
            if (m_ber->SinkIndex >= (m_SampleStop - m_SampleStart) - (m_SampleStop - m_SampleStart + 1) % StatusUpdatePeriod)
            {
                // 若为最后一组数据，去除多余逗号
                m_stream << "\t}" << "\r\n";
            }
            else
            {
                m_stream << "\t}," << "\r\n";
            }
            // 重置计数器
            m_ber->BitErrorCount = 0;
            m_ber->PeriodIndex = 0;
            // 输出索引步进
            m_ber->ResultIndex++;
        }

        // 当前收集到的点的索引，每个Run计数加一
        m_ber->SinkIndex++;
    }
    return true;
}

bool BER_Block::Initialize()
{
    SetBlockType(Block::BlockType::SINK);

    m_ber = std::make_unique<BER>();

    SetDefaultParameters();
    std::string File = getSimu().linkName + "_" + getInstanceName();

    m_StartStopOption = ConvertStringToSelectedStartStopOption(getParameter("StartStopOption").Value);
    m_fileName=CopyStringToCharPtr(getParameter("FileName").Value);
    //存储数据的json文件名
    FileName = CopyStringToCharPtr(File);

    m_sampleRate = getSimu().samplingRate;
    if(m_StartStopOption == BER::Auto) {
        Block::SetTerminalMode(TerminalMode::AUTO);
        m_TimeStart = getSimu().startTime;
        m_TimeStop = getSimu().stopTime;
    }
    else if(m_StartStopOption == BER::Samples) {
        Block::SetTerminalMode(TerminalMode::SAMPLES);
        m_SampleStart = std::stoi(getParameter("SampleStart").Value);
        m_SampleStop = std::stoi(getParameter("SampleStop").Value);
    }
    else if(m_StartStopOption == BER::Time) {
        Block::SetTerminalMode(TerminalMode::TIME);
        m_TimeStart = std::stod(getParameter("TimeStart").Value);
        m_TimeStop = std::stod(getParameter("TimeStop").Value);
    }

    SetParameters(m_SampleStart, m_SampleStop, m_TimeStart, m_TimeStop, m_StartStopOption, m_fileName);

    if(!ModelSetup()) return false;

    AddInputPort("test", m_ber->test, 1, DataType::CIRCULAR_BUFFER_INT);
    AddInputPort("ref", m_ber->ref, 1, DataType::CIRCULAR_BUFFER_INT);

    return true;
}

bool BER_Block::Done()
{
    // 结束JSON数组
    m_stream << "\r\n]";
    m_stream.flush();

    // 关闭文件
    m_qfile.close();

    // 验证文件
    QString filePath = QString::fromUtf8(FileName);
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

bool BER_Block::ModelSetup()
{
    bool bStatus = true;
    if (m_SampleStart < 0)
    {
        LOG_ERROR("SampleStart must be >= 0");
        bStatus = false;
    }

    if (m_SampleStop < m_SampleStart)
    {
        LOG_ERROR("SampleStop must be >= SampleStart");
        bStatus = false;
    }

    if (m_TimeStart < 0)
    {
        LOG_ERROR("TimeStart must be >= 0");
        bStatus = false;
    }

    if (m_TimeStop < m_TimeStart)
    {
        LOG_ERROR("TimeStop must be >= TimeStart");
        bStatus = false;
    }

    if (StatusUpdatePeriod <= 0)
    {
        LOG_ERROR("StatusUpdatePeriod must be > 0");
        bStatus = false;
    }
    if (m_SampleStop - m_SampleStart + 1 < StatusUpdatePeriod)
    {
        LOG_WARN("Not enough samples to yeild BER (SampleStop - SampleStart + 1 < StatusUpdatePeriod), the result may be empty.");
    }

    m_control.Initialize(nullptr, m_SampleStart, m_SampleStop);

    return bStatus;
}

void BER_Block::SetParameters(int SampleStart, int SampleStop, double TimeStart, double TimeStop, BER::SelectedStartStopOption StartStopOption, char *Filename)
{
    m_SampleStart = SampleStart;
    m_SampleStop = SampleStop;
    m_TimeStart = TimeStart;
    m_TimeStop = TimeStop;
    m_StartStopOption = StartStopOption;
    m_fileName = Filename;

    if(m_ber) {
        m_ber->SampleStart = m_SampleStart;
        m_ber->SampleStop = m_SampleStop;
        m_ber->TimeStart = m_TimeStart;
        m_ber->TimeStop = m_TimeStop;
        m_ber->FileName = m_fileName;
        m_ber->StatusUpdatePeriod = StatusUpdatePeriod;
    }


}

BER::SelectedStartStopOption BER_Block::ConvertStringToSelectedStartStopOption(const std::string &value)
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
        return BER::Auto;
    } else if (lowerValue == "samples" || lowerValue == "1") {
        return BER::Samples;
    } else if (lowerValue == "time" || lowerValue == "2") {
        return BER::Time;
    }
}

void BER_Block::CopyStringToCharPtr(const std::string &src, char *&dest)
{
    delete[] dest;
    if (!src.empty()) {
        dest = new char[src.length() + 1];
        strcpy(dest, src.c_str());
    } else {
        dest = nullptr;
    }
}

char* BER_Block::CopyStringToCharPtr(const std::string& src) {
    // 1. 分配内存：长度+1（预留'\0'的位置）
    size_t bufSize = src.length() + 1;
    char* dest = new (std::nothrow) char[bufSize]; // nothrow避免内存不足直接崩溃

    for (size_t i = 0; i < src.length(); i++) {
        dest[i] = src[i]; // 直接取string的字符，不依赖c_str()
    }
    dest[src.length()] = '\0'; // 手动加终止符（关键！）

    return dest;
}
char* BER_Block::combinePathWithJsonSuffix(const fs::path& linkKeyFolder, const char* fileName) {
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

void BER_Block::SetDefaultParameters()
{
    m_StartStopOption = BER::Auto;
    m_SampleStart = 0;
    m_SampleStop = 1;
    m_TimeStart = 0.0;
    m_TimeStop = 1.0;
    m_fileName = nullptr;
    StatusUpdatePeriod = 1000;
}

bool BER_Block::openFileForAppend()
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

void BER_Block::cleanup()
{
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

