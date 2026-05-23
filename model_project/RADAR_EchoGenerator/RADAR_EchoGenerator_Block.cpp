#include "RADAR_EchoGenerator_Block.h"
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
RADAR_EchoGenerator_Block::RADAR_EchoGenerator_Block(const std::string &name)
    :Block(name)
{

}

bool RADAR_EchoGenerator_Block::Setup()
{
    Block::Setup();
    bool bStatus = true;
    while(!m_TargetSignalQueue.empty()) m_TargetSignalQueue.pop();
    while(!m_outSignalQueue.empty()) m_outSignalQueue.pop();
    while(!m_RxSignalQueue.empty()) m_RxSignalQueue.pop();

    BufferReader* inSignalPort = GetInputPort("inSignal");
    BufferReader* TxPlatformLocPort = GetInputPort("TxPlatformLoc");
    BufferReader* RxPlatformLocPort = GetInputPort("RxPlatformLoc");
    BufferReader* TargetScatterLocPort = GetInputPort("TargetScatterLoc");
    BufferReader* TargetScatterRCSPort = GetInputPort("TargetScatterRCS");

    //  参数校验
    if (SampleRate <= 0)
    {
        LOG_ERROR("SampleRate must be > 0");
        bStatus = false;
    }
    if (SimulationSampleNum <= 0)
    {
        LOG_ERROR("SimulationSampleNum must be > 0");
        bStatus = false;
    }
    if (RF_Freq <= 0)
    {
        LOG_ERROR("RF_Freq must be > 0");
        bStatus = false;
    }
    if (TargetScatterLocPort->GetBusConnectionCount() == TargetScatterRCSPort->GetBusConnectionCount())
    {
        TargetNum = TargetScatterRCSPort->GetBusConnectionCount();	// 目标数量
    }
    else
    {
        LOG_ERROR("Port size of TargetScatterLoc and TargetScatterRCS must be the same");
        bStatus = false;
    }

    TxPlatformNum = TxPlatformLocPort->GetBusConnectionCount();	// 雷达发射平台数量
    RxPlatformNum = RxPlatformLocPort->GetBusConnectionCount();	// 雷达接收平台数量
    ChannelNum = inSignalPort->GetBusConnectionCount();			// 信号通道数量（雷达阵元数量）

    if (ChannelNum)
    {
        TargetDelayBuffer.Resize(ChannelNum, SimulationSampleNum);
        outDelayBuffer.Resize(ChannelNum, SimulationSampleNum);
        RxDelayBuffer.Resize(ChannelNum, SimulationSampleNum);
        TargetDelayBuffer.Zero();
        outDelayBuffer.Zero();
        RxDelayBuffer.Zero();
    }
    return bStatus;
}

bool RADAR_EchoGenerator_Block::Run()
{
    if(IsVariableStepMode()) return TimeDrivenRun();
    return DataStreamRun();
}

bool RADAR_EchoGenerator_Block::Initialize()
{
    SetBlockType(Block::BlockType::PROCESSOR);
    m_radar = std::make_unique<RADAR_EchoGenerator>();
    SetDefaultParameters();
    try {
        SampleRate = std::stod(getParameter("SampleRate").Value);
        SystemLoss = std::stod(getParameter("SystemLoss").Value);
        IncludePropagationEffect = ConvertStringToSelectedIncludePropagationEffect(getParameter("IncludePropagationEffect").Value);
        RF_Freq = std::stod(getParameter("RF_Freq").Value);
        SimulationSampleNum = std::stod(getParameter("SimulationSampleNum").Value);
    } catch (...) {
    }
    SetParameters();

    AddInputPort("inSignal", m_radar->inSignal, 1, DataType::ENVELOPE_BUS);
    AddInputPort("TxPlatformLoc", m_radar->TxPlatformLoc, 1, DataType::MATRIX_DOUBLE_BUS);
    AddInputPort("RxPlatformLoc", m_radar->RxPlatformLoc, 1, DataType::MATRIX_DOUBLE_BUS);
    AddInputPort("TargetScatterLoc", m_radar->TargetScatterLoc, 1, DataType::MATRIX_DOUBLE_BUS);
    AddInputPort("TargetScatterRCS", m_radar->TargetScatterRCS, 1, DataType::DOUBLE_BUS);

    AddOutputPort("TargetSignal", m_radar->TargetSignal, 1, DataType::ENVELOPE_BUS);
    AddOutputPort("outSignal", m_radar->outSignal, 1, DataType::ENVELOPE_BUS);
    AddOutputPort("RxSignal", m_radar->RxSignal, 1, DataType::ENVELOPE_BUS);
    return true;
}

