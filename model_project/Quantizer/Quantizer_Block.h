#ifndef QUANTIZER_BLOCK_H
#define QUANTIZER_BLOCK_H
#include "Quantizer.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Quantizer_Block : public Block
{
public:
    Quantizer_Block(const std::string& name);
    ~Quantizer_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
    bool ModelSetup();
private:
    void SetDefaultParameters();
    bool parseArrayString(const std::string& arrayStr, std::vector<double>& outArray);

    std::unique_ptr<Quantizer> m_Quantizer;

    double*  Thresholds;
    int ThresholdsSize;

    double*  Levels;
    int LevelsSize;

    std::vector<double> m_thresholdsData;
    std::vector<double> m_levelsData;

    std::vector<double> m_thresholds;
    std::vector<double> m_levels;
};
RegAlgo(Quantizer_Block);
#endif // QUANTIZER_BLOCK_H
