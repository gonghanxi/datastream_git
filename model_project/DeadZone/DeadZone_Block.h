#ifndef DEADZONE_BLOCK_H
#define DEADZONE_BLOCK_H

#include "Block.h"
#include "DeadZone.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API DeadZone_Block : public SystemVueModelBuilder::Block
{
public:
    DeadZone_Block(const std::string& name);
    ~DeadZone_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<DeadZone> m_DeadZone;

    double m_K;
    double m_Low;
    double m_High;

    // 时间驱动缓冲
    std::vector<double> m_inputBuffer;
    std::queue<double>  m_outputQueue;
};

RegAlgo(DeadZone_Block);

#endif // DEADZONE_BLOCK_H
