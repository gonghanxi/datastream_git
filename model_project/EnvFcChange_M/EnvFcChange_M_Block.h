#ifndef ENVFCCHANGE_M_BLOCK_H
#define ENVFCCHANGE_M_BLOCK_H

#include "Block.h"
#include "EnvFcChange_M.h"

#include <complex>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API EnvFcChange_M_Block : public Block
{
public:
    EnvFcChange_M_Block(const std::string& name);
    ~EnvFcChange_M_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    void resetLpfStateIfNeeded(size_t numElements);
    double getEffectiveBandwidth() const;

    std::unique_ptr<EnvFcChange_M> m_EnvFcChange_M;

    double m_OutputFc;
    double m_Bandwidth;
    double m_fc_in;
    double m_fc_out;

    SimuParameter m_simulatorParam;

    // LPF 状态（fc_in=0 且 fc_out>0 时使用）
    std::vector<std::complex<double>> m_lpfState;
    bool   m_lpfInitialized;
    size_t m_lpfNumElements;

    // 时间驱动缓冲
    std::vector<SystemVueModelBuilder::EnvelopeMatrix> m_inputBuffer;
    std::queue<SystemVueModelBuilder::EnvelopeMatrix>  m_outputQueue;
};
RegAlgo(EnvFcChange_M_Block);
#endif // ENVFCCHANGE_M_BLOCK_H
