#ifndef REPEATCX_BLOCK_H
#define REPEATCX_BLOCK_H
#include "RepeatCx.h"
#include "Block.h"
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RepeatCx_Block : public Block
{
public:
    RepeatCx_Block(const std::string& name);
    ~RepeatCx_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<RepeatCx> m_Rep;

    double BlockSize;
    double NumTimes;
};
RegAlgo(RepeatCx_Block);

#endif // REPEATCX_BLOCK_H
