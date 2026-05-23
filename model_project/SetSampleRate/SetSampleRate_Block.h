#ifndef SETSAMPLERATE_BLOCK_H
#define SETSAMPLERATE_BLOCK_H

#include "Block.h"
#include "SetSampleRate.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API SetSampleRate_Block : public SystemVueModelBuilder::Block
{
public:
	SetSampleRate_Block(const std::string& name);
	~SetSampleRate_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(double sampleRate);
	bool ValidateSampleRate();

	double m_sampleRate;
	std::unique_ptr<SetSampleRate> m_setSampleRate;
};
RegAlgo(SetSampleRate_Block);
#endif // SETSAMPLERATE_BLOCK_H
