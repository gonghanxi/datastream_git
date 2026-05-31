#ifndef REVERSECX_BLOCK_H
#define REVERSECX_BLOCK_H

#include "Block.h"
#include "ReverseCx.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API ReverseCx_Block : public SystemVueModelBuilder::Block
{
public:
	ReverseCx_Block(const std::string& name);
	~ReverseCx_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(int n);

	int m_n;
	std::unique_ptr<ReverseCx> m_reverseCx;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;
    std::queue<std::complex<double>> m_outputQueue;
    std::complex<double> m_lastOutput;
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(ReverseCx_Block);

#endif // REVERSECX_BLOCK_H
