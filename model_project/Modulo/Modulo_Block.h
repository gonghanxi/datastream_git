#ifndef MODULO_BLOCK_H
#define MODULO_BLOCK_H

#include "Modulo.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Modulo_Block : public Block
{
public:
    Modulo_Block(const std::string& name);
    ~Modulo_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    void SetParameters();
private:

    void SetDefaultParameters();

    std::unique_ptr<Modulo> m_Modulo;

    double m_moduloValue;
};
RegAlgo(Modulo_Block);

#endif // MODULO_BLOCK_H
