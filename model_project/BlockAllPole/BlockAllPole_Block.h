#ifndef BLOCKALLPOLE_BLOCK_H
#define BLOCKALLPOLE_BLOCK_H

#include "Block.h"
#include "BlockAllPole.h"
#include <queue>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API BlockAllPole_Block : public SystemVueModelBuilder::Block
{
public:
	BlockAllPole_Block(const std::string& name);
	~BlockAllPole_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	bool ValidateParameters();

	std::unique_ptr<SystemVueModelBuilder::BlockAllPole> m_blockAllPole;

	int m_blockSize;
	int m_order;
	std::vector<double> m_taps;
	std::vector<double> m_delayLine;

    bool DataStreamRun();
    bool TimeDrivenRun();

    //
    size_t m_maxBlock = 0;
    // ========== 时间驱动缓冲队列 ==========
    std::vector<double> m_signalInBuffer;   // 多输入累积缓冲区
    std::vector<double> m_coefsInBuffer;
    std::queue<double> m_outputQueue;    // 输出分发队列
    double m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(BlockAllPole_Block);

#endif // BLOCKALLPOLE_BLOCK_H
