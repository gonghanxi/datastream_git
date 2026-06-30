#ifndef RADAR_TX_SYNTHESIS_BLOCK_H
#define RADAR_TX_SYNTHESIS_BLOCK_H

#include "RADAR_Tx_Synthesis.h"
#include "Block.h"
#include "EnvelopeSignal.h"
#include <memory>
#include <vector>
#include <complex>
#include <deque>
#include <queue>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_Tx_Synthesis_Block : public Block
{
public:
    RADAR_Tx_Synthesis_Block(const std::string& name);
    ~RADAR_Tx_Synthesis_Block() = default;

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

    // 仅用于端口注册的算法实例
    std::unique_ptr<RADAR_Tx_Synthesis> m_algo;

    // ========== 参数 ==========
    int NumOfAntx;
    int NumOfAnty;
    RADAR_Tx_Synthesis::PhaseShiftTypeEnum Type;
    double Dx;
    double Dy;
    double Theta;
    double Phi;
    std::vector<double> DesiredPhaseShiftVec;

    // ========== 运行时状态 ==========
    int nAnt_;
    int inputBusSize_;
    std::vector<double> phaseCacheRad_;
    double inputFc_;

    // ========== 时间驱动模式缓冲区 ==========
    // 输入：每帧 nAnt_ 个 envelope 样本（bus 各通道展开）
    std::deque<std::vector<EnvelopeSignal>> m_inputBuffer;
    // 输出：每帧 1 个 envelope 样本
    std::queue<EnvelopeSignal> m_outputQueue;

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
    static RADAR_Tx_Synthesis::PhaseShiftTypeEnum ConvertStringToTypeEnum(const std::string& value);
    static std::vector<double> ParseDoubleArray(const std::string& str);
};

RegAlgo(RADAR_Tx_Synthesis_Block);

#endif // RADAR_TX_SYNTHESIS_BLOCK_H
