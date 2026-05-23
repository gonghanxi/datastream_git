#include "LinearQuantizer_Block.h"

#include <cmath>
#include <limits>
#include <vector>

LinearQuantizer_Block::LinearQuantizer_Block(const std::string& name)
    : Block(name)
    , m_levels(128)
    , m_low(-3.0)
    , m_high(3.0)
{
}

void LinearQuantizer_Block::SetDefaultParamters()
{
    m_levels = 128;
    m_low = -3.0;
    m_high = 3.0;
}

void LinearQuantizer_Block::SetParameters()
{
    if (!m_linearQuantizer) {
        return;
    }

    m_linearQuantizer->Levels = m_levels;
    m_linearQuantizer->Low = m_low;
    m_linearQuantizer->High = m_high;
}

bool LinearQuantizer_Block::Setup()
{
    Block::Setup();
    return true;
}

bool LinearQuantizer_Block::Run()
{
    if (!CanProcess()) {
        return false;
    }

    if (!m_linearQuantizer) {
        return false;
    }

    const std::string inputPortName = GetInputPortName(0);
    const std::string stepPortName = GetOutputPortName(0);
    const std::string ampPortName = GetOutputPortName(1);

    auto inputData = ReadInputData<double>(inputPortName);
    if (inputData.empty()) {
        return true;
    }

    std::vector<int> stepData;
    std::vector<double> ampData;
    stepData.reserve(inputData.size());
    ampData.reserve(inputData.size());

    const int L = m_levels;
    const double low = m_low;
    const double high = m_high;
    const double delta = (high - low) / static_cast<double>(L - 1);

    if (!(delta > 0.0) || !std::isfinite(delta)) {
        std::cout << "LinearQuantizer: Invalid delta computed." << std::endl;
        return false;
    }

    for (double x : inputData) {
        const double u = (x - low) / delta;
        double k = std::floor(u + 0.5);
        if (k < 0.0) k = 0.0;
        const double kmax = static_cast<double>(L - 1);
        if (k > kmax) k = kmax;

        const double qamp = low + k * delta;

        stepData.push_back(static_cast<int>(k));
        ampData.push_back(qamp);
    }

    WriteOutputData(stepPortName, stepData);
    WriteOutputData(ampPortName, ampData);

    return true;
}

bool LinearQuantizer_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);

    m_linearQuantizer = std::make_unique<LinearQuantizer>();

    AddInputPort("input", m_linearQuantizer->input, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("step", m_linearQuantizer->step, 1, Block::DataType::CIRCULAR_BUFFER_INT);
    AddOutputPort("amp", m_linearQuantizer->amp, 1, Block::DataType::CIRCULAR_BUFFER_DOUBLE);

    SetDefaultParamters();

    try { m_levels = std::stoi(getParameter("Levels").Value); } catch (...) { }
    try { m_low = std::stod(getParameter("Low").Value); } catch (...) { }
    try { m_high = std::stod(getParameter("High").Value); } catch (...) { }

    if (m_levels < 2) {
        std::cout << "LinearQuantizer: Levels must be >= 2." << std::endl;
        return false;
    }
    if (!(m_high > m_low)) {
        std::cout << "LinearQuantizer: High must be greater than Low." << std::endl;
        return false;
    }

    SetParameters();

    return true;
}
