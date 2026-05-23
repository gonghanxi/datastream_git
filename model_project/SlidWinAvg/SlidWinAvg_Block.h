#ifndef SLIDWINAVG_BLOCK_H
#define SLIDWINAVG_BLOCK_H
#include "SlidWinAvg.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API SlidWinAvg_Block : public Block
{
public:
    SlidWinAvg_Block(const std::string& name);
    ~SlidWinAvg_Block() = default;
    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    void SetDefaultParameters();

    std::unique_ptr<SlidWinAvg> m_SlidWinAvg;

    // Parameter
    int WindowSize;

    SystemVueModelBuilder::Matrix<double> slideWindow;
    int currentIndex;
    double currentSum;
};
RegAlgo(SlidWinAvg_Block)
#endif // SLIDWINAVG_BLOCK_H
