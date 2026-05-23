#ifndef CONSTCX_BLOCK_H
#define CONSTCX_BLOCK_H

#include "Block.h"
#include "ConstCx.h"
#include "DataTypesAndParsers.h"

#include <complex>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API ConstCx_Block : public SystemVueModelBuilder::Block
{
public:
	ConstCx_Block(const std::string& name);
	~ConstCx_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void SetParameters();
	ConstCx::SelectedShowAdvancedParams ConvertStringToShowAdvancedParams(const std::string& value);
	ConstCx::SelectedSampleRateOption ConvertStringToSampleRateOption(const std::string& value);
	std::complex<double> ParseComplexValue(const std::string& value);

	std::unique_ptr<ConstCx> m_constCx;

	std::complex<double> m_value;
	ConstCx::SelectedShowAdvancedParams m_showAdvancedParams;
	ConstCx::SelectedSampleRateOption m_sampleRateOption;
	double m_sampleRate;
	int m_initialDelay;
};

RegAlgo(ConstCx_Block);

#endif // CONSTCX_BLOCK_H
