#ifndef POLYNOMIALCX_BLOCK_H
#define POLYNOMIALCX_BLOCK_H
#include "PolynomialCx.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API PolynomialCx_Block : public Block
{
public:
    PolynomialCx_Block(const std::string& name);
    ~PolynomialCx_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<PolynomialCx> m_Polynomial;
    Matrix<std::complex<double>>	Coefficients;
};
RegAlgo(PolynomialCx_Block);

#endif // POLYNOMIALCX_BLOCK_H
