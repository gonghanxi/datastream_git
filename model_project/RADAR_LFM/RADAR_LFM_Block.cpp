#include "RADAR_LFM_Block.h"

RADAR_LFM_Block::RADAR_LFM_Block(const std::string &name)
    :Block(name), m_counter(0)
{

}

bool RADAR_LFM_Block::Setup()
{
    Block::Setup();
    return true;
}

bool RADAR_LFM_Block::Run()
{
    if(!CanProcess()) {
        return false;
    }

   if(m_radar_lfm->Run()) {
       //----------------数据处理---------------------
       std::vector<std::complex<double>> DComplexData;
       DComplexData.push_back(std::complex<double>(
           m_radar_lfm->output[0].real(),
           m_radar_lfm->output[0].imag()
           ));
       //----------------数据处理---------------------

       // 获取输出端口名称
       std::string DComplexPort = GetOutputPortName(0);
       //----------------写入数据---------------------
       if(!DComplexData.empty()) {
           WriteOutputData(DComplexPort, DComplexData);
       }

       m_counter++;
       return true;
   }
   else {
       LOG_INFO("RADAR_LFM_Block: Run() failed");
       return false;
   }
}

bool RADAR_LFM_Block::Initialize()
{
    SetBlockType(Block::BlockType::SOURCE);

    m_radar_lfm = std::make_unique<RADAR_LFM>();

    AddOutputPort("output", m_radar_lfm->output, 1, DataType::TIMED_DCOMPLEX);

    SetDefaultParameters();

	try { m_Pulsewidth = ParseStringToMatrix<double>(getParameter("Pulsewidth").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Pulsewidth', using default value."); }
	try { m_PRI = ParseStringToMatrix<double>(getParameter("PRI").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PRI', using default value."); }
	try { m_PRI_Combination = ParseStringToMatrix<int>(getParameter("PRI_Combination").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'PRI_Combination', using default value."); }
	try { m_Bandwidth = ParseStringToMatrix<double>(getParameter("Bandwidth").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'Bandwidth', using default value."); }
	try { m_FM_Offset = ParseStringToMatrix<double>(getParameter("FM_Offset").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'FM_Offset', using default value."); }
	try { m_SampleRate = std::stod(getParameter("SampleRate").Value); } catch (...) { LOG_WARN("Failed to parse parameter 'SampleRate', using default value."); }


    SetParameters(m_SampleRate, m_Pulsewidth, m_PRI, m_PRI_Combination, m_Bandwidth, m_FM_Offset);

    return true;
}

void RADAR_LFM_Block::SetParameters(double sampleRate, SystemVueModelBuilder::Matrix<double> pulsewidth,
                                    SystemVueModelBuilder::Matrix<double> pri, SystemVueModelBuilder::Matrix<int> pri_combination,
                                    SystemVueModelBuilder::Matrix<double> bandwidth, SystemVueModelBuilder::Matrix<double> fm_offset)
{
   if(m_radar_lfm) {
       m_radar_lfm->SampleRate = sampleRate;
       m_radar_lfm->Pulsewidth = pulsewidth;
       m_radar_lfm->PRI = pri;
       m_radar_lfm->PRI_Combination = pri_combination;
       m_radar_lfm->Bandwidth = bandwidth;
       m_radar_lfm->FM_Offset = fm_offset;
   }
}

int RADAR_LFM_Block::GetGeneratedSampleCount() const
{
    return m_counter;
}

void RADAR_LFM_Block::SetDefaultParameters()
{
    m_Pulsewidth.Resize(1,1);
    m_Pulsewidth(0,0) = 1e-5;

    m_PRI.Resize(1,1);
    m_PRI(0,0) = 1e-4;

    m_PRI_Combination.Resize(1,1);
    m_PRI_Combination(0,0) = 1;

    m_Bandwidth.Resize(1,1);
    m_Bandwidth(0,0) = 5e6;

    m_FM_Offset.Resize(1,1);
    m_FM_Offset(0,0) = 0;

    m_SampleRate = 10e6;
}
