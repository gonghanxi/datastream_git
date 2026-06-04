#ifndef RADAR_PHASEDARRAYRX_BLOCK_H
#define RADAR_PHASEDARRAYRX_BLOCK_H

#include "Block.h"
#include "RADAR_PhasedArrayRx.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_PhasedArrayRx_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_PhasedArrayRx_Block(const std::string& name);
    ~RADAR_PhasedArrayRx_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

    void SetDefaultParameters();
    void SetParameters();

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    // ConvertStringTo
    static RADAR_PhasedArrayRx::SelectedConfiguration     ConvertStringToConfiguration(const std::string& value);
    static RADAR_PhasedArrayRx::SelectedAxisType          ConvertStringToAxisType(const std::string& value);
    static RADAR_PhasedArrayRx::SelectedArray2DShapeType  ConvertStringToArray2DShapeType(const std::string& value);
    static RADAR_PhasedArrayRx::SelectedSpaceType         ConvertStringToSpaceType(const std::string& value);
    static RADAR_PhasedArrayRx::SelectedGridType          ConvertStringToGridType(const std::string& value);
    static RADAR_PhasedArrayRx::SelectedReliabilityType   ConvertStringToReliabilityType(const std::string& value);
    static RADAR_PhasedArrayRx::SelectedWindowType        ConvertStringToWindowType(const std::string& value);
    static RADAR_PhasedArrayRx::SelectedYesorNo           ConvertStringToYesorNo(const std::string& value);
    static RADAR_PhasedArrayRx::SelectedPhaseShiftType    ConvertStringToPhaseShiftType(const std::string& value);

    std::unique_ptr<RADAR_PhasedArrayRx> m_algo;

    // ===== 参数 =====
    RADAR_PhasedArrayRx::SelectedConfiguration     m_Configuration;
    RADAR_PhasedArrayRx::SelectedAxisType          m_AxisType;
    RADAR_PhasedArrayRx::SelectedArray2DShapeType  m_Array2DShapeType;
    int    m_NumOfAnt1D;
    int    m_NumOfAnt2D_H;
    int    m_NumOfAnt2D_V;
    double m_ElementFactor;
    RADAR_PhasedArrayRx::SelectedSpaceType         m_SpaceType;
    RADAR_PhasedArrayRx::SelectedGridType          m_GridType;
    double m_D;
    double m_D_H;
    double m_D_V;
    SystemVueModelBuilder::Matrix<double> m_D_array;
    SystemVueModelBuilder::Matrix<double> m_D_H_array;
    SystemVueModelBuilder::Matrix<double> m_D_V_array;
    SystemVueModelBuilder::Matrix<int>    m_mask_array;
    RADAR_PhasedArrayRx::SelectedReliabilityType   m_ReliabilityType;
    double m_FailureProbability;
    double m_TargetTheta;
    double m_TargetPhi;
    RADAR_PhasedArrayRx::SelectedWindowType        m_WindowType;
    double m_KaiserWindowParameter;
    double m_Sidelobe_Levels;
    int    m_nBar;
    RADAR_PhasedArrayRx::SelectedYesorNo           m_IsPhaseShift;
    double m_BeamTheta;
    double m_BeamPhi;
    RADAR_PhasedArrayRx::SelectedYesorNo           m_QuantizationType;
    int    m_PhaseShifterBitwidth;
    RADAR_PhasedArrayRx::SelectedPhaseShiftType    m_PhaseShiftType;
    SystemVueModelBuilder::Matrix<double> m_DesiredPhaseShiftAngle;

    // ===== 算法状态 =====
    int m_NumChannels;

    // ===== TimeDrivenRun 累积 =====
    std::vector<EnvelopeSignal> m_inputAccumulator;
    std::queue<std::vector<EnvelopeSignal>> m_outputQueue;

    // ===== 仿真参数 =====
    SimuParameter simulator_param;
};

RegAlgo(RADAR_PhasedArrayRx_Block);

#endif // RADAR_PHASEDARRAYRX_BLOCK_H
