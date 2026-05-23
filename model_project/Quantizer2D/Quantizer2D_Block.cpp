#include "Quantizer2D_Block.h"

Quantizer2D_Block::Quantizer2D_Block(const std::string &name)
    :Block(name)
{

}

bool Quantizer2D_Block::Setup()
{
    Block::Setup();
    if(!ModelSetup()) return false;
    return true;
}

bool Quantizer2D_Block::Run()
{
    std::string inputPort = GetInputPortName(0);
    std::string outputPort = GetOutputPortName(0);
    BufferReader* reader = GetInputPort(inputPort);
    Buffer* buffer = GetOutputPort(outputPort);


    std::complex<double> input;
    std::complex<double> output;
    reader->ReadData(input);

    const std::complex<double> x = input;

    const std::size_t numPoints = QuantList.NumElements();

    if (numPoints > 0) {
        double bestDist2 = std::numeric_limits<double>::infinity();
        std::complex<double> bestPoint(0.0, 0.0);

        for (std::size_t i = 0; i < numPoints; ++i) {
            const std::complex<double>& q = QuantList(i);
            const double dx = x.real() - q.real();
            const double dy = x.imag() - q.imag();
            const double d2 = dx * dx + dy * dy;

            if (d2 < bestDist2) {
                bestDist2 = d2;
                bestPoint = q;
            }
        }

        output = bestPoint;
        return true;
    }

    const int nx = static_cast<int>(std::floor(Nx + 0.5));
    const int ny = static_cast<int>(std::floor(Ny + 0.5));

    double tx = 0.0;
    if (VxMax != VxMin) {
        tx = (x.real() - VxMin) / (VxMax - VxMin) * static_cast<double>(nx - 1);
    }
    int ix = static_cast<int>(std::floor(tx + 0.5));
    if (ix < 0)        ix = 0;
    else if (ix > nx - 1) ix = nx - 1;

    double ty = 0.0;
    if (VyMax != VyMin) {
        ty = (x.imag() - VyMin) / (VyMax - VyMin) * static_cast<double>(ny - 1);
    }
    int iy = static_cast<int>(std::floor(ty + 0.5));
    if (iy < 0)        iy = 0;
    else if (iy > ny - 1) iy = ny - 1;

    const double xr = VxMin + static_cast<double>(ix) * xDelta;
    const double yi = VyMin + static_cast<double>(iy) * yDelta;

    output = std::complex<double>(xr, yi);
    buffer->WriteData(output);
    return true;
}

bool Quantizer2D_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_Quantizer = std::make_unique<Quantizer2D>();

    SetDefaultParameters();

    try { VxMax = std::stod(getParameter("VxMax").Value); } catch(...) {}
    try { VxMin = std::stod(getParameter("VxMin").Value); } catch(...) {}
    try { Nx = std::stod(getParameter("Nx").Value); } catch(...) {}
    try { VyMax = std::stod(getParameter("VxMax").Value); } catch(...) {}
    try { VyMin = std::stod(getParameter("VxMax").Value); } catch(...) {}
    try { Ny = std::stod(getParameter("VxMax").Value); } catch(...) {}
    try { QuantList = ParseStringToMatrix<std::complex<double>>(getParameter("VxMax").Value); } catch(...) {}

    SetParameters();

    AddInputPort("input", m_Quantizer->input, 1, DataType::COMPLEX_DOUBLE);
    AddOutputPort("output", m_Quantizer->output, 1, DataType::COMPLEX_DOUBLE);
    return true;
}

void Quantizer2D_Block::SetParameters()
{
    if(!m_Quantizer) return;
    m_Quantizer->VxMax     = VxMax;
    m_Quantizer->VxMin     = VxMin;
    m_Quantizer->Nx        = Nx;
    m_Quantizer->VyMax     = VyMax;
    m_Quantizer->VyMin     = VyMin;
    m_Quantizer->Ny        = Ny;
    m_Quantizer->QuantList = QuantList;
}

bool Quantizer2D_Block::ModelSetup()
{
    if (!ValidateParameters()) {
        return false;
    }

    const std::size_t numPoints = QuantList.NumElements();
    if (numPoints == 0) {
        const int nx = static_cast<int>(std::floor(Nx + 0.5));
        const int ny = static_cast<int>(std::floor(Ny + 0.5));

        if (nx > 1) {
            xDelta = (VxMax - VxMin) / static_cast<double>(nx - 1);
        }
        else {
            xDelta = 0.0;
        }

        if (ny > 1) {
            yDelta = (VyMax - VyMin) / static_cast<double>(ny - 1);
        }
        else {
            yDelta = 0.0;
        }
    }

    return true;
}

void Quantizer2D_Block::SetDefaultParameters()
{
    VxMax = 1;
    VxMin = -1;
    Nx = 16;
    VyMax = 1;
    VyMin = -1;
    Ny = 16;
    QuantList.Resize(1,1);
    QuantList(0,0) = std::complex<double>(0.0,0.0);
}

bool Quantizer2D_Block::ValidateParameters()
{
    const std::size_t numPoints = QuantList.NumElements();
    if (numPoints > 0) {
        return true;
    }

    bool ok = true;

    if (Nx < 1.0) {
        LOG_ERROR("Quantizer2D: Nx (number of real output levels) must be >= 1.");
        ok = false;
    }
    if (Ny < 1.0) {
        LOG_ERROR("Quantizer2D: Ny (number of imaginary output levels) must be >= 1.");
        ok = false;
    }
    if (!(VxMax > VxMin)) {
        LOG_ERROR("Quantizer2D: VxMax must be greater than VxMin.");
        ok = false;
    }
    if (!(VyMax > VyMin)) {
        LOG_ERROR("Quantizer2D: VyMax must be greater than VyMin.");
        ok = false;
    }

    return ok;
}
