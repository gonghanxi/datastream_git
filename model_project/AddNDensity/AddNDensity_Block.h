#ifndef ADDNDENSITY_BLOCK_H
#define ADDNDENSITY_BLOCK_H
#include "AddNDensity.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API AddNDensity_Block : public Block
{
public:
    AddNDensity_Block(const std::string& name);
    ~AddNDensity_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    void SetParameters();
private:
    void SetDefaultParamters();

    std::unique_ptr<AddNDensity> m_addn;

    bool DataStreamRun();

    double NDensity;
    double RefR;
};
RegAlgo(AddNDensity_Block);
#endif // ADDNDENSITY_BLOCK_H
