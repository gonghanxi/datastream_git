#ifndef SUB_BLOCK_H
#define SUB_BLOCK_H

#include "Block.h"
#include "Sub.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Sub_Block : public SystemVueModelBuilder::Block
{
public:
	Sub_Block(const std::string& name);
	~Sub_Block() = default;
	bool Setup() override;
	bool Run() override;
	bool Initialize() override;

private:
	void SetDefaultParamters();

	std::unique_ptr<Sub> m_sub;
};
RegAlgo(Sub_Block);
#endif // SUB_BLOCK_H
