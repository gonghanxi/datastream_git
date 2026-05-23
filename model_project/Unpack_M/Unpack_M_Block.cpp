#include "Unpack_M_Block.h"
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
Unpack_M_Block::Unpack_M_Block(const std::string &name)
    :Block(name)
{

}

bool Unpack_M_Block::Setup()
{
    Block::Setup();
    return true;
}

bool Unpack_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Unpack_M = std::make_unique<Unpack_M>();

    SetDefaultParameters();

    try { m_NumRows = std::stoi(getParameter("NumRows").Value); } catch (...) { }
    try { m_Format = ConvertStringToSelectedFormat(getParameter("SelectedFormat").Value); } catch (...) { }
    try { m_NumCols = std::stoi(getParameter("NumCols").Value); } catch (...) { }

    SetParameters();

    if (m_NumRows < 1 || m_NumCols < 1)
    {
        LOG_ERROR("NumRows and NumCols must be >= 1.");
        return false;
    }

    AddInputPort("input", m_Unpack_M->input, 1, Block::DataType::MATRIX_DOUBLE);
    AddOutputPort("output", m_Unpack_M->output, static_cast<size_t>(m_NumRows * m_NumCols), Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    return true;
}

bool Unpack_M_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    // 获取输入输出端口名称
    std::string inputPortName = GetInputPortName(0);
    std::string outputPortName = GetOutputPortName(0);

    // 读取输入矩阵数据（DoubleMatrix类型）
    auto inputData = ReadInputData<SystemVueModelBuilder::DoubleMatrix>(inputPortName);
    if (inputData.empty()) {
        return true;  // 没有数据，等待下次调用
    }

    // 获取输入矩阵
    const SystemVueModelBuilder::DoubleMatrix& inputMatrix = inputData[0];

    // 获取输入矩阵的实际尺寸
    int actualRows = static_cast<int>(inputMatrix.NumRows());
    int actualCols = static_cast<int>(inputMatrix.NumColumns());

    // 计算输出数据总数
    int outputSize = m_NumRows * m_NumCols;

    // 创建输出数据容器（vector，因为WriteOutputData需要vector）
    std::vector<double> outputData;
    outputData.reserve(outputSize);

    // 根据Format将矩阵元素解包到输出向量
    for (int i = 0; i < outputSize; ++i) {
        // 计算矩阵中的行列索引
        int row, col;
        if (m_Format == Unpack_M::ColumnMajor) {
            // 列优先：索引 = col * NumRows + row
            row = i % m_NumRows;
            col = i / m_NumRows;
        } else {
            // 行优先：索引 = row * NumCols + col
            row = i / m_NumCols;
            col = i % m_NumCols;
        }

        // 如果行列索引在输入矩阵范围内，取对应值，否则取0
        double value = (row < actualRows && col < actualCols) ?
                       inputMatrix(row, col) : 0.0;

        outputData.push_back(value);
    }
    qDebug() << "Unpack_M_Block::Run - outputData size: " << outputData.size();

    // 写入输出数据
    WriteOutputData(outputPortName, outputData);

    return true;
}

void Unpack_M_Block::SetParameters()
{
    if(!m_Unpack_M) return;
    m_Unpack_M->NumCols = m_NumCols;
    m_Unpack_M->NumRows = m_NumRows;
    m_Unpack_M->Format = m_Format;
}

Unpack_M::SelectedFormat Unpack_M_Block::ConvertStringToSelectedFormat(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "columnmajor" || lower == "0") {
        return Unpack_M::ColumnMajor;
    }
    if (lower == "rowmajor" || lower == "1") {
        return Unpack_M::RowMajor;
    }
    return Unpack_M::ColumnMajor;
}

void Unpack_M_Block::SetDefaultParameters()
{
    m_NumRows = 1;
    m_NumCols = 1;
    m_Format = Unpack_M::ColumnMajor;
}
