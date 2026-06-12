#include "DB_Block.h"
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
DB_Block::DB_Block(const std::string &name)
    :Block(name)
{

}

bool DB_Block::Setup()
{
    Block::Setup();
    return true;
}

bool DB_Block::Run()
{
    std::vector<double> inputData;
    inputData = ReadInputData<double>(GetInputPortName(0));

    const double x = inputData[0];
    double y;

    if (x <= 0.0)
    {
        y = m_Min;
    }
    else
    {
        double v;
        if (m_DbType == DB::POWER)
        {
            v = 10.0 * std::log10(x);
        }
        else
        {
            v = 20.0 * std::log10(x);
        }

        y = (v >= m_Min) ? v : m_Min;
    }

    std::vector<double> outputData;
    outputData.reserve(inputData.size());
    outputData.push_back(y);
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool DB_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_DB = std::make_unique<DB>();

    SetDefaultParamters();

    try { m_Min = std::stod(getParameter("NumRows").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'NumRows', using default value."); }
    try { m_DbType = ConvertStringToDbTypeEnum(getParameter("DbType").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'DbType', using default value."); }

    SetParameters();

    AddInputPort("input", m_DB->input, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_DB->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

void DB_Block::SetParameters()
{
    if(!m_DB) return;
    m_DB->Min = m_Min;
    m_DB->DbType = m_DbType;
}

DB::DbTypeEnum DB_Block::ConvertStringToDbTypeEnum(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "power" || lower == "0") {
        return DB::POWER;
    }
    if (lower == "amplitude" || lower == "1") {
        return DB::AMPLITUDE;
    }
    return DB::AMPLITUDE;
}

void DB_Block::SetDefaultParamters()
{
    m_Min = -100;
    m_DbType = DB::AMPLITUDE;
}
