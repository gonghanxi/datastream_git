#include "AddGuard_Block.h"

AddGuard_Block::AddGuard_Block(const std::string &name)
    :Block(name)
{

}

AddGuard_Block::~AddGuard_Block()
{
    if (m_cplxBuffer)
    {
        delete[] m_cplxBuffer;
        m_cplxBuffer = nullptr;
    }
}

bool AddGuard_Block::Setup()
{
    qDebug() << "AddGuard_Block::Setup begin";
    Block::Setup();
    return true;
}

bool AddGuard_Block::Run()
{
    if(IsVariableStepMode()) {
        return TimeDrivenRun();
    }
    return DataStreamRun();
}

bool AddGuard_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_AddGuard = std::make_unique<AddGuard>();
    SetDefaultParameters();
    try { m_iIFFTSize = std::stoi(getParameter("iIFFTSize").Value); } catch(...) {}
    try { m_iPreGuard = std::stoi(getParameter("iPreGuard").Value); } catch(...) {}
    try { m_iPostGuard = std::stoi(getParameter("iPostGuard").Value); } catch(...) {}
    try { m_iIntersection = std::stoi(getParameter("iIntersection").Value); } catch(...) {}
    SetParameters();

    if (m_cplxBuffer)
    {
        delete[] m_cplxBuffer;
        m_cplxBuffer = nullptr;
    }

    if (m_iIntersection > 0)
        m_cplxBuffer = new std::complex<double>[m_iIntersection];

    ClearCplxBuffer();

    if(!ModelSetup()) return false;
    AddInputPort("In", m_AddGuard->m_cbInput, static_cast<size_t>(m_iIFFTSize), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    AddInputPort("Window", m_AddGuard->m_cbWindow, static_cast<size_t>(m_iNwin), DataType::CIRCULAR_BUFFER_DOUBLE);
    AddOutputPort("Out", m_AddGuard->m_cbOutput, static_cast<size_t>(m_iNout), DataType::CIRCULAR_BUFFER_DCOMPLEX);
    return true;
}

void AddGuard_Block::SetParameters()
{
    if(!m_AddGuard) return;
    m_AddGuard->m_iIFFTSize = m_iIFFTSize;
    m_AddGuard->m_iPreGuard = m_iPreGuard;
    m_AddGuard->m_iPostGuard = m_iPostGuard;
    m_AddGuard->m_iIntersection = m_iIntersection;
}

void AddGuard_Block::SetDefaultParameters()
{
    m_iIFFTSize = 64;
    m_iPreGuard = 16;
    m_iPostGuard = 0;
    m_iIntersection = 0;

    m_iNout = 0;
    m_iNwin = 0;
    m_iNperiod = 0;
    m_cplxBuffer = nullptr;
}

void AddGuard_Block::ClearCplxBuffer()
{
    if (m_cplxBuffer && m_iIntersection > 0)
    {
        for (int i = 0; i < m_iIntersection; ++i)
            m_cplxBuffer[i] = std::complex<double>(0.0, 0.0);
    }
}

bool AddGuard_Block::ModelSetup()
{
    if (m_iIFFTSize < 1)
        m_iIFFTSize = 1;

    if (m_iPreGuard < 0)
        m_iPreGuard = 0;
    if (m_iPostGuard < 0)
        m_iPostGuard = 0;

    if (m_iPreGuard > m_iIFFTSize)
        m_iPreGuard = m_iIFFTSize;
    if (m_iPostGuard > m_iIFFTSize)
        m_iPostGuard = m_iIFFTSize;

    if (m_iIntersection < 0)
        m_iIntersection = 0;

    const int guardSum = m_iPreGuard + m_iPostGuard;
    if (m_iIntersection > guardSum)
        m_iIntersection = guardSum;

    const int L = m_iPreGuard + m_iIFFTSize + m_iPostGuard;

    if (2 * m_iIntersection > L)
        m_iIntersection = L / 2;

    m_iNwin = static_cast<size_t>(L);
    m_iNout = static_cast<size_t>(L - m_iIntersection);
    m_iNperiod = static_cast<size_t>(m_iIFFTSize + m_iPreGuard);

    return true;
}

