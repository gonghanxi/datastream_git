#include "RADAR_CFAR_Block.h"

#include <algorithm>
#include <cctype>
#include <cmath>

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

RADAR_CFAR_Block::RADAR_CFAR_Block(const std::string& name)
	: Block(name)
	, m_cfarType(RADAR_CFAR::CA)
	, m_cellSize(1000)
	, m_referenceCell(32)
	, m_guardCell(4)
	, m_kOrder(24)
	, m_thresholdScaleFactor(14.0)
	, m_detectorType(RADAR_CFAR::Square)
	, m_pf(1e-4)
	, m_alpha(1.0)
	, m_beta(1.0)
	, m_thresholdFactor(0.0)
{
}

void RADAR_CFAR_Block::SetDefaultParamters()
{
	m_cfarType = RADAR_CFAR::CA;
	m_cellSize = 1000;
	m_referenceCell = 32;
	m_guardCell = 4;
	m_kOrder = 24;
	m_thresholdScaleFactor = 14.0;
	m_detectorType = RADAR_CFAR::Square;
	m_pf = 1e-4;
	m_alpha = 1.0;
	m_beta = 1.0;
	m_thresholdFactor = 0.0;
}

void RADAR_CFAR_Block::SetParameters()
{
	if (!m_radarCfar) {
		return;
	}

	m_radarCfar->CFARType = m_cfarType;
	m_radarCfar->CellSize = m_cellSize;
	m_radarCfar->ReferenceCell = m_referenceCell;
	m_radarCfar->GuardCell = m_guardCell;
	m_radarCfar->kOrder = m_kOrder;
	m_radarCfar->ThresholdScaleFactor = m_thresholdScaleFactor;
	m_radarCfar->DetectorType = m_detectorType;
	m_radarCfar->Pf = m_pf;
	m_radarCfar->Alpha = m_alpha;
	m_radarCfar->Beta = m_beta;
	m_radarCfar->ThresholdFactor = m_thresholdFactor;
}

bool RADAR_CFAR_Block::ValidateParameters()
{
	if (m_cellSize <= 0) {
        LOG_ERROR("Port rate (CellSize) must be greater than 0.");
		return false;
	}

	if (m_kOrder <= 0 || m_kOrder > 2 * m_referenceCell) {
        LOG_ERROR("kOrder must be greater than 0 and smaller than 2 * ReferenceCell");
		return false;
	}

	return true;
}

void RADAR_CFAR_Block::UpdateThresholdFactor()
{
	switch (m_cfarType)
	{
	case RADAR_CFAR::CA:
		m_thresholdFactor = 2 * m_referenceCell * (std::pow(m_pf, -1.0 / (2 * m_referenceCell)) - 1);
		break;
	case RADAR_CFAR::SOCA:
		m_thresholdFactor = 2 * m_referenceCell * (std::pow(m_pf, -1.0 / (2 * m_referenceCell)) - 1);
		break;
	case RADAR_CFAR::GOCA:
		m_thresholdFactor = 2 * m_referenceCell * (std::pow(m_pf, -1.0 / (2 * m_referenceCell)) - 1);
		break;
	case RADAR_CFAR::OS:
		m_thresholdFactor = m_thresholdScaleFactor;
		break;
	case RADAR_CFAR::ClutterMap:
		break;
	default:
		break;
	}
}

bool RADAR_CFAR_Block::Setup()
{
	Block::Setup();
    if (!ValidateParameters()) {
        return false;
    }
    while(!m_outputQueue.empty()) m_outputQueue.pop();
    while(!m_thresholdQueue.empty()) m_thresholdQueue.pop();
	return true;
}

