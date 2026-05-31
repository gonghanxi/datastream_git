#ifndef REVERSEINT_BLOCK_H
#define REVERSEINT_BLOCK_H

#include "Block.h"
#include "ReverseInt.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API ReverseInt_Block : public SystemVueModelBuilder::Block
{
public:
	ReverseInt_Block(const std::string& name);
	~ReverseInt_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(int n);

	int m_n;
	std::unique_ptr<ReverseInt> m_reverseInt;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<int> m_inputBuffer;
    std::queue<int> m_outputQueue;
    int m_lastOutput;
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(ReverseInt_Block);

#endif // REVERSEINT_BLOCK_H
