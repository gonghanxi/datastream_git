#ifndef UNWRAP_BLOCK_H
#define UNWRAP_BLOCK_H
#include "Unwrap.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Unwrap_Block : public Block
{
public:
    Unwrap_Block(const std::string& name);
    ~Unwrap_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    Unwrap::SelectedPhaseType ConvertStringToSelectedPhaseType(const std::string& value);

    void SetDefaultParameters();

    std::unique_ptr<Unwrap> m_Unwrap;

    Unwrap::SelectedPhaseType	m_PhaseType;
    double m_OutPhase;
    double m_PrevPhase;
};
RegAlgo(Unwrap_Block);

#endif // UNWRAP_BLOCK_H
