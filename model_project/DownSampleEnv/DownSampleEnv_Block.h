#ifndef DOWNSAMPLEENV_BLOCK_H
#define DOWNSAMPLEENV_BLOCK_H

#include "Block.h"
#include "DownSampleEnv.h"
#include <queue>
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API DownSampleEnv_Block : public SystemVueModelBuilder::Block
{
public:
	DownSampleEnv_Block(const std::string& name);
	~DownSampleEnv_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(int factor, int phase);
	void UpdateCharacterizationFrequency();

	int m_factor;
	int m_phase;

	std::unique_ptr<DownSampleEnv> m_downSampleEnv;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<EnvelopeSignal> m_inputBuffer;   // 多输入累积缓冲区
    std::queue<EnvelopeSignal> m_outputQueue;
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};

RegAlgo(DownSampleEnv_Block);

#endif // DOWNSAMPLEENV_BLOCK_H
