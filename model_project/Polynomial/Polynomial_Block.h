#ifndef POLYNOMIAL_BLOCK_H
#define POLYNOMIAL_BLOCK_H
#include "Polynomial.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Polynomial_Block : public Block
{
public:
    Polynomial_Block(const std::string& name);
    ~Polynomial_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<Polynomial> m_Polynomial;
    Matrix<double>	Coefficients;
};
RegAlgo(Polynomial_Block);
#endif // POLYNOMIAL_BLOCK_H
