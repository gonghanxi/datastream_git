#ifndef SUBCX_BLOCK_H
#define SUBCX_BLOCK_H

#include "Block.h"
#include "SubCx.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API SubCx_Block : public SystemVueModelBuilder::Block
{
public:
    SubCx_Block(const std::string& name);
    ~SubCx_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParamters();

    std::unique_ptr<SubCx> m_subCx;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_posBuffer;
    std::map<BufferReader*, std::vector<std::complex<double>>> m_negBuffer;
    std::queue<std::complex<double>> m_outputQueue;
    std::complex<double> m_lastOutput;
    int m_inputCount;
    int m_outputCount;
};
RegAlgo(SubCx_Block);
#endif // SUBCX_BLOCK_H
