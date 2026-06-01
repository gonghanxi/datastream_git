#ifndef CHOPVAROFFSET_BLOCK_H
#define CHOPVAROFFSET_BLOCK_H

#include "Block.h"
#include "ChopVarOffset.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API ChopVarOffset_Block : public SystemVueModelBuilder::Block
{
public:
    ChopVarOffset_Block(const std::string& name);
    ~ChopVarOffset_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<ChopVarOffset> m_ChopVarOffset;

    int m_nRead;
    int m_nWrite;

    // 时间驱动缓冲
    std::vector<double> m_inputBuffer;
    std::vector<int>    m_offsetBuffer;
    std::queue<double>  m_outputQueue;
};

RegAlgo(ChopVarOffset_Block);

#endif // CHOPVAROFFSET_BLOCK_H
