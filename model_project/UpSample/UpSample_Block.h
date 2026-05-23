#ifndef UPSAMPLE_BLOCK_H
#define UPSAMPLE_BLOCK_H

#include "Block.h"
#include "UpSample.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API UpSample_Block : public SystemVueModelBuilder::Block
{
public:
	UpSample_Block(const std::string& name);
	~UpSample_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	UpSample::ModeEnum ConvertStringToModeEnum(const std::string& value);
	void SetDefaultParamters();
	void SetParameters(int factor, UpSample::ModeEnum mode, int phase);

	int m_factor;
	UpSample::ModeEnum m_mode;
	int m_phase;

	std::unique_ptr<UpSample> m_upSample;
};
RegAlgo(UpSample_Block);
#endif // UPSAMPLE_BLOCK_H
