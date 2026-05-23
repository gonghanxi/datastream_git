#ifndef ROTATE_BLOCK_H
#define ROTATE_BLOCK_H
#include "Rotate.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Rotate_Block : public Block
{
public:
    Rotate_Block(const std::string& name);
    ~Rotate_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    void SetParameters();
private:

    void SetDefaultParameters();

    std::unique_ptr<Rotate> m_Rotate;

    double m_RotationAngle;
};
RegAlgo(Rotate_Block);

#endif // ROTATE_BLOCK_H
