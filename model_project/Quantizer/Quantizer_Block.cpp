#include "Quantizer_Block.h"

Quantizer_Block::Quantizer_Block(const std::string &name)
    :Block(name)
{

}

bool Quantizer_Block::Setup()
{
    Block::Setup();
    if(!ModelSetup()) return false;
    return true;
}

bool Quantizer_Block::Run()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);
    std::string stepPort = GetOutputPortName(1);

    std::vector<double> inputData = ReadInputData<double>(inputPort);
    std::vector<double> outputData(1);
    std::vector<int> stepNumberData(1);

    const double x = inputData[0U];

    const unsigned N =
            static_cast<unsigned>(m_thresholds.size());

    if (N == 0U || m_levels.size() != N + 1U)
    {
        LOG_ERROR("Quantizer internal state is invalid.");
        return false;
    }

    const auto it =
            std::lower_bound(m_thresholds.begin(),
                             m_thresholds.end(), x);

    unsigned k = 0U;
    if (it == m_thresholds.end())
    {
        k = N;
    }
    else
    {
        k = static_cast<unsigned>(it - m_thresholds.begin()); // 0..N-1
    }

    outputData[0U] = m_levels[k];
    stepNumberData[0U] = static_cast<int>(k);
    WriteOutputData(outputPort, outputData);
    WriteOutputData(stepPort, stepNumberData);

    return true;
}

bool Quantizer_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Quantizer = std::make_unique<Quantizer>();
    SetDefaultParameters();

    // 获取 Thresholds 参数并解析
    std::string thresholdsStr = getParameter("Thresholds").Value;
    parseArrayString(thresholdsStr, m_thresholdsData);
    // 获取 Levels 参数并解析
    std::string levelsStr = getParameter("Levels").Value;
    parseArrayString(levelsStr, m_levelsData);


    SetParameters();
    AddInputPort("input", m_Quantizer->input, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_Quantizer->output, 1, DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("stepNumber", m_Quantizer->stepNumber, 1, DataType::CIRCULAR_BUFFER_INT);
    return true;
}

void Quantizer_Block::SetParameters()
{
    Thresholds = m_thresholdsData.data();
    ThresholdsSize = static_cast<int>(m_thresholdsData.size());
    Levels = m_levelsData.data();
    LevelsSize = static_cast<int>(m_levelsData.size());

    if(!m_Quantizer) return;
    m_Quantizer->Thresholds = Thresholds;
    m_Quantizer->ThresholdsSize = ThresholdsSize;
    m_Quantizer->Levels = Levels;
    m_Quantizer->LevelsSize = LevelsSize;
}

bool Quantizer_Block::ModelSetup()
{
    m_thresholds.clear();
    m_levels.clear();
    if (Thresholds == nullptr || ThresholdsSize == 0U)
    {
        m_thresholds.push_back(0.0);
    }
    else
    {
        m_thresholds.assign(Thresholds,
                            Thresholds + ThresholdsSize);
    }

    const unsigned N = static_cast<unsigned>(m_thresholds.size());
    if (N == 0U)
    {
        LOG_ERROR("Thresholds must contain at least one element.");
        return false;
    }

    for (unsigned i = 1; i < N; ++i)
    {
        if (!(m_thresholds[i - 1] < m_thresholds[i]))
        {
            LOG_ERROR("Thresholds must be in strictly increasing order.");
            return false;
        }
    }

    if (Levels == nullptr || LevelsSize == 0U)
    {
        m_levels.resize(N + 1U);
        for (unsigned k = 0; k <= N; ++k)
        {
            m_levels[k] = static_cast<double>(k);
        }
    }
    else
    {
        m_levels.assign(Levels, Levels + LevelsSize);

        if (m_levels.size() != N + 1U)
        {
            LOG_ERROR("Levels must have exactly N+1 elements.");
            return false;
        }
    }
    return true;
}

void Quantizer_Block::SetDefaultParameters()
{
    m_thresholdsData.clear();
    m_thresholdsData.push_back(0.0);
    Thresholds = m_thresholdsData.data();
    ThresholdsSize = 1;

    // Levels 默认值为空
    m_levelsData.clear();
    Levels = nullptr;
    LevelsSize = 0;
}

bool Quantizer_Block::parseArrayString(const std::string &arrayStr, std::vector<double> &outArray)
{
    outArray.clear();

    std::string str = arrayStr;
    // 去除首尾空格
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return false;
    size_t end = str.find_last_not_of(" \t\n\r");
    str = str.substr(start, end - start + 1);

    // 检查是否是数组格式
    if (str.empty() || str.front() != '[' || str.back() != ']') {
        return false;
    }

    // 去除外层括号
    std::string content = str.substr(1, str.length() - 2);

    // 去除首尾空格
    start = content.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        // 空数组
        return true;
    }
    end = content.find_last_not_of(" \t\n\r");
    content = content.substr(start, end - start + 1);

    // 按逗号分割
    std::stringstream ss(content);
    std::string item;

    while (std::getline(ss, item, ',')) {
        // 去除空格
        start = item.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue;
        end = item.find_last_not_of(" \t\n\r");
        item = item.substr(start, end - start + 1);

        if (!item.empty()) {
            try {
                double value = std::stod(item);
                outArray.push_back(value);
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to parse array element: ", item, " - ", e.what());
                return false;
            }
        }
    }

    return true;
}
