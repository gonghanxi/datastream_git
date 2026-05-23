#ifndef RECIPROCAL_BLOCK_H
#define RECIPROCAL_BLOCK_H
#include "Reciprocal.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Reciprocal_Block : public Block
{
public:
    Reciprocal_Block(const std::string& name);
    ~Reciprocal_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    void SetParameters();
private:

    void SetDefaultParameters();

    std::unique_ptr<Reciprocal> m_Reciprocal;

    double m_MagLimit;
};
RegAlgo(Reciprocal_Block);

#endif // RECIPROCAL_BLOCK_H
