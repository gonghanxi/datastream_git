#ifndef SPLITTER_M_BLOCK_H
#define SPLITTER_M_BLOCK_H
#include "Splitter_M.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Splitter_M_Block : public Block
{
public:
    Splitter_M_Block(const std::string& name);
    ~Splitter_M_Block() = default;
    bool Run() override;
    bool Setup() override;
    bool Initialize() override;
    void SetParameters();
private:
    void SetDefaultParameters();
    Splitter_M::SelectedMode ConvertStringToSelectedMode(const std::string& value);

    std::unique_ptr<Splitter_M> m_Splitter;

    Splitter_M::SelectedMode Mode;
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
RegAlgo(Splitter_M_Block)

#endif // SPLITTER_M_BLOCK_H
