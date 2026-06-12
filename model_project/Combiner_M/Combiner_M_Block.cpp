#include "Combiner_M_Block.h"
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
Combiner_M_Block::Combiner_M_Block(const std::string &name)
    :Block(name)
{

}

bool Combiner_M_Block::Setup()
{
    if (NumRows < 1)
    {
        LOG_ERROR("NumRows must be >= 1.");
        return false;
    }

    if (NumCols < 1)
    {
        LOG_ERROR("NumCols must be >= 1.");
        return false;
    }
    Block::Setup();
    return true;
}

bool Combiner_M_Block::Run()
{
    auto inputData = ReadInputData<EnvelopeMatrix>(GetInputPortName(0));
    std::vector<EnvelopeMatrix> outputData(1);
    inRow = inputData[0].NumRows();
    inCol = inputData[0].NumColumns();

    double InsertionLossM = std::pow(10, InsertionLoss / 10);

    switch (Mode)
    {
    case Combiner_M::SubArray:

        if (inRow % NumRows)
        {
            LOG_ERROR("The input matrix row number inRow must be divisible by NumRows.");
            return false;
        }

        if (inCol % NumCols)
        {
            LOG_ERROR("The input matrix column number inCol must be divisible by NumCols.");
            return false;
        }

        outRow = inRow / NumRows;
        outCol = inCol / NumCols;

        outputData[0].Resize(outRow, outCol);
        outputData[0].Zero();
        for (int m = 0; m < inRow; m++)
        {
            for (int n = 0; n < inCol; n++)
            {
                outputData[0](m / NumRows, n / NumCols) += inputData[0](m, n) / std::sqrt(InsertionLossM * NumRows * NumCols);
            }
        }
        break;

    case Combiner_M::Custom:
        numMap = ElementMap.NumElements();

        if (numMap != inRow * inCol)
        {
            LOG_ERROR("The number of input signals does not match the size of the ElementMap array.");
            return false;
        }

        // 求出输出最大通道数
        maxChannel = 0;
        for (int i = 0; i < numMap; i++)
        {
            if (maxChannel < ElementMap(i))
            {
                maxChannel = ElementMap(i);
            }
        }

        channelCount.Resize(maxChannel, 1);
        channelCount.Zero();
        outputData[0].Resize(maxChannel, 1);
        outputData[0].Zero();

        // 进行通道合成
        for (int i = 0; i < numMap; i++)
        {
            if (ElementMap(i)) // 映射图中指向0的元素会被忽略
            {
                outputData[0](ElementMap(i) - 1) += inputData[0](i);
                channelCount(ElementMap(i) - 1)++; // 对应通道计数+1
            }
        }

        // 根据合成损耗对输出进行加权
        for (int i = 0; i < maxChannel; i++)
        {
            outputData[0](i) /= std::sqrt(InsertionLossM * channelCount(i));
        }

        break;

    case Combiner_M::FullArray:
        outputData[0].Resize(1, 1);
        outputData[0].Zero();
        for (int m = 0; m < inRow; m++)
        {
            for (int n = 0; n < inCol; n++)
            {
                outputData[0](0, 0) += inputData[0](m, n) / std::sqrt(InsertionLossM * inRow * inCol);
            }
        }
        break;

    default:
        break;
    }
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool Combiner_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Combiner = std::make_unique<Combiner_M>();
    SetDefaultParameters();
    try { Mode = ConvertStringToSelectedMode(getParameter("Mode").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'Mode', using default value."); }
    try { NumRows = std::stoi(getParameter("NumRows").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'NumRows', using default value."); }
    try { NumCols = std::stoi(getParameter("NumCols").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'NumCols', using default value."); }
    try { ElementMap = ParseStringToMatrix<int>(getParameter("ElementMap").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'ElementMap', using default value."); }
    try { InsertionLoss = std::stod(getParameter("InsertionLoss").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'InsertionLoss', using default value."); }
    SetParameters();
    AddInputPort("input", m_Combiner->input, 1, DataType::MATRIX_ENVELOPE);
    AddOutputPort("output", m_Combiner->output, 1, DataType::MATRIX_ENVELOPE);
    return true;
}

void Combiner_M_Block::SetParameters()
{
    if(!m_Combiner) return;
    m_Combiner->Mode = Mode;
    m_Combiner->NumRows = NumRows;
    m_Combiner->NumCols = NumCols;
    m_Combiner->ElementMap = ElementMap;
    m_Combiner->InsertionLoss = InsertionLoss;
}

void Combiner_M_Block::SetDefaultParameters()
{
    Mode = Combiner_M::FullArray;
    NumRows = 1;
    NumCols = 1;
    ElementMap.Resize(1,1);
    ElementMap.Zero();
    InsertionLoss = 0;
}

Combiner_M::SelectedMode Combiner_M_Block::ConvertStringToSelectedMode(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "subarray" || lower == "0") return Combiner_M::SubArray;
    if(lower == "custom" || lower == "1") return Combiner_M::Custom;
    if(lower == "fullarray" || lower == "2") return Combiner_M::FullArray;
    return Combiner_M::FullArray;
}
