#ifndef RADAR_MTI_M_BLOCK_H
#define RADAR_MTI_M_BLOCK_H

#include "Block.h"
#include "RADAR_MTI_M.h"

#include <memory>
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RADAR_MTI_M_Block : public SystemVueModelBuilder::Block
{
public:
    RADAR_MTI_M_Block(const std::string& name);
    ~RADAR_MTI_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    // ===== 字符串转换 =====
    RADAR_MTI_M::SelectedMTI_Type ConvertStringToMTIType(const std::string& value);

    std::unique_ptr<RADAR_MTI_M> m_algo;

    // ===== 参数 =====
    RADAR_MTI_M::SelectedMTI_Type m_MTI_Type;

    // ===== TimeDrivenRun =====
    std::vector<SystemVueModelBuilder::DComplexMatrix> m_inputBuffer;
    std::queue<SystemVueModelBuilder::DComplexMatrix>  m_outputQueue;
};

RegAlgo(RADAR_MTI_M_Block);

#endif // RADAR_MTI_M_BLOCK_H
