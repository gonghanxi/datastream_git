#include "LookUpTable_Block.h"
#include "DataTypesAndParsers.h"

#include <algorithm>
#include <sstream>

namespace {
std::vector<double> ParseVectorDouble(const std::string& value)
{
    std::vector<double> result;

    try {
        auto mat = DataTypesAndParsers::ParseStringToMatrixDouble(value);
        result.reserve(mat.NumElements());
        for (size_t i = 0; i < mat.NumElements(); ++i) {
            result.push_back(mat(i));
        }
        return result;
    } catch (...) {
        // Fall through to basic parse
    }

    std::string s = value;
    s.erase(std::remove(s.begin(), s.end(), '['), s.end());
    s.erase(std::remove(s.begin(), s.end(), ']'), s.end());
    std::replace(s.begin(), s.end(), ',', ' ');

    std::stringstream ss(s);
    std::string token;
    while (ss >> token) {
        try {
            result.push_back(std::stod(token));
        } catch (...) {
        }
    }

    return result;
}
} // namespace

// ============================================================================
// 构造函数
// ============================================================================

LookUpTable_Block::LookUpTable_Block(const std::string& name)
    : Block(name)
{
}

// ============================================================================
// Setup
// ============================================================================

bool LookUpTable_Block::Setup()
{
    Block::Setup();
    while (!m_outputQueue.empty()) m_outputQueue.pop();
    m_inputBuffer.clear();
    return true;
}

// ============================================================================
// Run — 分发
// ============================================================================

bool LookUpTable_Block::Run()
{
    return DataStreamRun();
}

// ============================================================================
// DataStreamRun — 批量模式
// ============================================================================

bool LookUpTable_Block::DataStreamRun()
{
    std::string inputPort  = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<int>(inputPort);
    if (inputData.empty()) return true;

    const int idx = inputData[0];

    if (idx < 0 || static_cast<size_t>(idx) >= m_values.size())
    {
        LOG_ERROR("The \"input\" value (i.e. index for the Look Up Table) "
            "must be >= 0 and < the number of data in the \"Values\" table.");
        return false;
    }

    std::vector<double> outputData;
    outputData.push_back(m_values[static_cast<size_t>(idx)]);
    WriteOutputData(outputPort, outputData);

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool LookUpTable_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_LookUpTable = std::make_unique<LookUpTable>();

    // 直接从参数字符串解析 Values 表，不依赖算法对象
    m_values = ParseVectorDouble(getParameter("Values").Value);

    AddInputPort("input",  m_LookUpTable->input,  1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_LookUpTable->output, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}