void RADAR_EchoGenerator_Block::SetParameters()
{
    if(!m_radar) return;
    m_radar->SampleRate = SampleRate;
    m_radar->SystemLoss = SystemLoss;
    m_radar->IncludePropagationEffect = IncludePropagationEffect;
    m_radar->RF_Freq = RF_Freq;
    m_radar->SimulationSampleNum = SimulationSampleNum;
}

RADAR_EchoGenerator::SelectedIncludePropagationEffect RADAR_EchoGenerator_Block::ConvertStringToSelectedIncludePropagationEffect(const std::string &value)
{
    const std::string lower = ToLowerCopy(TrimCopy(value));
    if(lower == "no" || lower == "0") return RADAR_EchoGenerator::No;
    if(lower == "yes" || lower == "1") return RADAR_EchoGenerator::Yes;
    return RADAR_EchoGenerator::Yes;
}

void RADAR_EchoGenerator_Block::SetDefaultParameters()
{
    SampleRate = 10e6;
    SystemLoss = 0;
    IncludePropagationEffect = RADAR_EchoGenerator::Yes;
    RF_Freq = 10e9;
    SimulationSampleNum = 1000000;
}

bool RADAR_EchoGenerator_Block::DataStreamRun()
{
    //const double c = 3e8;
    const double c = 299792458;
    const double pi = std::acos(-1);
    const std::complex<double> imag_i{ 0,1 };

    SystemVueModelBuilder::Matrix<double>	TargetX(1, TargetNum), TargetY(1, TargetNum), TargetZ(1, TargetNum);	// 声明各目标的绝对坐标
    SystemVueModelBuilder::Matrix<double>	TxX(1, TxPlatformNum), TxY(1, TxPlatformNum), TxZ(1, TxPlatformNum);	// 声明各雷达发射平台的绝对坐标
    SystemVueModelBuilder::Matrix<double>	RxX(1, RxPlatformNum), RxY(1, RxPlatformNum), RxZ(1, RxPlatformNum);	// 声明各雷达接收平台的绝对坐标

    //输入端口
    BufferReader* inSignalPort = GetInputPort("inSignal");//读取了两次
    BufferReader* TxPlatformLocPort = GetInputPort("TxPlatformLoc");//读取了一次
    BufferReader* RxPlatformLocPort = GetInputPort("RxPlatformLoc");//读取了一次
    BufferReader* TargetScatterLocPort = GetInputPort("TargetScatterLoc");//读取了一次
    BufferReader* TargetScatterRCSPort = GetInputPort("TargetScatterRCS");//读取了两次

    //两次读取的inSignal输入数据存储
    std::vector<EnvelopeSignal> TotalinSignalData = ReadInputData<EnvelopeSignal>(inSignalPort->GetName());


    for (int i = 0; i < TargetNum; i++)
    {
        //读取 TargetScatterLoc 数据
        std::vector<DoubleMatrix> TargetScatterLocData(1);
        std::vector<BusConnection> BusStructs = TargetScatterLocPort->GetBusConnections();
        BusStructs[i].bridgeReader->ReadData(TargetScatterLocData);
        // 计算各目标在 ECI 坐标系下的坐标
        TargetX(i) = TargetScatterLocData[0](0);
        TargetY(i) = TargetScatterLocData[0](1);
        TargetZ(i) = TargetScatterLocData[0](2);
    }

    for (int m = 0; m < TxPlatformNum; m++)
    {
        //读取 RxPlatformLoc 数据
        std::vector<DoubleMatrix> RxPlatformLocData(1);
        std::vector<BusConnection> BusStructs = RxPlatformLocPort->GetBusConnections();
        BusStructs[m].bridgeReader->ReadData(RxPlatformLocData);
        // 计算各雷达发射平台在 ECI 坐标系下的坐标
        TxX(m) = RxPlatformLocData[0](0);
        TxY(m) = RxPlatformLocData[0](1);
        TxZ(m) = RxPlatformLocData[0](2);
    }

    for (int n = 0; n < RxPlatformNum; n++)
    {
        //读取 TxPlatformLoc 数据
        std::vector<DoubleMatrix> TxPlatformLocData(1);
        std::vector<BusConnection> BusStructs = TxPlatformLocPort->GetBusConnections();
        BusStructs[n].bridgeReader->ReadData(TxPlatformLocData);
        // 计算各雷达接收平台在 ECI 坐标系下的坐标
        RxX(n) = TxPlatformLocData[0](0);
        RxY(n) = TxPlatformLocData[0](1);
        RxZ(n) = TxPlatformLocData[0](2);
    }

    // 发射平台至目标传播路径下的路径长度、时延、衰减
    for (int m = 0; m < TxPlatformNum; m++)
    {
        for (int i = 0; i < TargetNum; i++)
        {
            // 发射平台至目标的距离
            double TargetRange = std::sqrt((TxX(m) - TargetX(i))*(TxX(m) - TargetX(i)) + (TxY(m) - TargetY(i))*(TxY(m) - TargetY(i)) + (TxZ(m) - TargetZ(i))*(TxZ(m) - TargetZ(i)));
            // 发射平台至目标的时延
            double TargetDelay = TargetRange / c;
            int TargetDelayN = static_cast<int>(TargetDelay * SampleRate);
            // 传播效应（衰减、相移）
            double TargetAtten = 1;
            double TargetPhaseShift = 0;
            if (IncludePropagationEffect)
            {
                // 发射平台至目标的衰减（相对值），注意作用在信号幅值上需要开方
                TargetAtten = TargetRange ? 1 / std::sqrt(8 * pi * std::sqrt(pi) * TargetRange * TargetRange / (c / RF_Freq)) : 1;
                // 相移
                TargetPhaseShift = 2 * pi*RF_Freq*TargetDelay;
            }
            // 多径合成
            for (int k = 0; k < ChannelNum; k++)
            {
                if (Index + TargetDelayN < SimulationSampleNum)
                {
                    //使用 inSignal 数据(第一次读取)
                    TargetDelayBuffer(k, Index + TargetDelayN) += TotalinSignalData[k].complex() * TargetAtten * std::exp(-imag_i * TargetPhaseShift);
                }
            }
        }
    }

    // 目标至接收平台传播路径下的路径长度、时延、衰减
    for (int i = 0; i < TargetNum; i++)
    {
        for (int n = 0; n < RxPlatformNum; n++)
        {
            // 目标至接收平台的距离
            double outRange = std::sqrt((RxX(n) - TargetX(i))*(RxX(n) - TargetX(i)) + (RxY(n) - TargetY(i))*(RxY(n) - TargetY(i)) + (RxZ(n) - TargetZ(i))*(RxZ(n) - TargetZ(i)));
            // 目标至接收平台的时延
            double outDelay = outRange / c;
            int outDelayN = static_cast<int>(outDelay * SampleRate);
            // 传播效应（衰减、相移）
            double outAtten = 1;
            double outPhaseShift = 0;
            if (IncludePropagationEffect)
            {
                // 目标至接收平台的衰减（相对值），注意作用在信号幅值上需要开方
                outAtten = outRange ? 1 / std::sqrt(8 * pi * std::sqrt(pi) * outRange * outRange / (c / RF_Freq)) : 1;
                // 相移
                outPhaseShift = 2 * pi*RF_Freq*outDelay;
            }
            // 多径合成
            for (int k = 0; k < ChannelNum; k++)
            {
                if (Index + outDelayN < SimulationSampleNum)
                {
                    std::vector<double> TargetScatterRCSData(1);
                    std::vector<BusConnection> BusStructs = TargetScatterRCSPort->GetBusConnections();
                    BusStructs[k].bridgeReader->ReadData(TargetScatterRCSData);
                    outDelayBuffer(k, Index + outDelayN) += TargetDelayBuffer(k, Index) * outAtten * std::exp(-imag_i * outPhaseShift) * TargetScatterRCSData[0];
                }
            }
        }
    }

    // 发射平台至接收平台（直达信号）的路径长度、时延、衰减
    for (int m = 0; m < TxPlatformNum; m++)
    {
        for (int n = 0; n < RxPlatformNum; n++)
        {
            // 发射平台至接收平台的距离
            double RxRange = std::sqrt((TxX(m) - RxX(n))*(TxX(m) - RxX(n)) + (TxY(m) - RxY(n))*(TxY(m) - RxY(n)) + (TxZ(m) - RxZ(n))*(TxZ(m) - RxZ(n)));
            // 发射平台至接收平台的时延
            double RxDelay = RxRange / c;
            int RxDelayN = static_cast<int>(RxDelay * SampleRate);
            // 传播效应（衰减、相移）
            double RxAtten = 1;
            double RxPhaseShift = 0;
            if (IncludePropagationEffect)
            {
                // 发射平台至接收平台的衰减（相对值），注意作用在信号幅值上需要开方
                RxAtten = RxRange ? 1 / std::sqrt(8 * pi * std::sqrt(pi) * RxRange * RxRange / (c / RF_Freq)) : 1;
                // 相移
                RxPhaseShift = 2 * pi*RF_Freq*RxDelay;
            }
            // 多径合成
            for (int k = 0; k < ChannelNum; k++)
            {
                if (Index + RxDelayN < SimulationSampleNum)
                {
                    RxDelayBuffer(k, Index + RxDelayN) += TotalinSignalData[k].complex() * RxAtten * std::exp(-imag_i * RxPhaseShift);
                }
            }
        }
    }

    Buffer* TargetSignalPort = GetOutputPort("TargetSignal");
    Buffer* outSignalPort = GetOutputPort("outSignal");
    Buffer* RxSignalPort = GetOutputPort("RxSignal");
    double TargetSignalfc = TargetSignalPort->getCharacterizationFrequency();
    double outSignalfc = outSignalPort->getCharacterizationFrequency();
    double RxSignalfc = RxSignalPort->getCharacterizationFrequency();

    // 输出当前索引下的缓存内容
    for (size_t k = 0; k < ChannelNum; k++)
    {
        if (k < TargetSignalPort->GetBusConnectionCount())
        {
            std::vector<EnvelopeSignal> TargetSignalData(1);
            TargetSignalData[0] = TargetDelayBuffer(k, Index);
            TargetSignalPort->WriteEnvelopeDataToChannel(k, TargetSignalData, TargetSignalfc);
        }
        if (k < outSignalPort->GetBusConnectionCount())
        {
            std::vector<EnvelopeSignal> outSignalData(1);
            outSignalData[0] = outDelayBuffer(k, Index) * std::pow(10, -SystemLoss / 20);
            outSignalPort->WriteEnvelopeDataToChannel(k, outSignalData, outSignalfc);
        }
        if (k < RxSignalPort->GetBusConnectionCount())
        {
            std::vector<EnvelopeSignal> RxSignalData(1);
            RxSignalData[0] = RxDelayBuffer(k, Index);
            RxSignalPort->WriteEnvelopeDataToChannel(k, RxSignalData, RxSignalfc);
        }
    }

    // 每个Run索引位置递增
    Index++;

    return true;
}

