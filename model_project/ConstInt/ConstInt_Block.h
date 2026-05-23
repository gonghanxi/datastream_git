#ifndef CONSTINT_BLOCK_H
#define CONSTINT_BLOCK_H

#include "Block.h"
#include "ConstInt.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API ConstInt_Block : public SystemVueModelBuilder::Block
{
public:
	ConstInt_Block(const std::string& name);
	~ConstInt_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters();
	ConstInt::SelectedShowAdvancedParams ConvertStringToShowAdvancedParams(const std::string& value);
	ConstInt::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);

	std::unique_ptr<ConstInt> m_constInt;

	int m_value;
	ConstInt::SelectedShowAdvancedParams m_showAdvancedParams;
	ConstInt::SelectedSampleRateOption m_sampleRateOption;
	double m_sampleRate;
	int m_initialDelay;
};

RegAlgo(ConstInt_Block);

#endif // CONSTINT_BLOCK_H
