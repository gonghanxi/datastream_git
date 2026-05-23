#ifndef COMMUTATORINT_BLOCK_H
#define COMMUTATORINT_BLOCK_H

#include "CommutatorInt.h"
#include "Block.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API CommutatorInt_Block : public SystemVueModelBuilder::Block
{
public:
	CommutatorInt_Block(const std::string& name);
	~CommutatorInt_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters();

	std::unique_ptr<CommutatorInt> m_commutatorInt;

	int m_blockSize = 1;
	size_t m_iBlockSize = 1U;

    bool ModelSetup();
    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::map<BufferReader*, std::vector<int>> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<int> m_outputQueue;    // 输出分发队列
    int m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(CommutatorInt_Block);

#endif // COMMUTATORINT_BLOCK_H
