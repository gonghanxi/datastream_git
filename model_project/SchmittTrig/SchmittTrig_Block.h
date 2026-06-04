#ifndef SCHMITTTRIG_BLOCK_H
#define SCHMITTTRIG_BLOCK_H

#include "Block.h"
#include "SchmittTrig.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API SchmittTrig_Block : public SystemVueModelBuilder::Block
{
public:
    SchmittTrig_Block(const std::string& name);
    ~SchmittTrig_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<SchmittTrig> m_SchmittTrig;

    double m_ILow;
    double m_IHigh;
    double m_OLow;
    double m_OHigh;
    bool   m_TrigStatus;

    // 时间驱动缓冲
    std::vector<double> m_inputBuffer;
    std::queue<double>  m_outputQueue;
};

RegAlgo(SchmittTrig_Block);

#endif // SCHMITTTRIG_BLOCK_H
