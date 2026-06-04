#ifndef PATTMATCH_BLOCK_H
#define PATTMATCH_BLOCK_H

#include "Block.h"
#include "PattMatch.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API PattMatch_Block : public SystemVueModelBuilder::Block
{
public:
    PattMatch_Block(const std::string& name);
    ~PattMatch_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

    void SetParameters();

private:
    void SetDefaultParameters();
    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<PattMatch> m_PattMatch;

    // m_index 原算法为 int 标量，需用 CircularBuffer 做端口
    SystemVueModelBuilder::CircularBuffer<int> m_indexPort;

    int m_tempSize;
    int m_winSize;

    // 时间驱动缓冲
    std::vector<double> m_templBuffer;
    std::vector<double> m_windowBuffer;
    std::queue<int>     m_indexQueue;
    std::queue<std::vector<double>> m_valuesQueue;
};

RegAlgo(PattMatch_Block);
#endif // PATTMATCH_BLOCK_H
