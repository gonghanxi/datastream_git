#include "ChopInt_Block.h"
#include <algorithm>
#include <cctype>
#include <vector>

namespace {
std::string NormalizeEnumString(const std::string& value)
{
    std::string out;
    out.reserve(value.size());
    for (unsigned char c : value) {
        if (std::isalnum(c) || c == '_') {
            out.push_back(static_cast<char>(std::tolower(c)));
        }
    }
    return out;
}
}

ChopInt_Block::ChopInt_Block(const std::string& name)
    : Block(name)
{
}

void ChopInt_Block::SetDefaultParamters()
{
    m_nRead = 128;
    m_nWrite = 64;
    m_offset = 0;
    m_usePastInputs = QUERY_YES;

    m_iReadFrom = 0;
    m_iReadNum = 0;
    m_iReadBufSize = 0;
    m_iWriteTo = 0;
    m_iWriteNum = 0;
    m_iWriteBufSize = 0;
    m_iZeroPadFrom = 0;
    m_iZeroPadNum = 0;
    m_history.clear();
}

void ChopInt_Block::SetParameters()
{
    if (!m_chop) {
        return;
    }

    m_chop->nRead = m_nRead;
    m_chop->nWrite = m_nWrite;
    m_chop->Offset = m_offset;
    m_chop->UsePastInputs = m_usePastInputs;
}

