#ifndef REPEATINT_BLOCK_H
#define REPEATINT_BLOCK_H
#include "RepeatInt.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API RepeatInt_Block : public Block
{
public:
    RepeatInt_Block(const std::string& name);
    ~RepeatInt_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<RepeatInt> m_Rep;

    double BlockSize;
    double NumTimes;
};
RegAlgo(RepeatInt_Block);

#endif // REPEATINT_BLOCK_H
