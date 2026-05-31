#ifndef REVERSEENV_BLOCK_H
#define REVERSEENV_BLOCK_H

#include "Block.h"
#include "ReverseEnv.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API ReverseEnv_Block : public SystemVueModelBuilder::Block
{
public:
	ReverseEnv_Block(const std::string& name);
	~ReverseEnv_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(int n);

	int m_n;
	std::unique_ptr<ReverseEnv> m_reverseEnv;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<EnvelopeSignal> m_inputBuffer;
    std::queue<EnvelopeSignal> m_outputQueue;
    EnvelopeSignal m_lastOutput;
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(ReverseEnv_Block);

#endif // REVERSEENV_BLOCK_H