bool AddGuard_Block::DataStreamRun()
{
    auto m_cbInputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    auto m_cbWindowData = ReadInputData<double>(GetInputPortName(1));
    std::vector<std::complex<double>> m_cbOutputData(m_iNout);
    const int N = m_iIFFTSize;
    const int Pg = m_iPreGuard;
    const int Po = m_iPostGuard;
    const int I = m_iIntersection;

    const int L = static_cast<int>(m_iNwin);
    const int Lout = static_cast<int>(m_iNout);

    auto compute_u = [&](int idx) -> std::complex<double>
    {
        std::complex<double> x;

        if (idx < Pg)
        {
            const int src = N - Pg + idx;
            x = m_cbInputData[src];
        }
        else if (idx < Pg + N)
        {
            const int src = idx - Pg;
            x = m_cbInputData[src];
        }
        else
        {
            const int src = idx - (Pg + N);
            x = m_cbInputData[src];
        }

        const double w = m_cbWindowData[idx];
        return x * w;
    };

    if (I <= 0)
    {
        for (int n = 0; n < L; ++n)
            m_cbOutputData[n] = compute_u(n);
        return true;
    }

    int outIdx = 0;

    for (int n = 0; n < I; ++n)
    {
        const std::complex<double> u = compute_u(n);
        const std::complex<double> prev =
            (m_cplxBuffer && n < m_iIntersection)
            ? m_cplxBuffer[n]
            : std::complex<double>(0.0, 0.0);

        m_cbOutputData[outIdx++] = prev + u;
    }

    const int middleStart = I;
    const int middleEnd = L - I; // [I, L-I)
    for (int n = middleStart; n < middleEnd; ++n)
    {
        m_cbOutputData[outIdx++] = compute_u(n);
    }

    if (m_cplxBuffer)
    {
        for (int k = 0; k < I; ++k)
        {
            const int n = L - I + k;
            m_cplxBuffer[k] = compute_u(n);
        }
    }

    (void)Lout;
    WriteOutputData(GetOutputPortName(0), m_cbOutputData);
    return true;
}

bool AddGuard_Block::TimeDrivenRun()
{
    auto m_cbInputData = ReadInputData<std::complex<double>>(GetInputPortName(0));
    auto m_cbWindowData = ReadInputData<double>(GetInputPortName(1));
    if(m_cbInputData.empty() && m_cbWindowData.empty()) {
        return true;
    }
    if(!m_cbInputData.empty()) {
        for(size_t i = 0; i < m_cbInputData.size();i++) {
            m_InBuffer.push_back(m_cbInputData[i]);
        }
    }
    if(!m_cbWindowData.empty()) {
        for(size_t i = 0; i < m_cbWindowData.size();i++) {
            m_WindowBuffer.push_back(m_cbWindowData[i]);
        }
    }
    if(m_InBuffer.size() == static_cast<size_t>(m_iIFFTSize) && m_WindowBuffer.size() == static_cast<size_t>(m_iNwin)) {
        const int N = m_iIFFTSize;
        const int Pg = m_iPreGuard;
        const int Po = m_iPostGuard;
        const int I = m_iIntersection;
        const int L = static_cast<int>(m_iNwin);
        const int Lout = static_cast<int>(m_iNout);

        std::vector<std::complex<double>> outputData;
        outputData.reserve(Lout);

        // 复用原数据流模式的计算逻辑
        auto compute_u = [&](int idx) -> std::complex<double>
        {
            std::complex<double> x;
            if (idx < Pg)
            {
                int src = N - Pg + idx;
                x = m_InBuffer[src];
            }
            else if (idx < Pg + N)
            {
                int src = idx - Pg;
                x = m_InBuffer[src];
            }
            else
            {
                int src = idx - (Pg + N);
                x = m_InBuffer[src];
            }
            double w = m_WindowBuffer[idx];
            return x * w;
        };

        if (I <= 0)
        {
            for (int n = 0; n < L; ++n)
                outputData.push_back(compute_u(n));
        }
        else
        {
            // 交叠部分：前 I 个输出需要加上上一次保留的交叠数据
            for (int n = 0; n < I; ++n)
            {
                std::complex<double> u = compute_u(n);
                std::complex<double> prev = (m_cplxBuffer && n < I) ? m_cplxBuffer[n] : std::complex<double>(0.0, 0.0);
                outputData.push_back(prev + u);
            }
            // 中间部分
            for (int n = I; n < L - I; ++n)
                outputData.push_back(compute_u(n));
            // 更新交叠缓冲区：保存当前块的最后 I 个未加权的值（用于下一块）
            if (m_cplxBuffer)
            {
                for (int k = 0; k < I; ++k)
                {
                    int n = L - I + k;
                    m_cplxBuffer[k] = compute_u(n);
                }
            }
        }

        // 将处理结果全部放入输出队列
        for (const auto& val : outputData)
            m_outputQueue.push(val);

        // 清空累积缓冲区
        m_InBuffer.clear();
        m_WindowBuffer.clear();
        if (!m_outputQueue.empty())
        {
            std::complex<double> outputValue = m_outputQueue.front();
            m_outputQueue.pop();
            m_outputCount++;

            WriteOutputData(GetOutputPortName(0), std::vector<std::complex<double>>{outputValue});
            m_lastOutput = outputValue;

            qDebug() << "[AddGuard_Block] 分发输出: " << m_outputCount << "/" << m_iNout
                     << " value: (" << outputValue.real() << "," << outputValue.imag() << ")";
        }
    }
    qDebug() << "[AddGuard_Block]";
    return true;
}
