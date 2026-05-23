#ifndef SetSampleRateEnv_Block_H
#define SetSampleRateEnv_Block_H

#include "Block.h"
#include "SetSampleRateEnv.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API SetSampleRateEnv_Block : public SystemVueModelBuilder::Block
{
public:
	SetSampleRateEnv_Block(const std::string& name);
	~SetSampleRateEnv_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(double sampleRate);
	bool ValidateSampleRate();

	double m_sampleRate;
	std::unique_ptr<SetSampleRateEnv> m_setSampleRate;
};
RegAlgo(SetSampleRateEnv_Block);
#endif // SetSampleRateEnv_Block_H

