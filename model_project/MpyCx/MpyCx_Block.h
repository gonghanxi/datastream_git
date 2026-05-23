#ifndef MPYCX_BLOCK_H
#define MPYCX_BLOCK_H

#include "Block.h"
#include "MpyCx.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API MpyCx_Block : public SystemVueModelBuilder::Block
{
public:
	MpyCx_Block(const std::string& name);
	~MpyCx_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();

	std::unique_ptr<MpyCx> m_mpyCx;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<std::complex<double>>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<std::complex<double>> m_outputQueue;    // 输出分发队列
    std::complex<double> m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(MpyCx_Block);

#endif // MPYCX_BLOCK_H