bool RADAR_CFAR_Block::DataStreamRun()
{
    std::string inputPort = GetInputPortName(0);
    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) {
        return false;
    }

    std::vector<double> detectorOut(static_cast<size_t>(m_cellSize), 0.0);
    for (int i = 0; i < m_cellSize; i++)
    {
        switch (m_detectorType)
        {
        case RADAR_CFAR::Envelope:
            detectorOut[static_cast<size_t>(i)] = std::abs(inputData[static_cast<size_t>(i)]);
            break;
        case RADAR_CFAR::Square:
            detectorOut[static_cast<size_t>(i)] = std::abs(inputData[static_cast<size_t>(i)]);
            break;
        case RADAR_CFAR::LogSquare:
            detectorOut[static_cast<size_t>(i)] = inputData[static_cast<size_t>(i)];
            break;
        case RADAR_CFAR::Log:
            detectorOut[static_cast<size_t>(i)] = inputData[static_cast<size_t>(i)];
            break;
        default:
            break;
        }
    }

    std::vector<double> thresholdData(static_cast<size_t>(m_cellSize), 0.0);
    std::vector<double> outputData(static_cast<size_t>(m_cellSize), 0.0);

    std::vector<double> leadingWindow(static_cast<size_t>(m_referenceCell), 0.0);
    std::vector<double> laggingWindow(static_cast<size_t>(m_referenceCell), 0.0);

    for (int i = 0; i < m_cellSize; i++)
    {
        for (int n = 0; n < m_referenceCell; n++)
        {
            int leadingIndex = i + n - m_guardCell - m_referenceCell;
            int laggingIndex = i + n + m_guardCell + 1;

            leadingWindow[static_cast<size_t>(n)] = detectorOut[static_cast<size_t>(leadingIndex < 0 ? leadingIndex + m_cellSize : leadingIndex)];
            laggingWindow[static_cast<size_t>(n)] = detectorOut[static_cast<size_t>(laggingIndex >= m_cellSize ? laggingIndex - m_cellSize : laggingIndex)];
        }

        double leadingAvg = 0.0;
        double laggingAvg = 0.0;
        for (int n = 0; n < m_referenceCell; n++)
        {
            leadingAvg += leadingWindow[static_cast<size_t>(n)];
            laggingAvg += laggingWindow[static_cast<size_t>(n)];
        }
        leadingAvg /= m_referenceCell;
        laggingAvg /= m_referenceCell;

        switch (m_cfarType)
        {
        case RADAR_CFAR::CA:
            thresholdData[static_cast<size_t>(i)] = m_thresholdFactor * (leadingAvg + laggingAvg) / 2;
            break;
        case RADAR_CFAR::SOCA:
            thresholdData[static_cast<size_t>(i)] = m_thresholdFactor * std::min(leadingAvg, laggingAvg);
            break;
        case RADAR_CFAR::GOCA:
            thresholdData[static_cast<size_t>(i)] = m_thresholdFactor * std::max(leadingAvg, laggingAvg);
            break;
        case RADAR_CFAR::OS:
        {
            std::vector<double> referenceWindowOrder;
            referenceWindowOrder.reserve(static_cast<size_t>(m_referenceCell * 2));
            for (int j = 0; j < m_referenceCell; j++)
            {
                referenceWindowOrder.push_back(leadingWindow[static_cast<size_t>(j)]);
                referenceWindowOrder.push_back(laggingWindow[static_cast<size_t>(j)]);
            }
            std::sort(referenceWindowOrder.begin(), referenceWindowOrder.end());

            if (m_kOrder > 0 && m_kOrder <= static_cast<int>(referenceWindowOrder.size())) {
                thresholdData[static_cast<size_t>(i)] = m_thresholdFactor * referenceWindowOrder[static_cast<size_t>(m_kOrder - 1)];
            }
            break;
        }
        case RADAR_CFAR::ClutterMap:
            break;
        default:
            break;
        }

        if (inputData[static_cast<size_t>(i)] > thresholdData[static_cast<size_t>(i)])
        {
            outputData[static_cast<size_t>(i)] = inputData[static_cast<size_t>(i)];
        }
        else
        {
            outputData[static_cast<size_t>(i)] = 0.0;
        }
    }

    WriteOutputData(GetOutputPortName(0), outputData);
    WriteOutputData(GetOutputPortName(1), thresholdData);

    return true;
}

