#include "Mapper_Block.h"
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
Mapper_Block::Mapper_Block(const std::string &name)
    :Block(name)
{

}

bool Mapper_Block::Setup()
{
    Block::Setup();
    if(!ModelSetup()) return false;
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool Mapper_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool Mapper_Block::DataStreamRun()
{
    auto inputData = ReadInputData<bool>(GetInputPortName(0));

    if (static_cast<int>(m_table.size()) != m_M)
        return false;

    bool bits[12] = { false };
    for (int i = 0; i < m_symbolLength; ++i)
        bits[i] = inputData[static_cast<unsigned>(i)];

    const int state = m_mapper->GetTableIndex(bits, m_symbolLength);
    int idx = state;

    if (ModType == Mapper::CustomAPSK && DefaultState == Mapper::FALSE_)
    {
        if (state < 0 || state >= m_M)
        {
            LOG_ERROR("Input state out of range.");
            return false;
        }
        idx = m_stateToIndex[static_cast<size_t>(state)];
        if (idx < 0 || idx >= m_M)
        {
            LOG_ERROR("Custom APSK state map is invalid.");
            return false;
        }
    }
    else if (state < 0 || state >= m_M)
    {
        LOG_ERROR("Input symbol value out of range.");
        return false;
    }

    std::vector<std::complex<double>> outputData(1);
    outputData[0] = m_table[static_cast<size_t>(idx)];
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool Mapper_Block::TimeDrivenRun()
{
    size_t inputRate = m_mapper->m_input.GetRate();
    size_t outputRate = m_mapper->m_output.GetRate();
    auto inputData = ReadInputData<bool>(GetInputPortName(0));
    if(inputData.empty()) return true;
    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    if(m_inputBuffer.size() >= inputRate) {
        if (static_cast<int>(m_table.size()) != m_M)
            return false;

        bool bits[12] = { false };
        for (int i = 0; i < m_symbolLength; ++i)
            bits[i] = inputData[static_cast<unsigned>(i)];

        const int state = m_mapper->GetTableIndex(bits, m_symbolLength);
        int idx = state;

        if (ModType == Mapper::CustomAPSK && DefaultState == Mapper::FALSE_)
        {
            if (state < 0 || state >= m_M)
            {
                LOG_ERROR("Input state out of range.");
                return false;
            }
            idx = m_stateToIndex[static_cast<size_t>(state)];
            if (idx < 0 || idx >= m_M)
            {
                LOG_ERROR("Custom APSK state map is invalid.");
                return false;
            }
        }
        else if (state < 0 || state >= m_M)
        {
            LOG_ERROR("Input symbol value out of range.");
            return false;
        }

        std::vector<std::complex<double>> outputData(1);
        outputData[0] = m_table[static_cast<size_t>(idx)];

        m_outputQueue.push(outputData[0]);
        std::complex<double> outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;
        WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});
        m_lastOutput = outputValue;
        m_inputBuffer.clear();

        qDebug() << "[Mapper_Block] 分发输出:" << m_outputCount
                 << " value:" << outputValue.real() << "," << outputValue.imag();
    }
    return true;
}

