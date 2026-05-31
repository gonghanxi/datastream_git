#ifndef SVD_M_BLOCK_H
#define SVD_M_BLOCK_H

#include "Block.h"
#include "SVD_M.h"

#include <memory>
#include <string>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API SVD_M_Block : public SystemVueModelBuilder::Block
{
public:
    SVD_M_Block(const std::string& name);
    ~SVD_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    SVD_M::GenerateLeftE  ConvertStringToGenerateLeft(const std::string& value);
    SVD_M::GenerateRightE ConvertStringToGenerateRight(const std::string& value);

    static void calc_svd(const SystemVueModelBuilder::DoubleMatrix& A,
                         SystemVueModelBuilder::DoubleMatrix& Uo,
                         SystemVueModelBuilder::DoubleMatrix& Wo,
                         SystemVueModelBuilder::DoubleMatrix& Vo,
                         double threshold, int maxIters, int needV);
    static void transpose(const SystemVueModelBuilder::DoubleMatrix& A,
                          SystemVueModelBuilder::DoubleMatrix& AT);

    double m_Threshold;
    int    m_MaxIterations;
    SVD_M::GenerateLeftE  m_GenerateLeft;
    SVD_M::GenerateRightE m_GenerateRight;

    std::unique_ptr<SVD_M> m_SVD_M;

    // 帧间符号稳定性状态
    bool m_hasPrevV;
    SystemVueModelBuilder::DoubleMatrix m_prevV;
};

RegAlgo(SVD_M_Block);

#endif // SVD_M_BLOCK_H
