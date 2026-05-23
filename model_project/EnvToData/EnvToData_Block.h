#ifndef ENVTODATA_BLOCK_H
#define ENVTODATA_BLOCK_H

#include "Block.h"
#include "EnvToData.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API EnvToData_Block : public SystemVueModelBuilder::Block
{
public:
	EnvToData_Block(const std::string& name);
	~EnvToData_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();
	void UpdateCharacterizationFrequency();

	std::unique_ptr<EnvToData> m_envToData;
};
RegAlgo(EnvToData_Block);
#endif // ENVTODATA_BLOCK_H
