#ifndef TRAINER_BLOCK_H
#define TRAINER_BLOCK_H

#include "Block.h"
#include "Trainer.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Trainer_Block : public SystemVueModelBuilder::Block
{
public:
    Trainer_Block(const std::string& name);
    ~Trainer_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<Trainer> m_Trainer;

    int m_TrainLength;
    int m_Count;

    // 时间驱动缓冲
    std::vector<double> m_trainBuffer;
    std::vector<double> m_decisionBuffer;
    std::queue<double>  m_outputQueue;
};

RegAlgo(Trainer_Block);

#endif // TRAINER_BLOCK_H
