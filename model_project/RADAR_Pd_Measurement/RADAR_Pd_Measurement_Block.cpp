#include "RADAR_Pd_Measurement_Block.h"
#include <QDir>

RADAR_Pd_Measurement_Block::RADAR_Pd_Measurement_Block(const std::string &name)
    :Block(name)
{
    DetectStatus = false;
    DetectCount = 0;
}

bool RADAR_Pd_Measurement_Block::Setup()
{
   Block::Setup();

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
   m_UserId = "";
   QString UserId = QString::fromStdString(m_UserId);

   qDebug() << "UserId: " << UserId;

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

bool RADAR_Pd_Measurement_Block::Initialize()
{
     SetBlockType(Block::BlockType::SINK);
     m_radar = std::make_unique<RADAR_Pd_Measurement>();

     SetDefaultParameters();

     try {
         Start = std::stoi(getParameter("Start").Value);
         PRI_NUM = std::stoi(getParameter("PRI_NUM").Value);;
         FFT_Size = std::stoi(getParameter("FFT_Size").Value);;
         DetectionNum = std::stoi(getParameter("DetectionNum").Value);;
         TargetsInPRI = std::stoi(getParameter("TargetsInPRI").Value);;
         TargetThreshold = std::stod(getParameter("TargetThreshold").Value);;
     } catch (...) {}

     SetParameters();

     AddInputPort("input", m_radar->input, 1, DataType::CIRCULAR_BUFFER_DOUBLE);

     return true;
}

bool RADAR_Pd_Measurement_Block::Run()
{
    std::string inputPortName = GetInputPortName(0);
    BufferReader* inputReader = GetInputPort(inputPortName);
    if(m_control.CollectData()) {
        // 获取输入端口名称

        if(inputReader->GetConnectedBuffer()->GetDataType() != DataType::DOUBLE) {
            // 读取数据
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

bool RADAR_Pd_Measurement_Block::Done()
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
    m_stream << "\t{" << "\r\n";
    m_stream << "\t\t" << R"("Index": )" << 1 << "," << "\r\n";
    m_stream << "\t\t" << R"("Pd":)" << 1.0*DetectCount / (TargetsInPRI*DetectionNum) << "\r\n";
    m_stream << "\t}" /*<< "\r\n"*/;

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

void RADAR_Pd_Measurement_Block::SetParameters()
{
    if(!m_radar) return;
    m_radar->Start = Start;
    m_radar->PRI_NUM = PRI_NUM;
    m_radar->FFT_Size = FFT_Size;
    m_radar->DetectionNum = DetectionNum;
    m_radar->TargetsInPRI = TargetsInPRI;
    m_radar->TargetThreshold = TargetThreshold;
}

void RADAR_Pd_Measurement_Block::SetDefaultParameters()
{
    Start = 0;
    PRI_NUM = 10000;
    FFT_Size = 16;
    DetectionNum = 1;
    TargetsInPRI = 1;
    TargetThreshold = 1e-8;
}

void RADAR_Pd_Measurement_Block::cleanup()
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

bool RADAR_Pd_Measurement_Block::openFileForAppend()
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

bool RADAR_Pd_Measurement_Block::ModelSetup()
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
    if (TargetThreshold <= 0)
    {
        LOG_ERROR("TargetThreshold must be > 0");
        bStatus = false;
    }
    m_control.Initialize(nullptr, Start, Start + PRI_NUM * FFT_Size * DetectionNum);
    return bStatus;
}
