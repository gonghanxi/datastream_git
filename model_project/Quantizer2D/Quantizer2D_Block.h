#ifndef QUANTIZER2D_BLOCK_H
#define QUANTIZER2D_BLOCK_H
#include "Quantizer2D.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Quantizer2D_Block : public Block
{
public:
    Quantizer2D_Block(const std::string& name);
    ~Quantizer2D_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
    bool ModelSetup();
private:
    void SetDefaultParameters();
    bool ValidateParameters();

    std::unique_ptr<Quantizer2D> m_Quantizer;

    double VxMax;
    double VxMin;
    double Nx;
    double VyMax;
    double VyMin;
    double Ny;
    DComplexMatrix QuantList;

    double xDelta;
    double yDelta;
};
RegAlgo(Quantizer2D_Block);

#endif // QUANTIZER2D_BLOCK_H
