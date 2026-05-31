#ifndef UPSAMPLECX_BLOCK_H
#define UPSAMPLECX_BLOCK_H

#include "Block.h"
#include "UpSampleCx.h"
#include <queue>
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API UpSampleCx_Block : public SystemVueModelBuilder::Block
{
public:
	UpSampleCx_Block(const std::string& name);
	~UpSampleCx_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	UpSampleCx::ModeEnum ConvertStringToModeEnum(const std::string& value);
	void SetDefaultParamters();
	void SetParameters(int factor, UpSampleCx::ModeEnum mode, int phase);

	bool ValidatePhase() const;

	int m_factor;
	UpSampleCx::ModeEnum m_mode;
	int m_phase;
	bool m_isInRun;

	std::unique_ptr<UpSampleCx> m_upSampleCx;

    bool DataStreamRun();
    bool TimeDrivenRun();
    // ========== 时间驱动缓冲队列 ==========
    std::vector<std::complex<double>> m_inputBuffer;
    std::queue<std::complex<double>> m_outputQueue;
    std::complex<double> m_lastOutput;
    int m_inputCount;
    int m_outputCount;
};
RegAlgo(UpSampleCx_Block);
#endif // UPSAMPLECX_BLOCK_H
