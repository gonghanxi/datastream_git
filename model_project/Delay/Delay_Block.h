#ifndef DELAY_BLOCK_H
#define DELAY_BLOCK_H

#include "Block.h"
#include "Delay.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Delay_Block : public SystemVueModelBuilder::Block
{
public:
	Delay_Block(const std::string& name);
	~Delay_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	Delay::OutputTimingEnum ConvertStringToOutputTimingEnum(const std::string& value);
	void SetDefaultParamters();
	void SetParameters(int n, Delay::OutputTimingEnum timing);
	void ResetState();

	int m_n;
	Delay::OutputTimingEnum m_outputTiming;
	std::unique_ptr<Delay> m_delay;

	std::vector<double> m_buf;
	std::size_t m_head;
	int m_warmup;
};
RegAlgo(Delay_Block);
#endif // DELAY_BLOCK_H
