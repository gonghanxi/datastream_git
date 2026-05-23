#include "RADAR_DOA_Block.h"

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
RADAR_DOA_Block::RADAR_DOA_Block(const std::string& name)
    :Block(name)
{

}

bool RADAR_DOA_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool RADAR_DOA_Block::DataStreamRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string numberPort = GetOutputPortName(0);
    std::string elevationPort = GetOutputPortName(1);
    std::string azimuthPort = GetOutputPortName(2);

    // 从Bus输入端口读取多通道复数数据
    // ReadInputData返回的是vector，其中包含了所有通道的数据
    // 由于是Bus端口，每个通道的数据会按顺序连续存储
    std::vector<std::complex<double>> inputData = ReadInputData<std::complex<double>>(inputPortName);

    if (inputData.empty()) {
        return false; // 没有数据时直接返回
    }

    // 根据参数设置数据维度
    int M = m_NumOfCh;           // 通道数（阵元数）
    int L = m_SnapShotLen;        // 快拍数（每个通道的采样点数）

    // 验证输入数据大小是否符合预期
    // Bus端口连接多个输出时，每个通道的数据会依次读取，总数据量应为 M × L
    if (inputData.size() != static_cast<size_t>(M * L)) {
        // 数据量不足时，可以选择填充或返回
        // 这里简单返回，实际应用中可能需要错误处理
        LOG_INFO("数据量不足!");
        return true;
    }

    // 将输入数据转换为Eigen矩阵格式（M×L）
    // 数据在inputData中的存储顺序：[通道1采样1, 通道1采样2, ..., 通道1采样L, 通道2采样1, ...]
    Eigen::MatrixXcd X(M, L);
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < L; j++) {
            // 计算在inputData中的索引：i * L + j
            // 这样确保每个通道的L个采样点连续存储
            X(i, j) = inputData[i * L + j];
        }
    }

    // 执行DOA估计
    RADAR_DOA::DOAResult result = m_RADAR_DOA->DOA_MUSIC_1D(X, M, m_D, m_RADAR_DOA->lambda_, L);

    // 准备输出数据
    // 输出信号源数量
    std::vector<int> numberData;
    numberData.push_back(result.number);
    WriteOutputData(numberPort, numberData);

    // 输出方位角矩阵
    if (!result.azimuth.empty()) {
        SystemVueModelBuilder::DoubleMatrix azMatrix(1, result.azimuth.size());
        for (size_t i = 0; i < result.azimuth.size(); i++) {
            azMatrix(0, i) = result.azimuth[i];
        }
        std::vector<SystemVueModelBuilder::DoubleMatrix> azimuthData;
        azimuthData.push_back(azMatrix);
        WriteOutputData(azimuthPort, azimuthData);
    } else {
        LOG_INFO("result.azimuth is empty");
        return false;
    }

    // 输出俯仰角矩阵（如果算法支持）
    Buffer* elevationOutPort = GetOutputPort(elevationPort);
    if(elevationOutPort->GetReaderCount() > 0) {
        if (!result.elevation.empty()) {
            SystemVueModelBuilder::DoubleMatrix elMatrix(1, result.elevation.size());
            for (size_t i = 0; i < result.elevation.size(); i++) {
                elMatrix(0, i) = result.elevation[i];
            }
            std::vector<SystemVueModelBuilder::DoubleMatrix> elevationData;
            elevationData.push_back(elMatrix);
            WriteOutputData(elevationPort, elevationData);
        } else {
            LOG_INFO("result.elevation is empty");
            return false;
        }
    }
    return true;
}

