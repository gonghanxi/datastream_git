#ifndef QUANTIZER_M_BLOCK_H
#define QUANTIZER_M_BLOCK_H

#include "Block.h"
#include "Quantizer_M.h"

#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Quantizer_M_Block : public SystemVueModelBuilder::Block
{
public:
    Quantizer_M_Block(const std::string& name);
    ~Quantizer_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

    void SetParameters();
    bool ModelSetup();

private:
    void SetDefaultParameters();
    bool parseArrayString(const std::string& arrayStr, std::vector<double>& outArray);
    int QuantizeIndex(double x) const;

    std::unique_ptr<Quantizer_M> m_Quantizer_M;

    double* Thresholds;
    int    ThresholdsSize;

    double* Levels;
    int    LevelsSize;

    std::vector<double> m_thresholdsData;
    std::vector<double> m_levelsData;

    std::vector<double> m_thresholds;
    std::vector<double> m_levels;
};

RegAlgo(Quantizer_M_Block);

#endif // QUANTIZER_M_BLOCK_H
