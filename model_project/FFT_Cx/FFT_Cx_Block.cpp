#include "FFT_Cx_Block.h"
#include <algorithm>

FFT_Cx_Block::FFT_Cx_Block(const std::string& name)
	: Block(name)
{
}

void FFT_Cx_Block::SetDefaultParamters()
{
	m_fftSize = 256;
	m_size = 256;
	m_direction = FFT_Cx::FFT;
	m_freqSequence = FFT_Cx::O_pos_neg;
}

void FFT_Cx_Block::SetParameters(int fftSize, int size, FFT_Cx::SelectedDirection dir, FFT_Cx::SelectedFreqSequence seq)
{
	m_fftSize = fftSize;
	m_size = size;
	m_direction = dir;
	m_freqSequence = seq;
	if (m_fftCx) {
		m_fftCx->FFTSize = fftSize;
		m_fftCx->Size = size;
		m_fftCx->Direction = dir;
		m_fftCx->FreqSequence = seq;
    }
}

bool FFT_Cx_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return false;
    }

    std::vector<std::complex<double>> outputData;
    outputData.reserve(static_cast<size_t>(m_fftSize));

    // Prepare sequence
    SystemVueModelBuilder::Matrix< std::complex<double> > fullSeq(1, m_fftSize);
    for (int i = 0; i < m_fftSize; i++) {
        if (i < m_size && i < static_cast<int>(inputData.size())) {
            fullSeq(i) = inputData[static_cast<size_t>(i)];
        } else {
            fullSeq(i) = 0.0;
        }
    }

    if (m_direction == FFT_Cx::FFT) {
        m_fftCx->fft(fullSeq, m_fftSize, 1);
        fullSeq *= m_fftSize;

        if (m_freqSequence == FFT_Cx::O_pos_neg) {
            for (int i = 0; i < m_fftSize; i++) {
                outputData.push_back(fullSeq(i));
            }
        } else if (m_freqSequence == FFT_Cx::neg_O_pos) {
            for (int i = 0; i < m_fftSize; i++) {
                int n = i - m_fftSize / 2;
                outputData.push_back(fullSeq(n >= 0 ? n : n + m_fftSize));
            }
        }
    } else if (m_direction == FFT_Cx::IFFT) {
        SystemVueModelBuilder::Matrix< std::complex<double> > shiftSeq(1, m_fftSize);

        if (m_freqSequence == FFT_Cx::O_pos_neg) {
            for (int i = 0; i < m_fftSize; i++) {
                shiftSeq(i) = fullSeq(i);
            }
        } else if (m_freqSequence == FFT_Cx::neg_O_pos) {
            for (int i = 0; i < m_fftSize; i++) {
                int n = i + m_fftSize / 2;
                shiftSeq(i) = fullSeq(n < m_fftSize ? n : n - m_fftSize);
            }
        }

        m_fftCx->fft(shiftSeq, m_fftSize, -1);

        for (int i = 0; i < m_fftSize; i++) {
            outputData.push_back(shiftSeq(m_fftSize - i<m_fftSize?m_fftSize-i:0));
        }
    }

    WriteOutputData(outputPort, outputData);

    return true;
}