bool RADAR_DOA_Block::TimeDrivenRun()
{
    std::string inputPortName = GetInputPortName(0);
    std::string numberPort = GetOutputPortName(0);
    std::string elevationPort = GetOutputPortName(1);
    std::string azimuthPort = GetOutputPortName(2);

    // 从Bus输入端口读取多通道复数数据
    // ReadInputData返回的是vector，其中包含了所有通道的数据
    // 由于是Bus端口，每个通道的数据会按顺序连续存储
    // 保证多输入同时读取数据
    BufferReader* master_reader = GetInputPort(inputPortName);
    auto bridge_readers = master_reader->GetBusConnections();
    for(const auto& bridge_reader : bridge_readers) {
        std::vector<std::complex<double>> inputData;
        bridge_reader.bridgeReader->ReadData(inputData);
        if(inputData.empty()) {
            //上游没产生，
            return true;
        }
        for(size_t i = 0; i < inputData.size();i++) {
            m_inputBuffer[bridge_reader.bridgeReader].push_back(inputData[i]);
        }
    }

    bool CanProcessData = true;
    for(auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it) {
        if(it->second.size() >= 1) {
            CanProcessData = false;
            break;
        }
    }
    if(CanProcessData) {
        // 根据参数设置数据维度
        int M = m_NumOfCh;           // 通道数（阵元数）
        int L = m_SnapShotLen;        // 快拍数（每个通道的采样点数）
        Eigen::MatrixXcd X(M, L);

        int row_idx = 0;
        for (auto it = m_inputBuffer.begin(); it != m_inputBuffer.end(); ++it, ++row_idx) {
            const std::vector<std::complex<double>>& ch_data = it->second;

            for (int j = 0; j < L; j++) {
                X(row_idx, j) = ch_data[j];
            }
        }

        // 执行DOA估计
        RADAR_DOA::DOAResult result = m_RADAR_DOA->DOA_MUSIC_1D(X, M, m_D, m_RADAR_DOA->lambda_, L);

        // 准备输出数据
        // 输出信号源数量
        std::vector<int> numberData;
        numberData.push_back(result.number);
        WriteOutputData(numberPort, numberData);

        // 输出方位角矩阵
        if (!result.azimuth.empty()) {
            SystemVueModelBuilder::DoubleMatrix azMatrix(1, result.azimuth.size());
            for (size_t i = 0; i < result.azimuth.size(); i++) {
                azMatrix(0, i) = result.azimuth[i];
            }
            std::vector<SystemVueModelBuilder::DoubleMatrix> azimuthData;
            azimuthData.push_back(azMatrix);
            WriteOutputData(azimuthPort, azimuthData);
        } else {
            LOG_INFO("result.azimuth is empty");
            return false;
        }

        // 输出俯仰角矩阵（如果算法支持）
        Buffer* elevationOutPort = GetOutputPort(elevationPort);
        if(elevationOutPort->GetReaderCount() > 0) {
            if (!result.elevation.empty()) {
                SystemVueModelBuilder::DoubleMatrix elMatrix(1, result.elevation.size());
                for (size_t i = 0; i < result.elevation.size(); i++) {
                    elMatrix(0, i) = result.elevation[i];
                }
                std::vector<SystemVueModelBuilder::DoubleMatrix> elevationData;
                elevationData.push_back(elMatrix);
                WriteOutputData(elevationPort, elevationData);
            } else {
                LOG_INFO("result.elevation is empty");
                return false;
            }
        }
    }
    return true;
}

bool RADAR_DOA_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RADAR_DOA_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_RADAR_DOA = std::make_unique<RADAR_DOA>();

    SetDefaultParameters();

    try { m_Fc = std::stod(getParameter("Fc").Value); } catch (...) { }
    try { m_D = std::stod(getParameter("D").Value); } catch (...) { }
    try { m_NumOfCh = std::stoi(getParameter("NumOfCh").Value); } catch (...) { }
    try { m_SnapShotLen = std::stoi(getParameter("SnapShotLen").Value); } catch (...) { }
    try { m_MTI_Type = ConvertStringToSelectedMTI_Type(getParameter("MTI_Type").Value); } catch (...) { }

    SetParameters();

    if (!m_RADAR_DOA->Setup()) {
        return false;
    }

    AddInputPort("input", m_RADAR_DOA->input, static_cast<size_t>(m_SnapShotLen), Block::DataType::DCOMPLEX_BUS);
    AddOutputPort("number", m_RADAR_DOA->number, 1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("elevation", m_RADAR_DOA->elevation, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("azimuth", m_RADAR_DOA->azimuth, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}

void RADAR_DOA_Block::SetParameters()
{
    if(!m_RADAR_DOA) {
        return;
    }
    m_RADAR_DOA->Fc = m_Fc;
    m_RADAR_DOA->D = m_D;
    m_RADAR_DOA->NumOfCh = m_NumOfCh;
    m_RADAR_DOA->SnapShotLen = m_SnapShotLen;
    m_RADAR_DOA->MTI_Type = m_MTI_Type;
}

RADAR_DOA::SelectedMTI_Type RADAR_DOA_Block::ConvertStringToSelectedMTI_Type(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "music") {
        return RADAR_DOA::MUSIC;
    }
    if (lower == "esprit" || lower == "1") {
        return RADAR_DOA::ESPRIT;
    }
    if (lower == "music_2d" || lower == "2") {
        return RADAR_DOA::MUSIC_2D;
    }
    return RADAR_DOA::MUSIC;
}

void RADAR_DOA_Block::SetDefaultParameters()
{
    m_Fc = 10e9;
    m_D = 0.5;
    m_NumOfCh = 16;
    m_SnapShotLen = 100;
    m_MTI_Type = RADAR_DOA::MUSIC;
}
