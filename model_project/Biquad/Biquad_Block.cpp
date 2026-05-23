#include "Biquad_Block.h"
#include <cmath>

Biquad_Block::Biquad_Block(const std::string& name)
	: Block(name)
	, m_dD1(-1.143)
	, m_dD2(0.4128)
	, m_dN0(0.067455)
	, m_dN1(0.135)
	, m_dN2(0.067455)
	, m_dState1(0.0)
	, m_dState2(0.0)
{
}

void Biquad_Block::SetDefaultParamters()
{
	m_dD1 = -1.143;
	m_dD2 = 0.4128;
	m_dN0 = 0.067455;
	m_dN1 = 0.135;
	m_dN2 = 0.067455;
	m_dState1 = 0.0;
	m_dState2 = 0.0;
}

void Biquad_Block::SetParameters()
{
	if (!m_biquad) {
		return;
	}

	m_biquad->m_dD1 = m_dD1;
	m_biquad->m_dD2 = m_dD2;
	m_biquad->m_dN0 = m_dN0;
	m_biquad->m_dN1 = m_dN1;
    m_biquad->m_dN2 = m_dN2;
}

bool Biquad_Block::Setup()
{
    Block::Setup();
	return true;
}

bool Biquad_Block::Run()
{
    std::string inputPort = GetInputPortName(0);
    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    std::vector<double> outputData;
    outputData.reserve(inputData.size());

    for (size_t i = 0; i < inputData.size(); ++i) {
        const double x = inputData[i];
        const double y = m_dN0 * x + m_dState1;
        const double newS1 = m_dN1 * x - m_dD1 * y + m_dState2;
        const double newS2 = m_dN2 * x - m_dD2 * y;
        m_dState1 = newS1;
        m_dState2 = newS2;
        outputData.push_back(y);
    }

    WriteOutputData(GetOutputPortName(0), outputData);

    return true;
}

bool Biquad_Block::Initialize()
{
	SetBlockType(Block::BlockType::PROCESSOR);

	m_biquad = std::make_unique<SystemVueModelBuilder::Biquad>();

	AddInputPort("input", m_biquad->m_dInput, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
	AddOutputPort("output", m_biquad->m_dOutput, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

	SetDefaultParamters();

	try { m_dD1 = std::stod(getParameter("D1").Value); } catch (...) {}
	try { m_dD2 = std::stod(getParameter("D2").Value); } catch (...) {}
	try { m_dN0 = std::stod(getParameter("N0").Value); } catch (...) {}
	try { m_dN1 = std::stod(getParameter("N1").Value); } catch (...) {}
	try { m_dN2 = std::stod(getParameter("N2").Value); } catch (...) {}

	SetParameters();

	m_dState1 = 0.0;
	m_dState2 = 0.0;

	if (!std::isfinite(m_dD1) || !std::isfinite(m_dD2) ||
		!std::isfinite(m_dN0) || !std::isfinite(m_dN1) || !std::isfinite(m_dN2))
	{
		std::cout << "Biquad: coefficients must be finite numbers." << std::endl;
		return false;
	}

	return true;
}
