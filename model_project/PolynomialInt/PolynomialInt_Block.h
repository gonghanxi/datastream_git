#ifndef POLYNOMIALINT_BLOCK_H
#define POLYNOMIALINT_BLOCK_H
#include "PolynomialInt.h"
#include "Block.h"
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API PolynomialInt_Block : public Block
{
public:
    PolynomialInt_Block(const std::string& name);
    ~PolynomialInt_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<PolynomialInt> m_Polynomial;
    Matrix<int>	Coefficients;
};
RegAlgo(PolynomialInt_Block);

#endif // POLYNOMIALINT_BLOCK_H
