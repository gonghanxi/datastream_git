#ifndef SWITCHSPST_BLOCK_H
#define SWITCHSPST_BLOCK_H

#include "Block.h"
#include "SwitchSPST.h"

#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API SwitchSPST_Block : public SystemVueModelBuilder::Block
{
public:
    SwitchSPST_Block(const std::string& name);
    ~SwitchSPST_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool DataStreamRun();

    // ---- algorithm instance ----
    std::unique_ptr<SwitchSPST> m_algo;

    // ---- parameters ----
    double m_Loss;
    double m_Isolation;
    double m_VThreshold;
    double m_TOn;
    double m_TOff;

    // ---- internal state (Block 层自行维护) ----
    bool   m_SwitchState;
    double m_Ts;

    // ---- simulator parameters ----
    SimuParameter m_simuParam;
    int           m_sampleCount;
    double        m_sampleRate;
};

RegAlgo(SwitchSPST_Block);

#endif // SWITCHSPST_BLOCK_H
