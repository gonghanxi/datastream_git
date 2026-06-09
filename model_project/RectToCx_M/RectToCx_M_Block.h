#ifndef RECTTOCX_M_BLOCK_H
#define RECTTOCX_M_BLOCK_H

#include "Block.h"
#include "RectToCx_M.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RectToCx_M_Block : public SystemVueModelBuilder::Block
{
public:
    RectToCx_M_Block(const std::string& name);
    ~RectToCx_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<RectToCx_M> m_RectToCx_M;

    // ===== TimeDrivenRun 输入缓冲(vector) + 输出队列 =====
    std::vector<SystemVueModelBuilder::Matrix<double>>               m_realBuffer;
    std::vector<SystemVueModelBuilder::Matrix<double>>               m_imagBuffer;
    std::queue<SystemVueModelBuilder::Matrix<std::complex<double>>>  m_outputQueue;
};

RegAlgo(RectToCx_M_Block);

#endif // RECTTOCX_M_BLOCK_H