bool RADAR_EchoGenerator_Block::TimeDrivenRun()
{
    //const double c = 3e8;
    const double c = 299792458;
    const double pi = std::acos(-1);
    const std::complex<double> imag_i{ 0,1 };

    SystemVueModelBuilder::Matrix<double>	TargetX(1, TargetNum), TargetY(1, TargetNum), TargetZ(1, TargetNum);	// 声明各目标的绝对坐标
    SystemVueModelBuilder::Matrix<double>	TxX(1, TxPlatformNum), TxY(1, TxPlatformNum), TxZ(1, TxPlatformNum);	// 声明各雷达发射平台的绝对坐标
    SystemVueModelBuilder::Matrix<double>	RxX(1, RxPlatformNum), RxY(1, RxPlatformNum), RxZ(1, RxPlatformNum);	// 声明各雷达接收平台的绝对坐标

    //输入端口
    BufferReader* inSignalPort = GetInputPort("inSignal");//读取了两次
    BufferReader* TxPlatformLocPort = GetInputPort("TxPlatformLoc");//读取了一次
    BufferReader* RxPlatformLocPort = GetInputPort("RxPlatformLoc");//读取了一次
    BufferReader* TargetScatterLocPort = GetInputPort("TargetScatterLoc");//读取了一次
    BufferReader* TargetScatterRCSPort = GetInputPort("TargetScatterRCS");//读取了两次

    bool inSignalEmpty = ReadDataToBuffer(inSignalPort, m_inSignalBuffer);
    bool txLocEmpty = ReadDataToBuffer(TxPlatformLocPort, m_TxPlatformLocBuffer);
    bool rxLocEmpty = ReadDataToBuffer(RxPlatformLocPort, m_RxPlatformLocBuffer);
    bool scatterLocEmpty = ReadDataToBuffer(TargetScatterLocPort, m_TargetScatterLocBuffer);
    bool rcsEmpty = ReadDataToBuffer(TargetScatterRCSPort, m_TargetScatterRCSBuffer);

    // 如果所有输入都为空，返回false表示不需要处理
    if(inSignalEmpty && txLocEmpty && rxLocEmpty && scatterLocEmpty && rcsEmpty) {
        return false;
    }

    // 检查所有缓冲区是否有足够数据
    bool canProcess =
        CheckBufferReady(m_inSignalBuffer) &&
        CheckBufferReady(m_TxPlatformLocBuffer) &&
        CheckBufferReady(m_RxPlatformLocBuffer) &&
        CheckBufferReady(m_TargetScatterLocBuffer) &&
        CheckBufferReady(m_TargetScatterRCSBuffer);

    if(!canProcess) {
        return true; // 需要等待更多数据
    }

    for (int i = 0; i < TargetNum; i++)
    {
        //读取 TargetScatterLoc 数据
        auto it = m_TargetScatterLocBuffer.find(TargetScatterLocPort->GetBusConnections()[i].bridgeReader);
        // 计算各目标在 ECI 坐标系下的坐标
        TargetX(i) = it->second[0](0);
        TargetY(i) = it->second[0](1);
        TargetZ(i) = it->second[0](2);
    }

    for (int m = 0; m < TxPlatformNum; m++)
    {
        //读取 RxPlatformLoc 数据
        auto it = m_RxPlatformLocBuffer.find(RxPlatformLocPort->GetBusConnections()[m].bridgeReader);
        // 计算各雷达发射平台在 ECI 坐标系下的坐标
        TxX(m) = it->second[0](0);
        TxY(m) = it->second[0](1);
        TxZ(m) = it->second[0](2);
    }

    for (int n = 0; n < RxPlatformNum; n++)
    {
        //读取 TxPlatformLoc 数据
        auto it = m_TxPlatformLocBuffer.find(RxPlatformLocPort->GetBusConnections()[n].bridgeReader);
        // 计算各雷达接收平台在 ECI 坐标系下的坐标
        RxX(n) = it->second[0](0);
        RxY(n) = it->second[0](1);
        RxZ(n) = it->second[0](2);
    }

    // 发射平台至目标传播路径下的路径长度、时延、衰减
    for (int m = 0; m < TxPlatformNum; m++)
    {
        for (int i = 0; i < TargetNum; i++)
        {
            // 发射平台至目标的距离
            double TargetRange = std::sqrt((TxX(m) - TargetX(i))*(TxX(m) - TargetX(i)) + (TxY(m) - TargetY(i))*(TxY(m) - TargetY(i)) + (TxZ(m) - TargetZ(i))*(TxZ(m) - TargetZ(i)));
            // 发射平台至目标的时延
            double TargetDelay = TargetRange / c;
            int TargetDelayN = static_cast<int>(TargetDelay * SampleRate);
            // 传播效应（衰减、相移）
            double TargetAtten = 1;
            double TargetPhaseShift = 0;
            if (IncludePropagationEffect)
            {
                // 发射平台至目标的衰减（相对值），注意作用在信号幅值上需要开方
                TargetAtten = TargetRange ? 1 / std::sqrt(8 * pi * std::sqrt(pi) * TargetRange * TargetRange / (c / RF_Freq)) : 1;
                // 相移
                TargetPhaseShift = 2 * pi*RF_Freq*TargetDelay;
            }
            // 多径合成
            for (int k = 0; k < ChannelNum; k++)
            {
                if (Index + TargetDelayN < SimulationSampleNum)
                {
                    //使用 inSignal 数据(第一次读取)
                    auto it = m_inSignalBuffer.find(inSignalPort->GetBusConnections()[k].bridgeReader);
                    TargetDelayBuffer(k, Index + TargetDelayN) += it->second[k].complex() * TargetAtten * std::exp(-imag_i * TargetPhaseShift);
                }
            }
        }
    }

    // 目标至接收平台传播路径下的路径长度、时延、衰减
    for (int i = 0; i < TargetNum; i++)
    {
        for (int n = 0; n < RxPlatformNum; n++)
        {
            // 目标至接收平台的距离
            double outRange = std::sqrt((RxX(n) - TargetX(i))*(RxX(n) - TargetX(i)) + (RxY(n) - TargetY(i))*(RxY(n) - TargetY(i)) + (RxZ(n) - TargetZ(i))*(RxZ(n) - TargetZ(i)));
            // 目标至接收平台的时延
            double outDelay = outRange / c;
            int outDelayN = static_cast<int>(outDelay * SampleRate);
            // 传播效应（衰减、相移）
            double outAtten = 1;
            double outPhaseShift = 0;
            if (IncludePropagationEffect)
            {
                // 目标至接收平台的衰减（相对值），注意作用在信号幅值上需要开方
                outAtten = outRange ? 1 / std::sqrt(8 * pi * std::sqrt(pi) * outRange * outRange / (c / RF_Freq)) : 1;
                // 相移
                outPhaseShift = 2 * pi*RF_Freq*outDelay;
            }
            // 多径合成
            for (int k = 0; k < ChannelNum; k++)
            {
                if (Index + outDelayN < SimulationSampleNum)
                {
                    auto it = m_TargetScatterRCSBuffer.find(TargetScatterRCSPort->GetBusConnections()[k].bridgeReader);
                    outDelayBuffer(k, Index + outDelayN) += TargetDelayBuffer(k, Index) * outAtten * std::exp(-imag_i * outPhaseShift) * it->second[0];
                }
            }
        }
    }

    // 发射平台至接收平台（直达信号）的路径长度、时延、衰减
    for (int m = 0; m < TxPlatformNum; m++)
    {
        for (int n = 0; n < RxPlatformNum; n++)
        {
            // 发射平台至接收平台的距离
            double RxRange = std::sqrt((TxX(m) - RxX(n))*(TxX(m) - RxX(n)) + (TxY(m) - RxY(n))*(TxY(m) - RxY(n)) + (TxZ(m) - RxZ(n))*(TxZ(m) - RxZ(n)));
            // 发射平台至接收平台的时延
            double RxDelay = RxRange / c;
            int RxDelayN = static_cast<int>(RxDelay * SampleRate);
            // 传播效应（衰减、相移）
            double RxAtten = 1;
            double RxPhaseShift = 0;
            if (IncludePropagationEffect)
            {
                // 发射平台至接收平台的衰减（相对值），注意作用在信号幅值上需要开方
                RxAtten = RxRange ? 1 / std::sqrt(8 * pi * std::sqrt(pi) * RxRange * RxRange / (c / RF_Freq)) : 1;
                // 相移
                RxPhaseShift = 2 * pi*RF_Freq*RxDelay;
            }
            // 多径合成
            for (int k = 0; k < ChannelNum; k++)
            {
                if (Index + RxDelayN < SimulationSampleNum)
                {
                    auto it = m_inSignalBuffer.find(inSignalPort->GetBusConnections()[k].bridgeReader);
                    RxDelayBuffer(k, Index + RxDelayN) += it->second[k].complex() * RxAtten * std::exp(-imag_i * RxPhaseShift);
                }
            }
        }
    }

    Buffer* TargetSignalPort = GetOutputPort("TargetSignal");
    Buffer* outSignalPort = GetOutputPort("outSignal");
    Buffer* RxSignalPort = GetOutputPort("RxSignal");
    double TargetSignalfc = TargetSignalPort->getCharacterizationFrequency();
    double outSignalfc = outSignalPort->getCharacterizationFrequency();
    double RxSignalfc = RxSignalPort->getCharacterizationFrequency();

    // 输出当前索引下的缓存内容
    for (size_t k = 0; k < ChannelNum; k++)
    {
        if (k < TargetSignalPort->GetBusConnectionCount())
        {
            std::vector<EnvelopeSignal> TargetSignalData(1);
            TargetSignalData[0] = TargetDelayBuffer(k, Index);
            m_TargetSignalQueue.push(TargetSignalData[0]);
            EnvelopeSignal outputValue = m_TargetSignalQueue.front();
            m_TargetSignalQueue.pop();
            m_lastTargetSignal = outputValue;
            TargetSignalPort->WriteEnvelopeDataToChannel(k, std::vector<EnvelopeSignal>{outputValue}, TargetSignalfc);
        }
        if (k < outSignalPort->GetBusConnectionCount())
        {
            std::vector<EnvelopeSignal> outSignalData(1);
            outSignalData[0] = outDelayBuffer(k, Index) * std::pow(10, -SystemLoss / 20);
            m_outSignalQueue.push(outSignalData[0]);
            EnvelopeSignal outputValue = m_outSignalQueue.front();
            m_outSignalQueue.pop();
            m_lastoutSignal = outputValue;
            outSignalPort->WriteEnvelopeDataToChannel(k, std::vector<EnvelopeSignal>{outputValue}, outSignalfc);
        }
        if (k < RxSignalPort->GetBusConnectionCount())
        {
            std::vector<EnvelopeSignal> RxSignalData(1);
            RxSignalData[0] = RxDelayBuffer(k, Index);
            m_RxSignalQueue.push(RxSignalData[0]);
            EnvelopeSignal outputValue = m_RxSignalQueue.front();
            m_RxSignalQueue.pop();
            m_lastRxSignal = outputValue;
            RxSignalPort->WriteEnvelopeDataToChannel(k, std::vector<EnvelopeSignal>{outputValue}, RxSignalfc);
        }
    }

    // 每个Run索引位置递增
    Index++;

    return true;
}

