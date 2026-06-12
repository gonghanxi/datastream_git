#include "Splitter_M_Block.h"
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
Splitter_M_Block::Splitter_M_Block(const std::string &name)
    :Block(name)
{

}
bool Splitter_M_Block::Setup()
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

bool Splitter_M_Block::Run()
{
    auto inputData = ReadInputData<EnvelopeMatrix>(GetInputPortName(0));
    std::vector<EnvelopeMatrix> outputData(1);
    inRow = inputData[0].NumRows();
    inCol = inputData[0].NumColumns();

    double InsertionLossM = std::pow(10, InsertionLoss / 10);

    switch (Mode)
    {
    case Splitter_M::SubArray:
        outRow = inRow * NumRows;
        outCol = inCol * NumCols;

        outputData[0].Resize(outRow, outCol);
        outputData[0].Zero();
        for (int m = 0; m < outRow; m++)
        {
            for (int n = 0; n < outCol; n++)
            {
                outputData[0](m, n) = inputData[0](m / NumRows, n / NumCols) / std::sqrt(InsertionLossM * NumRows * NumCols);
            }
        }
        break;

    case Splitter_M::Custom:
        numMap = ElementMap.NumElements();
        channelCount.Resize(inputData[0].NumElements(), 1);
        channelCount.Zero();

        // 检查映射图中元素是否有效，并对分配损耗序列进行计数
        for (int i = 0; i < numMap; i++)
        {
            if (ElementMap(i) < 1 || ElementMap(i) > inputData[0].NumElements())
            {
                LOG_ERROR("The element in the ElementMap array must be an integer between 1 and the number of splitters.");
                return false;
            }

            channelCount(ElementMap(i) - 1)++; // 对应通道计数+1
        }

        outputData[0].Resize(numMap, 1);
        outputData[0].Zero();

        // 进行通道分配
        for (int i = 0; i < numMap; i++)
        {
            outputData[0](i) = inputData[0](ElementMap(i) - 1) / std::sqrt(InsertionLossM * channelCount(ElementMap(i) - 1));
        }

        break;

    default:
        break;
    }
    WriteOutputData(GetOutputPortName(0), outputData);
    return true;
}

bool Splitter_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Splitter = std::make_unique<Splitter_M>();
    SetDefaultParameters();
    try { Mode = ConvertStringToSelectedMode(getParameter("Mode").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'Mode', using default value."); }
    try { NumRows = std::stoi(getParameter("NumRows").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'NumRows', using default value."); }
    try { NumCols = std::stoi(getParameter("NumCols").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'NumCols', using default value."); }
    try { ElementMap = ParseStringToMatrix<int>(getParameter("ElementMap").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'ElementMap', using default value."); }
    try { InsertionLoss = std::stod(getParameter("InsertionLoss").Value); } catch(...) { LOG_WARN("Failed to parse parameter 'InsertionLoss', using default value."); }
    SetParameters();
    AddInputPort("input", m_Splitter->input, 1, DataType::MATRIX_ENVELOPE);
    AddOutputPort("output", m_Splitter->output, 1, DataType::MATRIX_ENVELOPE);
    return true;
}

void Splitter_M_Block::SetParameters()
{
    if(!m_Splitter) return;
    m_Splitter->Mode = Mode;
    m_Splitter->NumRows = NumRows;
    m_Splitter->NumCols = NumCols;
    m_Splitter->ElementMap = ElementMap;
    m_Splitter->InsertionLoss = InsertionLoss;
}

void Splitter_M_Block::SetDefaultParameters()
{
    Mode = Splitter_M::SubArray;
    NumRows = 1;
    NumCols = 1;
    ElementMap.Resize(1,1);
    ElementMap.Zero();
    InsertionLoss = 0;
}

Splitter_M::SelectedMode Splitter_M_Block::ConvertStringToSelectedMode(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "subarray" || lower == "0") return Splitter_M::SubArray;
    if(lower == "custom" || lower == "1") return Splitter_M::Custom;
    return Splitter_M::SubArray;
}
