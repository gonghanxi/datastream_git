#include "Quantizer_M_Block.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

// ============================================================================
// 构造函数
// ============================================================================

Quantizer_M_Block::Quantizer_M_Block(const std::string& name)
    : Block(name)
    , Thresholds(nullptr)
    , ThresholdsSize(0)
    , Levels(nullptr)
    , LevelsSize(0)
{
}

// ============================================================================
// Setup
// ============================================================================

bool Quantizer_M_Block::Setup()
{
    Block::Setup();
    if (!ModelSetup()) return false;
    return true;
}

// ============================================================================
// Run
// ============================================================================

bool Quantizer_M_Block::Run()
{
    auto inputData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(GetInputPortName(0));
    if (inputData.empty()) {
        return true;
    }

    const SystemVueModelBuilder::DoubleMatrix& inMat = inputData[0];

    const size_t rows = inMat.NumRows();
    const size_t cols = inMat.NumColumns();

    const int N = static_cast<int>(m_thresholds.size());

    if (N == 0 || static_cast<int>(m_levels.size()) != N + 1)
    {
        LOG_ERROR("Quantizer internal state is invalid.");
        return false;
    }

    SystemVueModelBuilder::DoubleMatrix outMat;
    SystemVueModelBuilder::IntMatrix idxMat;
    outMat.Resize(static_cast<int>(rows), static_cast<int>(cols));
    idxMat.Resize(static_cast<int>(rows), static_cast<int>(cols));

    for (size_t r = 0; r < rows; ++r)
    {
        for (size_t c = 0; c < cols; ++c)
        {
            const int k = QuantizeIndex(inMat(r, c));
            outMat(r, c) = m_levels[static_cast<size_t>(k)];
            idxMat(r, c) = k;
        }
    }

    std::vector<SystemVueModelBuilder::DoubleMatrix> outputData;
    outputData.push_back(outMat);
    WriteOutputData(GetOutputPortName(0), outputData);

    std::vector<SystemVueModelBuilder::IntMatrix> stepData;
    stepData.push_back(idxMat);
    WriteOutputData(GetOutputPortName(1), stepData);

    return true;
}

// ============================================================================
// Initialize
// ============================================================================

bool Quantizer_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Quantizer_M = std::make_unique<Quantizer_M>();

    SetDefaultParameters();

    // 读取 Thresholds 参数
    try {
        std::string thresholdsStr = getParameter("Thresholds").Value;
        parseArrayString(thresholdsStr, m_thresholdsData);
    } catch (...) {}

    // 读取 Levels 参数
    try {
        std::string levelsStr = getParameter("Levels").Value;
        parseArrayString(levelsStr, m_levelsData);
    } catch (...) {}

    SetParameters();

    AddInputPort("input", m_Quantizer_M->input, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_Quantizer_M->output, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("stepNumber", m_Quantizer_M->stepNumber, 1, Block::DataType::MATRIX_INT);

    return true;
}

// ============================================================================
// 参数设置
// ============================================================================

void Quantizer_M_Block::SetDefaultParameters()
{
    m_thresholdsData.clear();
    m_thresholdsData.push_back(0.0);
    Thresholds = m_thresholdsData.data();
    ThresholdsSize = 1;

    m_levelsData.clear();
    Levels = nullptr;
    LevelsSize = 0;
}

void Quantizer_M_Block::SetParameters()
{
    Thresholds = m_thresholdsData.data();
    ThresholdsSize = static_cast<int>(m_thresholdsData.size());
    Levels = m_levelsData.data();
    LevelsSize = static_cast<int>(m_levelsData.size());

    if (!m_Quantizer_M) return;
    m_Quantizer_M->Thresholds = Thresholds;
    m_Quantizer_M->ThresholdsSize = ThresholdsSize;
    m_Quantizer_M->Levels = Levels;
    m_Quantizer_M->LevelsSize = LevelsSize;
}

// ============================================================================
// ModelSetup
// ============================================================================

bool Quantizer_M_Block::ModelSetup()
{
    m_thresholds.clear();
    m_levels.clear();

    if (Thresholds == nullptr || ThresholdsSize <= 0)
    {
        m_thresholds.push_back(0.0);
    }
    else
    {
        m_thresholds.assign(Thresholds, Thresholds + ThresholdsSize);
    }

    const size_t N = m_thresholds.size();
    if (N == 0)
    {
        LOG_ERROR("Thresholds must contain at least one element.");
        return false;
    }

    for (size_t i = 1; i < N; ++i)
    {
        if (!(m_thresholds[i - 1] < m_thresholds[i]))
        {
            LOG_ERROR("Thresholds must be in strictly increasing order.");
            return false;
        }
    }

    if (Levels == nullptr || LevelsSize <= 0)
    {
        m_levels.resize(N + 1);
        for (size_t k = 0; k <= N; ++k)
        {
            m_levels[k] = static_cast<double>(k);
        }
    }
    else
    {
        m_levels.assign(Levels, Levels + LevelsSize);

        if (m_levels.size() != N + 1)
        {
            LOG_ERROR("Levels must have exactly N+1 elements.");
            return false;
        }
    }

    return true;
}

// ============================================================================
// QuantizeIndex
// ============================================================================

int Quantizer_M_Block::QuantizeIndex(double x) const
{
    const int N = static_cast<int>(m_thresholds.size());

    const auto it = std::lower_bound(m_thresholds.begin(), m_thresholds.end(), x);

    int k = static_cast<int>(it - m_thresholds.begin());
    if (k < 0) k = 0;
    if (k > N) k = N;
    return k;
}

// ============================================================================
// parseArrayString
// ============================================================================

bool Quantizer_M_Block::parseArrayString(const std::string& arrayStr, std::vector<double>& outArray)
{
    outArray.clear();

    std::string str = arrayStr;

    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return false;
    size_t end = str.find_last_not_of(" \t\n\r");
    str = str.substr(start, end - start + 1);

    if (str.empty() || str.front() != '[' || str.back() != ']')
    {
        return false;
    }

    std::string content = str.substr(1, str.length() - 2);

    start = content.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
    {
        return true;
    }
    end = content.find_last_not_of(" \t\n\r");
    content = content.substr(start, end - start + 1);

    std::stringstream ss(content);
    std::string item;

    while (std::getline(ss, item, ','))
    {
        start = item.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue;
        end = item.find_last_not_of(" \t\n\r");
        item = item.substr(start, end - start + 1);

        if (!item.empty())
        {
            try
            {
                double value = std::stod(item);
                outArray.push_back(value);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("Failed to parse array element: " + std::string(e.what()));
                return false;
            }
        }
    }

    return true;
}
