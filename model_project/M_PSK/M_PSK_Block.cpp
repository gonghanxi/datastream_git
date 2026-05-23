#include "M_PSK_Block.h"
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
M_PSK_Block::M_PSK_Block(const std::string &name)
    :Block(name)
{

}
bool M_PSK_Block::Setup()
{
    Block::Setup();
    if (!m_psk->ValidateParameters())
        return false;

    if (m_modType != m_psk->m_setupModType)
    {
        LOG_ERROR("ModType changed after Setup. This is not supported (would require rate/table rebuild).");
        return false;
    }
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool M_PSK_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool M_PSK_Block::DataStreamRun()
{
    auto inputData = ReadInputData<int>(GetInputPortName(0));


    std::vector<int> bits(m_psk->m_modBits, 0);
    for (std::size_t i = 0; i < m_psk->m_modBits; ++i)
    {
        const int x = inputData[static_cast<int>(i)];
        bits[i] = (x & 1) ? 1 : 0;
    }

    const int g = m_psk->ConvertBitsToInt(bits);

    const int k = m_psk->m_grayTable[static_cast<std::size_t>(g)];

    if (k < 0 || static_cast<std::size_t>(k) >= m_psk->m_constTable.size())
    {
        LOG_ERROR("Internal constellation table index out of range.");
        return false;
    }

    const std::complex<float>& c = m_psk->m_constTable[static_cast<std::size_t>(k)];
    m_psk->m_out = std::complex<double>((double)c.real(), (double)c.imag());

    Buffer* out = GetOutputPort(GetOutputPortName(0));

    out->WriteData(m_psk->m_out);
    return true;
}

bool M_PSK_Block::TimeDrivenRun()
{
    auto inputData = ReadInputData<int>(GetInputPortName(0));
    if(inputData.empty()) return true;
    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    size_t inputRate = m_psk->m_in.GetRate();
    if(m_inputBuffer.size() >= inputRate) {
        std::vector<int> bits(m_psk->m_modBits, 0);
        for (std::size_t i = 0; i < m_psk->m_modBits; ++i)
        {
            const int x = inputData[static_cast<int>(i)];
            bits[i] = (x & 1) ? 1 : 0;
        }

        const int g = m_psk->ConvertBitsToInt(bits);

        const int k = m_psk->m_grayTable[static_cast<std::size_t>(g)];

        if (k < 0 || static_cast<std::size_t>(k) >= m_psk->m_constTable.size())
        {
            LOG_ERROR("Internal constellation table index out of range.");
            return false;
        }

        const std::complex<float>& c = m_psk->m_constTable[static_cast<std::size_t>(k)];
        m_psk->m_out = std::complex<double>((double)c.real(), (double)c.imag());

        Buffer* out = GetOutputPort(GetOutputPortName(0));

        m_outputQueue.push(m_psk->m_out);
        std::complex<double> outputValue = m_outputQueue.front();
        m_outputQueue.pop();
        m_outputCount++;
        out->WriteData(outputValue);
        m_lastOutput = outputValue;
        m_inputBuffer.clear();

        qDebug() << "[M_PSK_Block] 分发输出:" << m_outputCount
                 << " value:" << outputValue.real() << "," << outputValue.imag();
    }
    return true;
}

bool M_PSK_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_psk = std::make_unique<M_PSK>();

    SetDefaultParameters();

    try { m_modType = ConvertStringToModType(getParameter("ModType").Value); } catch(...) {}
    try { m_bitOrder = ConvertStringToBitOrder(getParameter("BitOrder").Value); } catch(...) {}

    SetParameters();

    if(!m_psk->Setup()) return false;
    size_t inputRate = m_psk->m_in.GetRate();

    AddInputPort("In", m_psk->m_in, inputRate, DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("Out", m_psk->m_out, 1, DataType::COMPLEX_DOUBLE);

    return true;
}

void M_PSK_Block::SetParameters()
{
    if(!m_psk) return;
    m_psk->m_modType = m_modType;
    m_psk->m_bitOrder = m_bitOrder;
}

M_PSK::ModType M_PSK_Block::ConvertStringToModType(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));

    if (lower == "bpsk" || lower == "0") return M_PSK::BPSK;
    if (lower == "qpsk" || lower == "1") return M_PSK::QPSK;
    if (lower == "psk8" || lower == "psk8" || lower == "2") return M_PSK::PSK8;
    if (lower == "psk16" || lower == "psk16" || lower == "3") return M_PSK::PSK16;
    if (lower == "psk32" || lower == "qam16" || lower == "4") return M_PSK::PSK32;
    if (lower == "psk64" || lower == "qam32" || lower == "5") return M_PSK::PSK64;
    if (lower == "psk128" || lower == "qam64" || lower == "6") return M_PSK::PSK128;
    if (lower == "psk256" || lower == "qam128" || lower == "7") return M_PSK::PSK256;
    if (lower == "psk512" || lower == "qam256" || lower == "8") return M_PSK::PSK512;
    return M_PSK::QPSK;  // 默认值
}

M_PSK::BitOrder M_PSK_Block::ConvertStringToBitOrder(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));

    if (lower == "lsb first" || lower == "lsb_first" || lower == "lsb" || lower == "0") return M_PSK::LSB_first;
    if (lower == "msb first" || lower == "msb_first" || lower == "msb" || lower == "1") return M_PSK::MSB_first;

    return M_PSK::MSB_first;  // 默认值
}

void M_PSK_Block::SetDefaultParameters()
{
    m_modType = M_PSK::QPSK;
    m_bitOrder = M_PSK::MSB_first;
}

bool M_PSK_Block::ModelSetup()
{
    if (!m_psk->ValidateParameters())
        return false;

    m_psk->m_in.SetRate(static_cast<unsigned>(m_psk->m_modBits));

    m_psk->GenerateGrayCoding();
    m_psk->BuildConstellationTable();

    m_psk->m_setupModType = m_modType;

    return true;
}