bool Mapper_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_mapper = std::make_unique<Mapper>();

    SetDefaultParameters();

    try { ModType = ConvertStringToModTypeEnum(getParameter("ModType").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'ModType', using default value."); }
    try { DefaultState = ConvertStringToDefaultStateEnum(getParameter("DefaultState").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'DefaultState', using default value."); }
    try { BitOrder = ConvertStringToBitOrderEnum(getParameter("BitOrder").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'BitOrder', using default value."); }
    try { MappingTable = ParseStringToMatrix<std::complex<double>>(getParameter("MappingTable").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'MappingTable', using default value."); }
    try { Ratio_R2_R1 = std::stod(getParameter("Ratio_R2_R1").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'Ratio_R2_R1', using default value."); }
    try { Ratio_R3_R1 = std::stod(getParameter("Ratio_R3_R1").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'Ratio_R3_R1', using default value."); }
    try { Ratio_R4_R1 = std::stod(getParameter("Ratio_R4_R1").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'Ratio_R4_R1', using default value."); }
    try { RingStates = ParseStringToMatrix<int>(getParameter("RingStates").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'RingStates', using default value."); }
    try { RingMagnitudes = ParseStringToMatrix<double>(getParameter("RingMagnitudes").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'RingMagnitudes', using default value."); }
    try { RinginitialPhases = ParseStringToMatrix<double>(getParameter("RinginitialPhases").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'RinginitialPhases', using default value."); }
    try { States = ParseStringToMatrix<int>(getParameter("States").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'States', using default value."); }

    SetParameters();

    if(!m_mapper->Setup()) return false;
    size_t inputRate = m_mapper->m_input.GetRate();
    size_t outputRate = m_mapper->m_output.GetRate();

    AddInputPort("m_input", m_mapper->m_input, inputRate, DataType::CIRCULAR_BUFFER_BOOL);
    AddOutputPort("m_output", m_mapper->m_output, outputRate, DataType::CIRCULAR_BUFFER_DCOMPLEX);

    return true;
}

void Mapper_Block::SetParameters()
{
    if(!m_mapper) return;
    m_mapper->ModType = ModType;
    m_mapper->DefaultState = DefaultState;
    m_mapper->BitOrder = BitOrder;
    m_mapper->MappingTable = MappingTable;
    m_mapper->Ratio_R2_R1 = Ratio_R2_R1;
    m_mapper->Ratio_R3_R1 = Ratio_R3_R1;
    m_mapper->Ratio_R4_R1 = Ratio_R4_R1;
    m_mapper->RingStates = RingStates;
    m_mapper->RingMagnitudes = RingMagnitudes;
    m_mapper->RinginitialPhases = RinginitialPhases;
    m_mapper->States = States;
}

Mapper::ModTypeEnum Mapper_Block::ConvertStringToModTypeEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));

    if (lower == "bpsk" || lower == "0") return Mapper::BPSK;
    if (lower == "qpsk" || lower == "1") return Mapper::QPSK;
    if (lower == "8-psk" || lower == "psk8" || lower == "2") return Mapper::PSK8;
    if (lower == "16-psk" || lower == "psk16" || lower == "3") return Mapper::PSK16;
    if (lower == "16-qam" || lower == "qam16" || lower == "4") return Mapper::QAM16;
    if (lower == "32-qam" || lower == "qam32" || lower == "5") return Mapper::QAM32;
    if (lower == "64-qam" || lower == "qam64" || lower == "6") return Mapper::QAM64;
    if (lower == "128-qam" || lower == "qam128" || lower == "7") return Mapper::QAM128;
    if (lower == "256-qam" || lower == "qam256" || lower == "8") return Mapper::QAM256;
    if (lower == "user defined" || lower == "user_defined" || lower == "9") return Mapper::User_Defined;
    if (lower == "512-qam" || lower == "qam512" || lower == "10") return Mapper::QAM512;
    if (lower == "1024-qam" || lower == "qam1024" || lower == "11") return Mapper::QAM1024;
    if (lower == "2048-qam" || lower == "qam2048" || lower == "12") return Mapper::QAM2048;
    if (lower == "4096-qam" || lower == "qam4096" || lower == "13") return Mapper::QAM4096;
    if (lower == "16-apsk" || lower == "apsk16" || lower == "14") return Mapper::APSK16;
    if (lower == "32-apsk" || lower == "apsk32" || lower == "15") return Mapper::APSK32;
    if (lower == "star 16-qam" || lower == "star16qam" || lower == "16") return Mapper::Star16QAM;
    if (lower == "star 32-qam" || lower == "star32qam" || lower == "17") return Mapper::Star32QAM;
    if (lower == "custom apsk" || lower == "customapsk" || lower == "18") return Mapper::CustomAPSK;

    return Mapper::QPSK;  // 默认值
}

Mapper::DefaultStateEnum Mapper_Block::ConvertStringToDefaultStateEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));

    if (lower == "false" || lower == "0" || lower == "f") return Mapper::FALSE_;
    if (lower == "true" || lower == "1" || lower == "t") return Mapper::TRUE_;

    return Mapper::TRUE_;  // 默认值
}

Mapper::BitOrderEnum Mapper_Block::ConvertStringToBitOrderEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));

    if (lower == "lsb first" || lower == "lsb_first" || lower == "lsb" || lower == "0") return Mapper::LSB_first;
    if (lower == "msb first" || lower == "msb_first" || lower == "msb" || lower == "1") return Mapper::MSB_first;

    return Mapper::LSB_first;  // 默认值
}

void Mapper_Block::SetDefaultParameters()
{
    ModType = Mapper::QPSK;
    DefaultState = Mapper::TRUE_;
    BitOrder = Mapper::LSB_first;
    MappingTable.Resize(1,2);
    MappingTable(0,0) = 1;
    MappingTable(0,1) = -1;

    Ratio_R2_R1 = 2;
    Ratio_R3_R1 = 3;
    Ratio_R4_R1 = 4;

    RingStates.Resize(2,1);
    RingStates(0,0) = 4;
    RingStates(1,0) = 4;

    RingMagnitudes.Resize(2,1);
    RingMagnitudes(0,0) = 1;
    RingMagnitudes(1,0) = 2;

    RinginitialPhases.Resize(2,1);
    RinginitialPhases(0,0) = 0;
    RinginitialPhases(0,0) = 0;

    States.Resize(8,1);
    States(0,0) = 0;
    States(1,0) = 1;
    States(2,0) = 2;
    States(3,0) = 3;
    States(4,0) = 4;
    States(5,0) = 5;
    States(6,0) = 6;
    States(7,0) = 7;
}

bool Mapper_Block::ModelSetup()
{
    m_symbolLength = m_mapper->m_symbolLength;
    m_M = m_mapper->m_M;
    m_table = m_mapper->m_table;
    m_stateToIndex = m_mapper->m_stateToIndex;
    qDebug() << "Mapper_Block::ModelSetup - m_symbolLength: " << m_symbolLength;
    qDebug() << "Mapper_Block::ModelSetup - m_M: " << m_M;
    qDebug() << "Mapper_Block::ModelSetup - m_table size: " << m_table.size();
    qDebug() << "Mapper_Block::ModelSetup - m_stateToIndex size: " << m_stateToIndex.size();
    return true;
}
