#ifndef RADAR_NONCOINTGR_M_BLOCK_H
#define RADAR_NONCOINTGR_M_BLOCK_H

#include "Block.h"
#include "RADAR_NonCoIntgr_M.h"

#include <memory>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_NonCoIntgr_M_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_NonCoIntgr_M_Block(const std::string& name);
    ~RADAR_NonCoIntgr_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<RADAR_NonCoIntgr_M> m_algo;

    // ===== 参数 =====
    int m_Number;

    // ===== TimeDrivenRun =====
    std::vector<SystemVueModelBuilder::DComplexMatrix> m_inputBuffer;
    std::queue<SystemVueModelBuilder::Matrix<double>>   m_outputQueue;
};

RegAlgo(RADAR_NonCoIntgr_M_Block);

#endif // RADAR_NONCOINTGR_M_BLOCK_H
