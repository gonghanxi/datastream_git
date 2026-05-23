#ifndef LOGIC_BLOCK_H
#define LOGIC_BLOCK_H
#include "Logic.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Logic_Block : public Block
{
public:
    Logic_Block(const std::string& name);
    ~Logic_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    Logic::SelectedLogicOperation ConvertStringToSelectedLogicOperation(const std::string& value);

    void SetDefaultParameters();

    std::unique_ptr<Logic> m_Logic;

    Logic::SelectedLogicOperation	m_LogicOperation;
};
RegAlgo(Logic_Block);
#endif // LOGIC_BLOCK_H
