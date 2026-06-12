#ifndef RADAR_CICINTERP_BLOCK_H
#define RADAR_CICINTERP_BLOCK_H

#include "Block.h"
#include "RADAR_CICInterp.h"

#include <complex>
#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_CICInterp_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_CICInterp_Block(const std::string& name);
    ~RADAR_CICInterp_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool DataStreamRun();
    bool TimeDrivenRun();

    // ---- algorithm instance ----
    std::unique_ptr<RADAR_CICInterp> m_algo;

    // ---- parameters ----
    int  m_Order;
    int  m_Ratio;
    int  m_DiffDelay;
    int  m_Phase;
    using Cx = std::complex<double>;
    Cx   m_Fill;

    // ---- algorithm state (inlined from RADAR_CICInterp private members) ----
    int    m_cachedOrder;
    int    m_cachedRatio;
    int    m_cachedDiffDelay;
    int    m_cachedPhase;
    double m_gainScale;

    std::vector<std::vector<Cx>> m_combDelay;   // order_ × diffDelay_ 延迟线
    std::vector<int>             m_combWriteIndex;
    std::vector<Cx>              m_integratorState;

    // ---- inlined algorithm methods ----
    bool validateAndPrepare();
    void resetStates();
    void updateGainScale();
    Cx   runCombStages(const Cx& x);
    Cx   runIntegratorStages(const Cx& x);

    // ---- TimeDrivenRun buffers ----
    std::vector<Cx> m_inputBuffer;
    std::queue<Cx>  m_outputQueue;
};

RegAlgo(RADAR_CICInterp_Block);

#endif // RADAR_CICINTERP_BLOCK_H