bool RADAR_CFAR_Block::TimeDrivenRun()
{
    std::string inputPort = GetInputPortName(0);
    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) {
        return true;
    }
    for(const auto& val : inputData) m_inputBuffer.push_back(val);

    if(m_inputBuffer.size() >= static_cast<size_t>(m_cellSize)) {

        std::vector<double> detectorOut(static_cast<size_t>(m_cellSize), 0.0);
        for (int i = 0; i < m_cellSize; i++)
        {
            switch (m_detectorType)
            {
            case RADAR_CFAR::Envelope:
                detectorOut[static_cast<size_t>(i)] = std::abs(m_inputBuffer[static_cast<size_t>(i)]);
                break;
            case RADAR_CFAR::Square:
                detectorOut[static_cast<size_t>(i)] = std::abs(m_inputBuffer[static_cast<size_t>(i)]);
                break;
            case RADAR_CFAR::LogSquare:
                detectorOut[static_cast<size_t>(i)] = m_inputBuffer[static_cast<size_t>(i)];
                break;
            case RADAR_CFAR::Log:
                detectorOut[static_cast<size_t>(i)] = m_inputBuffer[static_cast<size_t>(i)];
                break;
            default:
                break;
            }
        }

        std::vector<double> thresholdData(static_cast<size_t>(m_cellSize), 0.0);
        std::vector<double> outputData(static_cast<size_t>(m_cellSize), 0.0);

        std::vector<double> leadingWindow(static_cast<size_t>(m_referenceCell), 0.0);
        std::vector<double> laggingWindow(static_cast<size_t>(m_referenceCell), 0.0);

        for (int i = 0; i < m_cellSize; i++)
        {
            for (int n = 0; n < m_referenceCell; n++)
            {
                int leadingIndex = i + n - m_guardCell - m_referenceCell;
                int laggingIndex = i + n + m_guardCell + 1;

                leadingWindow[static_cast<size_t>(n)] = detectorOut[static_cast<size_t>(leadingIndex < 0 ? leadingIndex + m_cellSize : leadingIndex)];
                laggingWindow[static_cast<size_t>(n)] = detectorOut[static_cast<size_t>(laggingIndex >= m_cellSize ? laggingIndex - m_cellSize : laggingIndex)];
            }

            double leadingAvg = 0.0;
            double laggingAvg = 0.0;
            for (int n = 0; n < m_referenceCell; n++)
            {
                leadingAvg += leadingWindow[static_cast<size_t>(n)];
                laggingAvg += laggingWindow[static_cast<size_t>(n)];
            }
            leadingAvg /= m_referenceCell;
            laggingAvg /= m_referenceCell;

            switch (m_cfarType)
            {
            case RADAR_CFAR::CA:
                thresholdData[static_cast<size_t>(i)] = m_thresholdFactor * (leadingAvg + laggingAvg) / 2;
                break;
            case RADAR_CFAR::SOCA:
                thresholdData[static_cast<size_t>(i)] = m_thresholdFactor * std::min(leadingAvg, laggingAvg);
                break;
            case RADAR_CFAR::GOCA:
                thresholdData[static_cast<size_t>(i)] = m_thresholdFactor * std::max(leadingAvg, laggingAvg);
                break;
            case RADAR_CFAR::OS:
            {
                std::vector<double> referenceWindowOrder;
                referenceWindowOrder.reserve(static_cast<size_t>(m_referenceCell * 2));
                for (int j = 0; j < m_referenceCell; j++)
                {
                    referenceWindowOrder.push_back(leadingWindow[static_cast<size_t>(j)]);
                    referenceWindowOrder.push_back(laggingWindow[static_cast<size_t>(j)]);
                }
                std::sort(referenceWindowOrder.begin(), referenceWindowOrder.end());

                if (m_kOrder > 0 && m_kOrder <= static_cast<int>(referenceWindowOrder.size())) {
                    thresholdData[static_cast<size_t>(i)] = m_thresholdFactor * referenceWindowOrder[static_cast<size_t>(m_kOrder - 1)];
                }
                break;
            }
            case RADAR_CFAR::ClutterMap:
                break;
            default:
                break;
            }

            if (m_inputBuffer[static_cast<size_t>(i)] > thresholdData[static_cast<size_t>(i)])
            {
                outputData[static_cast<size_t>(i)] = m_inputBuffer[static_cast<size_t>(i)];
            }
            else
            {
                outputData[static_cast<size_t>(i)] = 0.0;
            }
        }
        for(const auto& val : outputData) m_outputQueue.push(val);
        for(const auto& val : thresholdData) m_thresholdQueue.push(val);
        //执行写入
        if (!m_outputQueue.empty() && !m_thresholdQueue.empty()) {
            double outputValue = m_outputQueue.front();
            double thresholdValue = m_thresholdQueue.front();
            m_outputQueue.pop();
            m_thresholdQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<double>{outputValue});
            WriteOutputData(GetOutputPortName(1), std::vector<double>{thresholdValue});
            m_lastOutput = outputValue;


            qDebug() << "[RADAR_CFAR_Block] 分发输出:" << m_outputCount
                     << " value:" << outputValue << "|" << thresholdValue;
            m_inputBuffer.clear();
        }
    }
    return true;
}

