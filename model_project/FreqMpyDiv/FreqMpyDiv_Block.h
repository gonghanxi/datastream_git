#ifndef FREQMPYDIV_BLOCK_H
#define FREQMPYDIV_BLOCK_H

#include "Block.h"
#include "FreqMpyDiv.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API FreqMpyDiv_Block : public SystemVueModelBuilder::Block
{
public:
	FreqMpyDiv_Block(const std::string& name);
	~FreqMpyDiv_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	FreqMpyDiv::MultDivEnum ConvertStringToMultDivEnum(const std::string& value);
	FreqMpyDiv::OperatorTypeEnum ConvertStringToOperatorTypeEnum(const std::string& value);
	void SetDefaultParamters();
	void SetParameters();
	void UpdateCharacterizationFrequency();

	FreqMpyDiv::MultDivEnum m_multDiv;
	FreqMpyDiv::OperatorTypeEnum m_operatorType;
	double m_nominalX;
	double m_maxX;
	double m_minX;

	double m_fcIn;
	double m_fcOut;

	unsigned long long GetCount() const { return m_iFiringCount; }
	void Advance() { ++m_iFiringCount; }
	unsigned long long m_iFiringCount = 0;

	std::unique_ptr<FreqMpyDiv> m_freqMpyDiv;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<EnvelopeSignal> m_inputBuffer;   // 多输入累积缓冲区
    std::vector<EnvelopeSignal> m_controlBuffer;
    std::queue<EnvelopeSignal> m_outputQueue;
    EnvelopeSignal m_lastOutput;                 // 上次输出值（用于保持）
    int m_inputCount;                    // 当前已累积输入数
    int m_outputCount;                   // 当前已分发输出数
};
RegAlgo(FreqMpyDiv_Block);
#endif // FREQMPYDIV_BLOCK_H
