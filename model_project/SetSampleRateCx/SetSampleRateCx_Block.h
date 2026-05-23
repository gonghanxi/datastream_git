#ifndef SetSampleRateCx_Block_H
#define SetSampleRateCx_Block_H

#include "Block.h"
#include "SetSampleRateCx.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API SetSampleRateCx_Block : public SystemVueModelBuilder::Block
{
public:
	SetSampleRateCx_Block(const std::string& name);
	~SetSampleRateCx_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters(double sampleRate);
	bool ValidateSampleRate();

	double m_sampleRate;
	std::unique_ptr<SetSampleRateCx> m_setSampleRate;
};
RegAlgo(SetSampleRateCx_Block);
#endif // SetSampleRateCx_Block_H

