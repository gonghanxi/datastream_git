#ifndef RADAR_PHASEDARRAYTX_BLOCK_H
#define RADAR_PHASEDARRAYTX_BLOCK_H

#include "Block.h"
#include "RADAR_PhasedArrayTx.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_PhasedArrayTx_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_PhasedArrayTx_Block(const std::string& name);
    ~RADAR_PhasedArrayTx_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

    void SetDefaultParameters();
    void SetParameters();

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    // ConvertStringTo
    static RADAR_PhasedArrayTx::SelectedConfiguration     ConvertStringToConfiguration(const std::string& value);
    static RADAR_PhasedArrayTx::SelectedAxisType          ConvertStringToAxisType(const std::string& value);
    static RADAR_PhasedArrayTx::SelectedArray2DShapeType  ConvertStringToArray2DShapeType(const std::string& value);
    static RADAR_PhasedArrayTx::SelectedSpaceType         ConvertStringToSpaceType(const std::string& value);
    static RADAR_PhasedArrayTx::SelectedGridType          ConvertStringToGridType(const std::string& value);
    static RADAR_PhasedArrayTx::SelectedReliabilityType   ConvertStringToReliabilityType(const std::string& value);
    static RADAR_PhasedArrayTx::SelectedWindowType        ConvertStringToWindowType(const std::string& value);
    static RADAR_PhasedArrayTx::SelectedYesorNo           ConvertStringToYesorNo(const std::string& value);
    static RADAR_PhasedArrayTx::SelectedPhaseShiftType    ConvertStringToPhaseShiftType(const std::string& value);

    std::unique_ptr<RADAR_PhasedArrayTx> m_algo;

    // ===== 参数 =====
    RADAR_PhasedArrayTx::SelectedConfiguration     m_Configuration;
    RADAR_PhasedArrayTx::SelectedAxisType          m_AxisType;
    RADAR_PhasedArrayTx::SelectedArray2DShapeType  m_Array2DShapeType;
    int    m_NumOfAnt1D;
    int    m_NumOfAnt2D_H;
    int    m_NumOfAnt2D_V;
    double m_ElementFactor;
    RADAR_PhasedArrayTx::SelectedSpaceType         m_SpaceType;
    RADAR_PhasedArrayTx::SelectedGridType          m_GridType;
    double m_D;
    double m_D_H;
    double m_D_V;
    SystemVueModelBuilder::Matrix<double> m_D_array;
    SystemVueModelBuilder::Matrix<double> m_D_H_array;
    SystemVueModelBuilder::Matrix<double> m_D_V_array;
    SystemVueModelBuilder::Matrix<int>    m_mask_array;
    RADAR_PhasedArrayTx::SelectedReliabilityType   m_ReliabilityType;
    double m_FailureProbability;
    double m_TargetTheta;
    double m_TargetPhi;
    RADAR_PhasedArrayTx::SelectedWindowType        m_WindowType;
    double m_KaiserWindowParameter;
    double m_Sidelobe_Levels;
    int    m_nBar;
    RADAR_PhasedArrayTx::SelectedYesorNo           m_IsPhaseShift;
    double m_BeamTheta;
    double m_BeamPhi;
    RADAR_PhasedArrayTx::SelectedYesorNo           m_QuantizationType;
    int    m_PhaseShifterBitwidth;
    RADAR_PhasedArrayTx::SelectedPhaseShiftType    m_PhaseShiftType;
    SystemVueModelBuilder::Matrix<double> m_DesiredPhaseShiftAngle;

    // ===== 算法状态 =====
    int m_NumChannels;

    // ===== TimeDrivenRun 累积 =====
    std::vector<EnvelopeSignal> m_inputAccumulator;
    std::queue<std::vector<EnvelopeSignal>> m_outputQueue;

    // ===== 仿真参数 =====
    SimuParameter simulator_param;
};

RegAlgo(RADAR_PhasedArrayTx_Block);

#endif // RADAR_PHASEDARRAYTX_BLOCK_H
