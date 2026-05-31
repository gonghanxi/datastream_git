#ifndef TOEPLITZ_M_BLOCK_H
#define TOEPLITZ_M_BLOCK_H

#include "Block.h"
#include "Toeplitz_M.h"

#include <memory>
#include <queue>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Toeplitz_M_Block : public SystemVueModelBuilder::Block
{
public:
    Toeplitz_M_Block(const std::string& name);
    ~Toeplitz_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    bool DataStreamRun();
    bool TimeDrivenRun();

    std::unique_ptr<Toeplitz_M> m_Toeplitz_M;

    int m_NumRows;
    int m_NumCols;

    // 时间驱动缓冲
    std::vector<double> m_inputBuffer;
    std::queue<SystemVueModelBuilder::DoubleMatrix> m_outputQueue;
};

RegAlgo(Toeplitz_M_Block);

#endif // TOEPLITZ_M_BLOCK_H
