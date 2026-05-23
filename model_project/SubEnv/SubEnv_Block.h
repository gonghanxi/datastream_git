#ifndef SUBENV_BLOCK_H
#define SUBENV_BLOCK_H

#include "SubEnv.h"
#include "Block.h"

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API SubEnv_Block : public SystemVueModelBuilder::Block
{
public:
	SubEnv_Block(const std::string& name);
	~SubEnv_Block() = default;

	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

	void SetParameters(double userDefinedFc = 100e6, SubEnv::SelectedFcOut fcOut = SubEnv::center);

private:
	SubEnv::SelectedFcOut ConvertStringToSelectedFcOut(const std::string& value);
	unsigned long long GetCount() const { return m_firingCount; }

	void SetDefaultParameters();
	void PropagateCharacterizationFrequency();

	std::unique_ptr<SubEnv> m_subEnv;

	SubEnv::SelectedFcOut m_FcOut;
	double m_UserDefinedFc;

	SimuParameter simulator_param;
	double fcOut;
	double fc, fcmax, fcmin, fcmean;
	unsigned long long m_firingCount = 0;
};

RegAlgo(SubEnv_Block);

#endif // SUBENV_BLOCK_H
