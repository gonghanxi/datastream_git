#include "RADAR_JammingEffect_Block.h"
#include <QDir>
#include <QFileInfo>
#include <algorithm>
#include <cctype>

// ============================================================================
// 字符串处理 (from RADAR_JammingEffect original)
// ============================================================================

namespace {
std::string TrimCopy(const std::string& value)
{
    std::string s = value;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
}

std::string ToLowerCopy(const std::string& value)
{
    std::string s = value;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}
}

// ============================================================================
// 构造函数
// ============================================================================

RADAR_JammingEffect_Block::RADAR_JammingEffect_Block(const std::string &name)
    :Block(name)
{
    DetectStatus = false;
    DetectCount = 0;
    SweepIndex = 0;
}

RADAR_JammingEffect_Block::~RADAR_JammingEffect_Block()
{
    if (FileName) {
        delete[] FileName;
        FileName = nullptr;
    }
}

// ============================================================================
// Setup
// ============================================================================

bool RADAR_JammingEffect_Block::Setup()
{
   Block::Setup();

   // 创建完整路径
   QString outputPath = QString::fromStdString(getOutPutPath());
   QString folderPath = outputPath + "/01";

   QDir dir(folderPath);
   if (!dir.exists()) {
       dir.mkpath(".");
   }

   // 构造文件名 - 加入子系统名称
   QString fileName;
   QString linkName = QString::fromStdString(getSimu().linkName);
   QString subsystemName = QString::fromStdString(getSubsystemName());
   QString instanceName = QString::fromStdString(getInstanceName());
   m_UserId = "";
   QString UserId = QString::fromStdString(m_UserId);

   qDebug() << "UserId: " << UserId;

   if (!subsystemName.isEmpty()) {
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
   } else {
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

   }

   QString fullPath = folderPath + "/" + fileName;

   qDebug() << "fullPath: " << fullPath;

   //后端存储路径
   m_WritePath = "/01/" + fileName;

   // 保存路径（转换为char*给原有代码使用）
   QByteArray pathBytes = fullPath.toUtf8();
   FileName = new char[pathBytes.size() + 1];
   strcpy(FileName, pathBytes.constData());

   m_fullPath = fullPath;

   qDebug() << "m_fullPath: " << m_fullPath;

   if(!ModelSetup()) return false;

   return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool RADAR_JammingEffect_Block::Initialize()
{
     SetBlockType(Block::BlockType::SINK);
     m_radar = std::make_unique<RADAR_JammingEffect>();

     SetDefaultParameters();

     try {
         JammingType     = ConvertStringToJammingType(getParameter("JammingType").Value);
         Start           = std::stoi(getParameter("Start").Value);
         PRI_NUM         = std::stoi(getParameter("PRI_NUM").Value);
         FFT_Size        = std::stoi(getParameter("FFT_Size").Value);
         DetectionNum    = std::stoi(getParameter("DetectionNum").Value);
         TargetsInPRI    = std::stoi(getParameter("TargetsInPRI").Value);
         FalseTargetNum  = std::stoi(getParameter("FalseTargetNum").Value);
         TargetThreshold = std::stod(getParameter("TargetThreshold").Value);
     } catch (...) {}

     SetParameters();

     // 每个 sweep 重置检测状态
     DetectCount  = 0;
     DetectStatus = false;
     SweepIndex++;

     AddInputPort("input", m_radar->input, 1, DataType::CIRCULAR_BUFFER_DOUBLE);

     return true;
}

// ============================================================================
// Run (from RADAR_Pd_Measurement_Block pattern)
// ============================================================================

bool RADAR_JammingEffect_Block::Run()
{
    std::string inputPortName = GetInputPortName(0);
    BufferReader* inputReader = GetInputPort(inputPortName);
    if(m_control.CollectData()) {
        if(inputReader->GetConnectedBuffer()->GetDataType() != DataType::DOUBLE) {
            auto inputData = ReadInputData<double>(inputPortName);
            if(inputData.empty()) {
                return false;
            }
            // 连续的过阈值点视作一个目标
            if (inputData[0] > TargetThreshold && !DetectStatus)
            {
                DetectStatus = true;
                DetectCount++; // 信号过阈值的上升沿时计数
            }
            if (inputData[0] <= TargetThreshold && DetectStatus)
            {
                DetectStatus = false;
            }
            return true;
        }
        else {
            double inputData;
            if(!inputReader->ReadData(inputData)) {
                LOG_ERROR("Sink '" , GetName(), "' read Double data Failed!");
                return false;
            }
            // 连续的过阈值点视作一个目标
            if (inputData > TargetThreshold && !DetectStatus)
            {
                DetectStatus = true;
                DetectCount++; // 信号过阈值的上升沿时计数
            }
            if (inputData <= TargetThreshold && DetectStatus)
            {
                DetectStatus = false;
            }
            return true;
        }
    }
    if(inputReader->GetConnectedBuffer()->GetDataType() != DataType::DOUBLE) {
        auto inputData = ReadInputData<double>(inputPortName);
        if(inputData.empty()) {
            return false;
        }
    }
    else {
        double inputData;
        if(!inputReader->ReadData(inputData)) {
            LOG_ERROR("Sink '" , GetName(), "' read Double data Failed!");
            return false;
        }
    }
    return true;
}

// ============================================================================
// Done — 每个 sweep 追加一条结果 (支持多 sweep 模式)
// ============================================================================

bool RADAR_JammingEffect_Block::Done()
{
    QFileInfo fileInfo(m_fullPath);
    bool isFirstSweep = !fileInfo.exists() || fileInfo.size() <= 2;

    if (isFirstSweep) {
        // 首次 sweep：新建文件
        m_qfile.setFileName(m_fullPath);
        if (!m_qfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            LOG_INFO("无法创建文件:", m_qfile.errorString().toStdString());
            return false;
        }
        m_stream.setDevice(&m_qfile);
        m_stream.setCodec("UTF-8");
        m_stream << "[" << "\r\n";
        m_stream.flush();
    } else {
        // 非首次 sweep：以读写模式打开，回退到文件末尾 ] 之前
        m_qfile.setFileName(m_fullPath);
        if (!m_qfile.open(QIODevice::ReadWrite | QIODevice::Text)) {
            LOG_INFO("无法打开文件:", m_qfile.errorString().toStdString());
            cleanup();
            return false;
        }
        // 文件末尾是 \r\n] (3 bytes)，回退到 ] 之前进行追加
        m_qfile.seek(m_qfile.size() - 3);
        m_stream.setDevice(&m_qfile);
        m_stream.setCodec("UTF-8");
        m_stream << "," << "\r\n";
    }

    // 写入当前 sweep 结果（逐 sweep 公式，不乘 DetectionNum）
    m_stream << "\t{" << "\r\n";
    m_stream << "\t\t" << R"("Index": )" << SweepIndex << "," << "\r\n";
    m_stream << "\t\t" << R"("Detection Hit":)" << DetectCount << "," << "\r\n";
    m_stream << "\t\t" << R"("True Target Count":)" << TargetsInPRI << "," << "\r\n";

    switch (JammingType)
    {
    case RADAR_JammingEffect::CoverJamming:
        m_stream << "\t\t" << R"("Pd":)" << 1.0*DetectCount / TargetsInPRI << "\r\n";
        break;
    case RADAR_JammingEffect::DeceptionJamming:
        m_stream << "\t\t" << R"("False Target Count":)" << FalseTargetNum << "\r\n";
        break;
    default:
        break;
    }

    m_stream << "\t}" /*<< "\r\n"*/;

    // 结束 JSON 数组
    m_stream << "\r\n]";
    m_stream.flush();

    // 关闭文件
    m_qfile.close();

    // 验证文件
    QString filePath = QString::fromUtf8(FileName);
    std::cout << "[RESULT]结果写入文件路径: " << m_WritePath.toStdString() << std::endl;
    QFile checkFile(filePath);
    if (checkFile.open(QIODevice::ReadOnly)) {
        // 读取并显示前200个字符
        QByteArray preview = checkFile.read(200);
        if (checkFile.size() <= 2) {
        }
        checkFile.close();
    } else {
    }

    cleanup();

    return true;
}

// ============================================================================
// SetParameters
// ============================================================================

void RADAR_JammingEffect_Block::SetParameters()
{
    if(!m_radar) return;
    m_radar->JammingType      = JammingType;
    m_radar->Start            = Start;
    m_radar->PRI_NUM          = PRI_NUM;
    m_radar->FFT_Size         = FFT_Size;
    m_radar->DetectionNum     = DetectionNum;
    m_radar->TargetsInPRI     = TargetsInPRI;
    m_radar->FalseTargetNum   = FalseTargetNum;
    m_radar->TargetThreshold  = TargetThreshold;
}

// ============================================================================
// SetDefaultParameters
// ============================================================================

void RADAR_JammingEffect_Block::SetDefaultParameters()
{
    JammingType     = RADAR_JammingEffect::CoverJamming;
    Start           = 0;
    PRI_NUM         = 10000;
    FFT_Size        = 16;
    DetectionNum    = 1;
    TargetsInPRI    = 1;
    FalseTargetNum  = 1;
    TargetThreshold = 1e-8;
}

// ============================================================================
// cleanup
// ============================================================================

void RADAR_JammingEffect_Block::cleanup()
{
    // 确保文件关闭
    if (m_qfile.isOpen()) {
        m_qfile.close();
    }
}

// ============================================================================
// openFileForAppend
// ============================================================================

bool RADAR_JammingEffect_Block::openFileForAppend()
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

// ============================================================================
// ModelSetup — 参数校验 + SinkControl 初始化
// ============================================================================

bool RADAR_JammingEffect_Block::ModelSetup()
{
    bool bStatus = true;

    if (Start < 0)
    {
        LOG_ERROR("Start must be >= 0");
        bStatus = false;
    }
    if (PRI_NUM <= 0)
    {
        LOG_ERROR("PRI_NUM must be > 0");
        bStatus = false;
    }
    if (FFT_Size <= 0)
    {
        LOG_ERROR("FFT_Size must be > 0");
        bStatus = false;
    }
    if (DetectionNum <= 0)
    {
        LOG_ERROR("DetectionNum must be > 0");
        bStatus = false;
    }
    if (TargetsInPRI <= 0)
    {
        LOG_ERROR("TargetsInPRI must be > 0");
        bStatus = false;
    }
    if (FalseTargetNum <= 0 && JammingType == RADAR_JammingEffect::DeceptionJamming)
    {
        LOG_ERROR("FalseTargetNum must be > 0");
        bStatus = false;
    }
    if (TargetThreshold <= 0)
    {
        LOG_ERROR("TargetThreshold must be > 0");
        bStatus = false;
    }
    // 每个 sweep 收集一轮检测的数据（PRI_NUM * FFT_Size），由仿真框架控制 sweep 次数
    m_control.Initialize(nullptr, Start, Start + PRI_NUM * FFT_Size);
    return bStatus;
}

// ============================================================================
// ConvertStringToJammingType
// ============================================================================

RADAR_JammingEffect::SelectedJammingType RADAR_JammingEffect_Block::ConvertStringToJammingType(const std::string& value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "coverjamming"     || lower == "0") return RADAR_JammingEffect::CoverJamming;
    if (lower == "deceptionjamming" || lower == "1") return RADAR_JammingEffect::DeceptionJamming;
    return RADAR_JammingEffect::CoverJamming;
}
