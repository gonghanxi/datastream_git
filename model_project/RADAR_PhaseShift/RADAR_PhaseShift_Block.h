#ifndef RADAR_PHASESHIFT_BLOCK_H
#define RADAR_PHASESHIFT_BLOCK_H

#include "RADAR_PhaseShift.h"
#include "Block.h"
#include "EnvelopeSignal.h"
#include <memory>
#include <vector>
#include <complex>
#include <deque>
#include <queue>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_PhaseShift_Block : public Block
{
public:
    RADAR_PhaseShift_Block(const std::string& name);
    ~RADAR_PhaseShift_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();

private:
    bool ModelSetup();
    void SetDefaultParameters();
    void UpdateCharacterizationFrequency();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<RADAR_PhaseShift> m_ps;

    // ========== 参数 ==========
    int NumOfAntx;
    int NumOfAnty;
    RADAR_PhaseShift::PhaseShiftTypeEnum Type;
    double Dx;
    double Dy;
    double Theta;
    double Phi;
    std::vector<double> DesiredPhaseShiftVec;

    // ========== 运行时状态 ==========
    int nAnt_;
    int outBusSize_;
    std::vector<double> phaseCacheRad_;
    double inputFc_;

    // ========== 时间驱动模式缓冲区 ==========
    std::deque<EnvelopeSignal> m_inputBuffer;                    // 输入累积缓冲区
    std::queue<std::vector<EnvelopeSignal>> m_outputQueue;       // 输出队列（每帧 nAnt_ 个样本）

    // ========== 相位表构建 ==========
    void buildPhaseTable_();
    double getThetaRad_();
    double getPhiRad_();
    double computePhaseRad_(int kx, int ky, double thetaRad, double phiRad) const;
    double getDesiredPhaseRad_(int index) const;

    // ========== 相位旋转 ==========
    std::complex<double> phaseRotator_(double phaseRad) const;

    // ========== 工具函数 ==========
    static double deg2rad(double x);
    static double clampFinite(double x, double fallback);

    // ========== 枚举解析 ==========
    static RADAR_PhaseShift::PhaseShiftTypeEnum ConvertStringToTypeEnum(const std::string& value);
    static std::vector<double> ParseDoubleArray(const std::string& str);
};

RegAlgo(RADAR_PhaseShift_Block);

#endif // RADAR_PHASESHIFT_BLOCK_H
