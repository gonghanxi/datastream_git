#include "BiquadCascade_Block.h"
#include "DataTypesAndParsers.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <sstream>

namespace {
std::vector<double> ParseVectorDouble(const std::string& value)
{
	std::vector<double> result;

	try {
		auto mat = DataTypesAndParsers::ParseStringToMatrixDouble(value);
		result.reserve(mat.NumElements());
		for (size_t i = 0; i < mat.NumElements(); ++i) {
			result.push_back(mat(i));
		}
		return result;
	} catch (...) {
	}

	std::string s = value;
	s.erase(std::remove(s.begin(), s.end(), '['), s.end());
	s.erase(std::remove(s.begin(), s.end(), ']'), s.end());
	std::replace(s.begin(), s.end(), ',', ' ');

	std::stringstream ss(s);
	std::string token;
	while (ss >> token) {
		try {
			result.push_back(std::stod(token));
		} catch (...) {
		}
	}

	return result;
}
}

BiquadCascade_Block::BiquadCascade_Block(const std::string& name)
	: Block(name)
	, m_numBiquads(0)
{
}

void BiquadCascade_Block::SetDefaultParamters()
{
	m_taps = {0.067455, 0.135, 0.067455, 1.0, -1.143, 0.4128};
}

bool BiquadCascade_Block::BuildCascade()
{
	m_blocks.clear();
	m_state1.clear();
	m_state2.clear();
	m_numBiquads = 0;

	if (m_taps.empty() || m_taps.size() < 6) {
        LOG_ERROR("BiquadCascade: Taps must contain at least 6 coefficients.");
		return false;
	}

	if (m_taps.size() % 6 != 0) {
        LOG_ERROR("BiquadCascade: Taps length must be a multiple of 6 "
            "(N0, N1, N2, D0, D1, D2 for each section).");
		return false;
	}

	m_numBiquads = static_cast<std::size_t>(m_taps.size() / 6);
	m_blocks.resize(m_numBiquads);
	m_state1.assign(m_numBiquads, 0.0);
	m_state2.assign(m_numBiquads, 0.0);

	for (std::size_t i = 0; i < m_numBiquads; ++i) {
		const size_t base = 6 * i;
		const double N0 = m_taps[base + 0];
		const double N1 = m_taps[base + 1];
		const double N2 = m_taps[base + 2];
		const double D0 = m_taps[base + 3];
		const double D1 = m_taps[base + 4];
		const double D2 = m_taps[base + 5];

		if (std::fabs(D0) < DBL_EPSILON) {
            LOG_ERROR("BiquadCascade: D0 (denominator constant term) "
                "must be non-zero in each biquad.");
			return false;
		}

		const double invD0 = 1.0 / D0;

		BiquadBlock& b = m_blocks[i];
		b.b0 = N0 * invD0;
		b.b1 = N1 * invD0;
		b.b2 = N2 * invD0;
		b.a1 = D1 * invD0;
		b.a2 = D2 * invD0;
	}

	return true;
}

bool BiquadCascade_Block::Setup()
{
	Block::Setup();
	return true;
}

bool BiquadCascade_Block::Run()
{
    if (m_numBiquads == 0 || m_blocks.empty()) {
        std::vector<double> zeroData(1, 0.0);
        WriteOutputData(GetOutputPortName(0), zeroData);
        return true;
    }

    std::string inputPort = GetInputPortName(0);
    auto inputData = ReadInputData<double>(inputPort);
    if (inputData.empty()) {
        return true;
    }

    double u = inputData[0];
    std::vector<double> stageOut(m_numBiquads, 0.0);

    for (std::size_t i = 0; i < m_numBiquads; ++i) {
        BiquadBlock& b = m_blocks[i];
        double& z1 = m_state1[i];
        double& z2 = m_state2[i];

        const double t = b.b0 * u + z1;
        const double y = t;

        const double new_z1 = b.b1 * u - b.a1 * y + z2;
        const double new_z2 = b.b2 * u - b.a2 * y;

        z1 = new_z1;
        z2 = new_z2;

        stageOut[i] = y;
        u = y;
    }

    // 获取输出 Buffer
    std::string outputPort = GetOutputPortName(0);
    Buffer* outputBuffer = GetOutputPort(outputPort);

    if (!outputBuffer) {
        LOG_ERROR("Failed to get output buffer");
        return false;
    }

    // 逐个通道写入不同数据
    for (std::size_t j = 0; j < m_numBiquads; ++j) {
        const std::size_t stageIndex = m_numBiquads - 1 - j;
        std::vector<double> singleData;
        singleData.push_back(stageOut[stageIndex]);

        // 写入到指定通道
        outputBuffer->WriteDataToChannel(static_cast<int>(j), singleData);
    }

    return true;
}

bool BiquadCascade_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_biquadCascade = std::make_unique<SystemVueModelBuilder::BiquadCascade>();

    AddInputPort("input", m_biquadCascade->m_dInput, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("output", m_biquadCascade->m_dOutput, 1, Block::DataType::DOUBLE_BUS);

    SetDefaultParamters();

    try { m_taps = ParseVectorDouble(getParameter("Taps").Value); } catch (...) { }

    if (!BuildCascade()) {
        return false;
    }

    return true;
}
