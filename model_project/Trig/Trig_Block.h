#ifndef TRIG_BLOCK_H
#define TRIG_BLOCK_H
#include "Trig.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Trig_Block : public Block
{
public:
    Trig_Block(const std::string& name);
    ~Trig_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    Trig::SelectedFunctionType ConvertStringToSelectedFunctionType(const std::string& value);

    void SetDefaultParameters();

    std::unique_ptr<Trig> m_Trig;

    Trig::SelectedFunctionType	m_FunctionType;
};
RegAlgo(Trig_Block);

#endif // TRIG_BLOCK_H