bool RADAR_CFAR_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RADAR_CFAR_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_radarCfar = std::make_unique<RADAR_CFAR>();

	SetDefaultParamters();

	try { m_cfarType = ConvertStringToCFARType(getParameter("CFARType").Value); } catch (...) { }
	try { m_cellSize = std::stoi(getParameter("CellSize").Value); } catch (...) { }
	try { m_referenceCell = std::stoi(getParameter("ReferenceCell").Value); } catch (...) { }
	try { m_guardCell = std::stoi(getParameter("GuardCell").Value); } catch (...) { }
	try { m_kOrder = std::stoi(getParameter("kOrder").Value); } catch (...) { }
	try { m_thresholdScaleFactor = std::stod(getParameter("ThresholdScaleFactor").Value); } catch (...) { }
	try { m_detectorType = ConvertStringToDetectorType(getParameter("DetectorType").Value); } catch (...) { }
	try { m_pf = std::stod(getParameter("Pf").Value); } catch (...) { }
	try { m_alpha = std::stod(getParameter("Alpha").Value); } catch (...) { }
	try { m_beta = std::stod(getParameter("Beta").Value); } catch (...) { }

	if (!ValidateParameters()) {
		return false;
	}

	UpdateThresholdFactor();
	SetParameters();

	AddInputPort("input", m_radarCfar->input, static_cast<size_t>(m_cellSize), Block::DataType::CIRCULAR_BUFFER_DOUBLE);
	AddOutputPort("output", m_radarCfar->output, static_cast<size_t>(m_cellSize), Block::DataType::CIRCULAR_BUFFER_DOUBLE);
	AddOutputPort("threshold", m_radarCfar->threshold, static_cast<size_t>(m_cellSize), Block::DataType::CIRCULAR_BUFFER_DOUBLE);

	return true;
}

RADAR_CFAR::SelectedCFARType RADAR_CFAR_Block::ConvertStringToCFARType(const std::string& value)
{
	const std::string lower = ToLowerCopy(TrimCopy(value));
	if (lower == "ca" || lower == "0") {
		return RADAR_CFAR::CA;
	}
	if (lower == "soca" || lower == "1") {
		return RADAR_CFAR::SOCA;
	}
	if (lower == "goca" || lower == "2") {
		return RADAR_CFAR::GOCA;
	}
	if (lower == "os" || lower == "3") {
		return RADAR_CFAR::OS;
	}
	if (lower == "clutter map" || lower == "cluttermap" || lower == "4") {
		return RADAR_CFAR::ClutterMap;
	}
	return RADAR_CFAR::CA;
}

RADAR_CFAR::SelectedDetectorType RADAR_CFAR_Block::ConvertStringToDetectorType(const std::string& value)
{
	const std::string lower = ToLowerCopy(TrimCopy(value));
	if (lower == "envelope" || lower == "0") {
		return RADAR_CFAR::Envelope;
	}
	if (lower == "square" || lower == "1") {
		return RADAR_CFAR::Square;
	}
	if (lower == "logsquare" || lower == "2") {
		return RADAR_CFAR::LogSquare;
	}
	if (lower == "log" || lower == "3") {
		return RADAR_CFAR::Log;
	}
    return RADAR_CFAR::Square;
}


