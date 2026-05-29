#ifndef ENVFCCHANGE_M_BLOCK_H
#define ENVFCCHANGE_M_BLOCK_H

#include "Block.h"
#include "EnvFcChange_M.h"

#include <complex>
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

private:
    void SetDefaultParamters();
    void SetParameters();
    void UpdateCharacterizationFrequency();

    // LPF 辅助（参考原算法 input fc=0 分支）
    void resetLpfStateIfNeeded(size_t numElements);
    double getEffectiveBandwidth() const;
    double getInputTimeStep() const;

    double m_OutputFc;
    double m_Bandwidth;

    // LPF 状态（input fc=0 时使用）
    std::vector<std::complex<double>> m_lpfState;
    bool m_lpfInitialized;
    size_t m_lpfNumElements;

    SimuParameter simulator_param;

    std::unique_ptr<EnvFcChange_M> m_EnvFcChange_M;
};
RegAlgo(EnvFcChange_M_Block);
#endif // ENVFCCHANGE_M_BLOCK_H
