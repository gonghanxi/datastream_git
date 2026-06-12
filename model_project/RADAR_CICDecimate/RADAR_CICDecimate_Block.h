#ifndef RADAR_CICDECIMATE_BLOCK_H
#define RADAR_CICDECIMATE_BLOCK_H

#include "Block.h"
#include "RADAR_CICDecimate.h"

#include <complex>
#include <deque>
#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_CICDecimate_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_CICDecimate_Block(const std::string& name);
    ~RADAR_CICDecimate_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool DataStreamRun();
    bool TimeDrivenRun();

    // ---- algorithm instance ----
    std::unique_ptr<RADAR_CICDecimate> m_algo;

    // ---- parameters ----
    int m_Order;
    int m_Ratio;
    int m_DiffDelay;
    int m_Phase;

    // ---- algorithm state (inlined from RADAR_CICDecimate private members) ----
    using Cx = std::complex<double>;

    int    m_cachedOrder;
    int    m_cachedRatio;
    int    m_cachedDiffDelay;
    int    m_cachedPhase;
    double m_gainScale;

    std::vector<Cx>              m_integratorState;
    std::vector<std::deque<Cx>>  m_combDelay;

    // ---- inlined algorithm methods ----
    bool   validateAndPrepare();
    void   resetStates();
    Cx     runIntegratorStages(const Cx& x);
    Cx     runCombStages(const Cx& x);
    double computeGainScale() const;
    static int clampInt(int x, int lo, int hi);

    // ---- TimeDrivenRun buffers ----
    std::vector<Cx>     m_inputBuffer;
    std::queue<Cx>      m_outputQueue;
};

RegAlgo(RADAR_CICDecimate_Block);

#endif // RADAR_CICDECIMATE_BLOCK_H
