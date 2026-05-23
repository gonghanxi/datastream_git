#include "Pack_M_Block.h"
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
Pack_M_Block::Pack_M_Block(const std::string& name)
    :Block(name)
{

}

bool Pack_M_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool Pack_M_Block::DataStreamRun()
{
    // 读取输入数据（double类型）
    std::string inputPortName = GetInputPortName(0);
    std::vector<double> inputData = ReadInputData<double>(inputPortName);

    // 检查输入数据数量是否足够
    size_t expectedSize = static_cast<size_t>(m_NumRows * m_NumCols);
    if (inputData.size() < expectedSize) {
        LOG_ERROR(QString("Insufficient input data. Expected: %1, Got: %2")
                  .arg(expectedSize).arg(inputData.size()).toStdString());
        return false;
    }

    // 创建输出数据容器
    std::vector<SystemVueModelBuilder::DoubleMatrix> outputData;
    outputData.reserve(1);  // 预分配空间

    // 创建输出矩阵（DoubleMatrix类型）
    SystemVueModelBuilder::DoubleMatrix outputMatrix;
    outputMatrix.Resize(m_NumRows, m_NumCols);

    // 根据Format填充矩阵
    // ColumnMajor: 按列优先填充（先填满第一列，再第二列...）
    // RowMajor: 按行优先填充（先填满第一行，再第二行...）
    for (int row = 0; row < m_NumRows; row++) {
        for (int col = 0; col < m_NumCols; col++) {
            int inputIndex;
            if (m_Format == Pack_M::ColumnMajor) {
                // 列优先：索引 = col * NumRows + row
                inputIndex = col * m_NumRows + row;
            } else {
                // 行优先：索引 = row * NumCols + col
                inputIndex = row * m_NumCols + col;
            }

            // 确保索引不越界
            if (inputIndex >= 0 && inputIndex < static_cast<int>(inputData.size())) {
                outputMatrix(row, col) = inputData[inputIndex];  // 直接赋值double
            } else {
                outputMatrix(row, col) = 0.0;
            }
        }
    }

    // 将矩阵添加到输出容器
    outputData.push_back(outputMatrix);

    // 写入输出数据
    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

bool Pack_M_Block::TimeDrivenRun()
{
    // 读取输入数据（double类型）
    std::string inputPortName = GetInputPortName(0);
    std::vector<double> inputData = ReadInputData<double>(inputPortName);

    size_t expectedSize = static_cast<size_t>(m_NumRows * m_NumCols);
    if (inputData.empty()) {
        return true;
    }
    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    if(m_inputBuffer.size() >= expectedSize) {
        // 创建输出数据容器
        // 创建输出矩阵（DoubleMatrix类型）
        SystemVueModelBuilder::DoubleMatrix outputMatrix;
        outputMatrix.Resize(m_NumRows, m_NumCols);

        // 根据Format填充矩阵
        // ColumnMajor: 按列优先填充（先填满第一列，再第二列...）
        // RowMajor: 按行优先填充（先填满第一行，再第二行...）
        for (int row = 0; row < m_NumRows; row++) {
            for (int col = 0; col < m_NumCols; col++) {
                int inputIndex;
                if (m_Format == Pack_M::ColumnMajor) {
                    // 列优先：索引 = col * NumRows + row
                    inputIndex = col * m_NumRows + row;
                } else {
                    // 行优先：索引 = row * NumCols + col
                    inputIndex = row * m_NumCols + col;
                }

                // 确保索引不越界
                if (inputIndex >= 0 && inputIndex < static_cast<int>(m_inputBuffer.size())) {
                    outputMatrix(row, col) = m_inputBuffer[inputIndex];  // 直接赋值double
                } else {
                    outputMatrix(row, col) = 0.0;
                }
            }
        }
        // 将矩阵添加到输出容器
        m_outputQueue.push(outputMatrix);
        //执行写入
        if (!m_outputQueue.empty()) {
            DoubleMatrix outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<DoubleMatrix>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[Pack_M_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue(0,0);
            m_inputBuffer.clear();
            return true;
        }
    }
    return true;
}

bool Pack_M_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool Pack_M_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_Pack_M = std::make_unique<Pack_M>();

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

    AddInputPort("input", m_Pack_M->input, static_cast<size_t>(m_NumRows * m_NumCols), Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_Pack_M->output, 1, Block::DataType::MATRIX_DOUBLE);

    return true;
}

void Pack_M_Block::SetParameters()
{
    if(!m_Pack_M) {
        return;
    }
    m_Pack_M->NumCols = m_NumCols;
    m_Pack_M->NumRows = m_NumRows;
    m_Pack_M->Format = m_Format;
}

Pack_M::SelectedFormat Pack_M_Block::ConvertStringToSelectedFormat(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if (lower == "columnmajor" || lower == "0") {
        return Pack_M::ColumnMajor;
    }
    if (lower == "rowmajor" || lower == "1") {
        return Pack_M::RowMajor;
    }
    return Pack_M::ColumnMajor;
}

void Pack_M_Block::SetDefaultParameters()
{
    m_NumRows = 1;
    m_NumCols = 1;
    m_Format = Pack_M::ColumnMajor;
}


