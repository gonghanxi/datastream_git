#include "RADAR_PULSE_Block.h"

//#include <string>
//#include <map>
//template class RADAR_PULSE_API std::map<std::string, Parameter>;
//template class RADAR_PULSE_API std::map<int, PortMsg>;

//template class RADAR_PULSE_API std::map<int, std::string>;


RADAR_PULSE_Block::RADAR_PULSE_Block(const std::string& name)
    : Block(name), m_counter(0)
{}

bool RADAR_PULSE_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RADAR_PULSE_Block::Run()
{
     if(!CanProcess()) {
         return false;
     }

    if(m_radarPULSE->Run()) {
        //qDebug() << "m_radarPULSE->output[0]: " << m_radarPULSE->output[0];
        //----------------数据处理---------------------
        std::vector<double> DoubleData;
        DoubleData.push_back(m_radarPULSE->output[0]);
        //----------------数据处理---------------------

        // 获取输出端口名称
        std::string DoublePort = GetOutputPortName(0);
        //----------------写入数据---------------------
        if(!DoubleData.empty()) {
            WriteOutputData(DoublePort, DoubleData);
        }

        m_counter++;
        return true;
    }
    else {
        return false;
    }
}

bool RADAR_PULSE_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_radarPULSE = std::make_unique<RADAR_PULSE>();

    AddOutputPort("output", m_radarPULSE->output, 1, Block::DataType::TIMED_DOUBLE);


    SetDefaultParameters();

    m_pulsewidth = ParseStringToMatrix<double>(getParameter("Pulsewidth").Value);
    m_PRI = ParseStringToMatrix<double>(getParameter("PRI").Value);
    m_PRI_Combination = ParseStringToMatrix<int>(getParameter("PRI_Combination").Value);
    m_sampleRate = std::stod(getParameter("SampleRate").Value);

    SetParameters(m_sampleRate, m_pulsewidth, m_PRI, m_PRI_Combination);

    return true;
}

void RADAR_PULSE_Block::SetParameters(double sampleRate, SystemVueModelBuilder::Matrix<double> pulsewidth, SystemVueModelBuilder::Matrix<double> pri, SystemVueModelBuilder::Matrix<int> pri_combination)
{
    if(m_radarPULSE) {
        m_radarPULSE->Pulsewidth = pulsewidth;
        m_radarPULSE->PRI = pri;
        m_radarPULSE->PRI_Combination = pri_combination;
        m_radarPULSE->SampleRate = sampleRate;
    }
}

int RADAR_PULSE_Block::GetGeneratedSampleCount() const
{
    return m_counter;
}

void RADAR_PULSE_Block::SetDefaultParameters()
{
    m_pulsewidth.Resize(1,1);
    m_pulsewidth(0,0) = 1e-5;

//    qDebug() << "m_pulsewidth" << m_pulsewidth.Size(1);

    m_PRI.Resize(1,1);
    m_PRI(0,0) = 1e-4;

    m_PRI_Combination.Resize(1,1);
    m_PRI_Combination(0,0) = 1;

    m_sampleRate = 10e6;
}
