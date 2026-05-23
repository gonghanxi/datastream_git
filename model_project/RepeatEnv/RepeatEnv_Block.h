#ifndef REPEATENV_BLOCK_H
#define REPEATENV_BLOCK_H
#include "RepeatEnv.h"
#include "Block.h"
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API RepeatEnv_Block : public Block
{
public:
    RepeatEnv_Block(const std::string& name);
    ~RepeatEnv_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<RepeatEnv> m_Rep;

    double BlockSize;
    double NumTimes;
};
RegAlgo(RepeatEnv_Block);

#endif // REPEATENV_BLOCK_H
