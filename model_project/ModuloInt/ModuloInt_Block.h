#ifndef MODULOINT_BLOCK_H
#define MODULOINT_BLOCK_H

#include "ModuloInt.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API ModuloInt_Block : public Block
{
public:
    ModuloInt_Block(const std::string& name);
    ~ModuloInt_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    void SetParameters();
private:

    void SetDefaultParameters();

    std::unique_ptr<ModuloInt> m_ModuloInt;

    int m_moduloValue;
};
RegAlgo(ModuloInt_Block);

#endif // MODULOINT_BLOCK_H