bool ChopInt_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool ChopInt_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool ChopInt_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_chop = std::make_unique<SystemVueModelBuilder::ChopInt>();

    AddInputPort("input", m_chop->input, 1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("output", m_chop->output, 1, Block::DataType::CIRCULAR_BUFFER_INT);

    SetDefaultParamters();

    try { m_nRead = std::stoi(getParameter("nRead").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'nRead', using default value."); }
    try { m_nWrite = std::stoi(getParameter("nWrite").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'nWrite', using default value."); }
    try { m_offset = std::stoi(getParameter("Offset").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Offset', using default value."); }
    try { m_usePastInputs = ConvertStringToQueryEnum(getParameter("UsePastInputs").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'UsePastInputs', using default value."); }

    if (m_nRead <= 0 || m_nWrite <= 0) {
        return false;
    }

    m_iReadBufSize = static_cast<std::size_t>(m_nRead);
    if ((m_usePastInputs == QUERY_YES) && (m_offset > 0)) {
        m_iReadBufSize += static_cast<std::size_t>(m_offset);
    }

    auto* inputPort = GetInputPort(GetInputPortName(0));
    auto* outputPort = GetOutputPort(GetOutputPortName(0));

    if (inputPort) {
        inputPort->SetReadSize(static_cast<size_t>(m_nRead));
    }
    if (outputPort) {
        outputPort->SetWriteSize(static_cast<size_t>(m_nWrite));
    }

    m_iWriteBufSize = static_cast<std::size_t>(m_nWrite);

    InitializeRanges();

    SetParameters();

    if (m_chop) {
        m_chop->output.SetRate(static_cast<unsigned>(m_nWrite));
    }

    m_history.clear();
    if ((m_usePastInputs == QUERY_YES) && (m_offset > 0)) {
        m_history.resize(m_iReadBufSize, 0);
    }

    return true;
}

QueryEnum ChopInt_Block::ConvertStringToQueryEnum(const std::string& value)
{
    const std::string key = NormalizeEnumString(value);
    if (key == "query_no" || key == "no" || key == "0") {
        return QUERY_NO;
    }
    if (key == "query_yes" || key == "yes" || key == "1") {
        return QUERY_YES;
    }
    return QUERY_YES;
}

bool ChopInt_Block::InitializeRanges()
{
    if ((m_usePastInputs == QUERY_YES) && (m_offset > 0)) {
        m_iReadFrom = 0;
        m_iWriteTo = 0;
    } else if (m_offset > 0) {
        m_iReadFrom = 0;
        m_iWriteTo = static_cast<std::size_t>(m_offset);
    } else {
        m_iReadFrom = static_cast<std::size_t>(-m_offset);
        m_iWriteTo = 0;
    }

    if (m_iWriteTo >= m_iWriteBufSize) {
        m_iWriteNum = 0;
        m_iWriteTo = m_iWriteBufSize;
        m_iZeroPadNum = 0;
    } else {
        std::size_t availableFromInput = (m_iReadFrom < m_iReadBufSize) ? (m_iReadBufSize - m_iReadFrom) : 0;
        m_iWriteNum = (std::min)(m_iWriteBufSize - m_iWriteTo, availableFromInput);
        m_iZeroPadFrom = m_iWriteNum + m_iWriteTo;
        if (m_iZeroPadFrom < m_iWriteBufSize) {
            m_iZeroPadNum = m_iWriteBufSize - m_iZeroPadFrom;
        } else {
            m_iZeroPadNum = 0;
        }
    }

    return true;
}

bool ChopInt_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<int>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    if ((m_usePastInputs == QUERY_YES) && (m_offset > 0)) {
        for (int v : inputData) {
            m_history.push_back(v);
        }
        while (m_history.size() > m_iReadBufSize) {
            m_history.pop_front();
        }
        if (m_history.size() < m_iReadBufSize) {
            const size_t pad = m_iReadBufSize - m_history.size();
            m_history.insert(m_history.begin(), pad, 0);
        }
    } else {
        m_history.assign(inputData.begin(), inputData.end());
        if (m_history.size() < m_iReadBufSize) {
            m_history.resize(m_iReadBufSize, 0);
        }
    }

    std::vector<int> outputData(m_nWrite, 0);

    if (m_iWriteNum > 0) {
        const size_t inStart = m_iReadFrom;
        const size_t inEnd = std::min(inStart + m_iWriteNum, m_history.size());
        const size_t outStart = m_iWriteTo;

        if (inStart < m_history.size() && outStart < outputData.size()) {
            const size_t copyCount = std::min(inEnd - inStart, outputData.size() - outStart);
            std::copy_n(m_history.begin() + inStart, copyCount, outputData.begin() + outStart);
        }
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

bool ChopInt_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<int>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    for(size_t i = 0; i < inputData.size(); i++) {
        m_inputBuffer.push_back(inputData[i]);
    }

    if(m_inputBuffer.size() >= static_cast<size_t>(m_nRead)) {
        if ((m_usePastInputs == QUERY_YES) && (m_offset > 0)) {
            for (int v : m_inputBuffer) {
                m_history.push_back(v);
            }
            while (m_history.size() > m_iReadBufSize) {
                m_history.pop_front();
            }
            if (m_history.size() < m_iReadBufSize) {
                const size_t pad = m_iReadBufSize - m_history.size();
                m_history.insert(m_history.begin(), pad, 0.0);
            }
        } else {
            m_history.assign(m_inputBuffer.begin(), m_inputBuffer.end());
            if (m_history.size() < m_iReadBufSize) {
                m_history.resize(m_iReadBufSize, 0.0);
            }
        }
        std::vector<int> outputData(m_nWrite);

        if (m_iWriteNum > 0) {
            const size_t inStart = m_iReadFrom;
            const size_t inEnd = std::min(inStart + m_iWriteNum, m_history.size());
            const size_t outStart = m_iWriteTo;

            if (inStart < m_history.size() && outStart < outputData.size()) {
                const size_t copyCount = std::min(inEnd - inStart, outputData.size() - outStart);
                std::copy_n(m_history.begin() + inStart, copyCount, outputData.begin() + outStart);
            }
        }
        for (const auto& val : outputData)
            m_outputQueue.push(val);
        m_inputBuffer.clear();
        if (!m_outputQueue.empty())
        {
            int outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<int>{outputValue});
            m_lastOutput = outputValue;
        }
    }
    return true;
}



