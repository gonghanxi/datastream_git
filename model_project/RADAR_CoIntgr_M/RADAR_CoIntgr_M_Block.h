#ifndef RADAR_COINTGR_M_BLOCK_H
#define RADAR_COINTGR_M_BLOCK_H

#include "Block.h"
#include "RADAR_CoIntgr_M.h"

#include <complex>
#include <memory>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_CoIntgr_M_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_CoIntgr_M_Block(const std::string& name);
    ~RADAR_CoIntgr_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<RADAR_CoIntgr_M> m_algo;

    // ===== 参数 =====
    int m_NumOfPulse;

    // ===== TimeDrivenRun 累积 =====
    std::vector<SystemVueModelBuilder::DComplexMatrix> m_inputBuffer;
    std::queue<SystemVueModelBuilder::DComplexMatrix> m_outputQueue;
};

RegAlgo(RADAR_CoIntgr_M_Block);

#endif // RADAR_COINTGR_M_BLOCK_H
