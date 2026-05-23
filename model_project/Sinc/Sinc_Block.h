#ifndef SINC_BLOCK_H
#define SINC_BLOCK_H
#include "Sinc.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Sinc_Block : public Block
{
public:
    Sinc_Block(const std::string& name);
    ~Sinc_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
private:
    std::unique_ptr<Sinc> m_Sinc;
};
RegAlgo(Sinc_Block);

#endif // SINC_BLOCK_H
