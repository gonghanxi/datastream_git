#ifndef COMBINER_M_BLOCK_H
#define COMBINER_M_BLOCK_H
#include "Combiner_M.h"
#include "Block.h"
using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Combiner_M_Block : public Block
{
public:
    Combiner_M_Block(const std::string& name);
    ~Combiner_M_Block() = default;
    bool Run() override;
    bool Setup() override;
    bool Initialize() override;
    void SetParameters();
private:
    void SetDefaultParameters();
    Combiner_M::SelectedMode ConvertStringToSelectedMode(const std::string& value);

    std::unique_ptr<Combiner_M> m_Combiner;

    Combiner_M::SelectedMode Mode;
    int NumRows;
    int NumCols;
    Matrix<int> ElementMap;
    double InsertionLoss;

    int inRow;
    int inCol;
    int outRow;
    int outCol;
    int numMap;
    int maxChannel;
    SystemVueModelBuilder::Matrix<int> channelCount;
};
RegAlgo(Combiner_M_Block)
#endif // COMBINER_M_BLOCK_H
