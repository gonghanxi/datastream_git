#ifndef REPEAT_BLOCK_H
#define REPEAT_BLOCK_H
#include "Repeat.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Repeat_Block : public Block
{
public:
    Repeat_Block(const std::string& name);
    ~Repeat_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<Repeat> m_Repeat;

    double m_BlockSize;
    double m_NumTimes;
};
RegAlgo(Repeat_Block);

#endif // REPEAT_BLOCK_H
