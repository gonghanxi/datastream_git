#ifndef SetSampleRateInt_Block_H
#define SetSampleRateInt_Block_H

#include "Block.h"
#include "SetSampleRateInt.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API SetSampleRateInt_Block : public SystemVueModelBuilder::Block
{
public:
	SetSampleRateInt_Block(const std::string& name);
	~SetSampleRateInt_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(double sampleRate);
	bool ValidateSampleRate();

	double m_sampleRate;
	std::unique_ptr<SetSampleRateInt> m_setSampleRate;
};
RegAlgo(SetSampleRateInt_Block);
#endif // SetSampleRateInt_Block_H

