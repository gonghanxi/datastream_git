#ifndef RECTTOPOLAR_BLOCK_H
#define RECTTOPOLAR_BLOCK_H
#include "RectToPolar.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API RectToPolar_Block : public Block
{
public:
    RectToPolar_Block(const std::string& name);
    ~RectToPolar_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
private:
    std::unique_ptr<RectToPolar> m_RectToPolar;
};
RegAlgo(RectToPolar_Block);

#endif // RECTTOPOLAR_BLOCK_H