bool FFT_Cx_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);

    auto inputData = ReadInputData<std::complex<double>>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    for(const auto& val : inputData) m_inputBuffer.push_back(val);
    if(m_inputBuffer.size() >= static_cast<size_t>(m_size)) {
        std::vector<std::complex<double>> outputData;
        outputData.reserve(static_cast<size_t>(m_fftSize));

        // Prepare sequence
        SystemVueModelBuilder::Matrix< std::complex<double> > fullSeq(1, m_fftSize);
        for (int i = 0; i < m_fftSize; i++) {
            if (i < m_size && i < static_cast<int>(inputData.size())) {
                fullSeq(i) = m_inputBuffer[static_cast<size_t>(i)];
            } else {
                fullSeq(i) = 0.0;
            }
        }

        if (m_direction == FFT_Cx::FFT) {
            m_fftCx->fft(fullSeq, m_fftSize, 1);
            fullSeq *= m_fftSize;

            if (m_freqSequence == FFT_Cx::O_pos_neg) {
                for (int i = 0; i < m_fftSize; i++) {
                    outputData.push_back(fullSeq(i));
                }
            } else if (m_freqSequence == FFT_Cx::neg_O_pos) {
                for (int i = 0; i < m_fftSize; i++) {
                    int n = i - m_fftSize / 2;
                    outputData.push_back(fullSeq(n >= 0 ? n : n + m_fftSize));
                }
            }
        } else if (m_direction == FFT_Cx::IFFT) {
            SystemVueModelBuilder::Matrix< std::complex<double> > shiftSeq(1, m_fftSize);

            if (m_freqSequence == FFT_Cx::O_pos_neg) {
                for (int i = 0; i < m_fftSize; i++) {
                    shiftSeq(i) = fullSeq(i);
                }
            } else if (m_freqSequence == FFT_Cx::neg_O_pos) {
                for (int i = 0; i < m_fftSize; i++) {
                    int n = i + m_fftSize / 2;
                    shiftSeq(i) = fullSeq(n < m_fftSize ? n : n - m_fftSize);
                }
            }

            m_fftCx->fft(shiftSeq, m_fftSize, -1);

            for (int i = 0; i < m_fftSize; i++) {
                outputData.push_back(shiftSeq(m_fftSize - i<m_fftSize?m_fftSize-i:0));
            }
        }
        for (const auto& val : outputData)
        {
            m_outputQueue.push(val);
        }
        if (!m_outputQueue.empty())
        {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(outputPort, std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;
            m_inputBuffer.clear();

            qDebug() << "[DeMux_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue.real() << "," << outputValue.imag();
        }
    }
    return true;
}

bool FFT_Cx_Block::Setup()
{
    Block::Setup();
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    return true;
}

bool FFT_Cx_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool FFT_Cx_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_fftCx = std::make_unique<FFT_Cx>();

	SetDefaultParamters();

	try { m_fftSize = std::stoi(getParameter("FFTSize").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FFTSize', using default value."); }
	try { m_size = std::stoi(getParameter("Size").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Size', using default value."); }
	try { m_direction = ConvertStringToDirection(getParameter("Direction").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Direction', using default value."); }
	try { m_freqSequence = ConvertStringToFreqSequence(getParameter("FreqSequence").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FreqSequence', using default value."); }

	SetParameters(m_fftSize, m_size, m_direction, m_freqSequence);

    if (!m_fftCx->Setup())
    {
        return false;
    }

    AddInputPort("input", m_fftCx->input, m_size, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddOutputPort("output", m_fftCx->output, m_fftSize, Block::DataType::CIRCULAR_BUFFER_DCOMPLEX);

	return true;
}

FFT_Cx::SelectedDirection FFT_Cx_Block::ConvertStringToDirection(const std::string& value)
{
	std::string trimmedValue;
	trimmedValue.reserve(value.size());
	for (char c : value) {
		if (!std::isspace(static_cast<unsigned char>(c))) {
			trimmedValue.push_back(c);
		}
	}
	std::string lowerValue = trimmedValue;
	std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
		[](unsigned char c) { return std::tolower(c); });

	if (lowerValue == "fft" || lowerValue == "0") {
		return FFT_Cx::FFT;
	} else if (lowerValue == "ifft" || lowerValue == "1") {
		return FFT_Cx::IFFT;
	}
	return FFT_Cx::FFT;
}

FFT_Cx::SelectedFreqSequence FFT_Cx_Block::ConvertStringToFreqSequence(const std::string& value)
{
	std::string trimmedValue;
	trimmedValue.reserve(value.size());
	for (char c : value) {
		if (!std::isspace(static_cast<unsigned char>(c))) {
			trimmedValue.push_back(c);
		}
	}
	std::string lowerValue = trimmedValue;
	std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(),
		[](unsigned char c) { return std::tolower(c); });

	if (lowerValue == "o_pos_neg" || lowerValue == "0posneg" || lowerValue == "0") {
		return FFT_Cx::O_pos_neg;
	} else if (lowerValue == "neg_o_pos" || lowerValue == "neg0pos" || lowerValue == "1") {
		return FFT_Cx::neg_O_pos;
	}
	return FFT_Cx::O_pos_neg;
}



