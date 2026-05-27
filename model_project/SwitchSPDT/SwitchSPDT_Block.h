#ifndef SWITCHSPDT_BLOCK_H
#define SWITCHSPDT_BLOCK_H

#include "Block.h"
#include "SwitchSPDT.h"

#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API SwitchSPDT_Block : public SystemVueModelBuilder::Block
{
public:
    SwitchSPDT_Block(const std::string& name);
    ~SwitchSPDT_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool DataStreamRun();

    // ---- algorithm instance ----
    std::unique_ptr<SwitchSPDT> m_algo;

    // ---- parameters ----
    double m_Loss1;
    double m_Isolation1;
    double m_Loss2;
    double m_Isolation2;
    double m_VThreshold;
    double m_TOn1;
    double m_TOff1;
    double m_TOn2;
    double m_TOff2;

    // ---- internal state (Block 层自行维护) ----
    bool   m_SwitchState;
    double m_Ts;

    // ---- simulator parameters ----
    SimuParameter m_simuParam;
    int           m_sampleCount;
    double        m_sampleRate;
};

RegAlgo(SwitchSPDT_Block);

#endif // SWITCHSPDT_BLOCK_H
