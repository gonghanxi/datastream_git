#ifndef LIMIT_M_BLOCK_H
#define LIMIT_M_BLOCK_H

#include "Block.h"
#include "Limit_M.h"

#include <memory>
#include <string>
#include <vector>

using namespace SystemVueModelBuilder;

class SYSTEMVUEMODELBUILDER_API Limit_M_Block : public SystemVueModelBuilder::Block
{
public:
    Limit_M_Block(const std::string& name);
    ~Limit_M_Block() = default;

    bool Setup()      override;
    bool Run()        override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();

    Limit_M::SelectedLimiterType ConvertStringToSelectedLimiterType(const std::string& value);

    double m_K;
    double m_Bottom;
    double m_Top;
    Limit_M::SelectedLimiterType m_LimiterType;

    std::unique_ptr<Limit_M> m_Limit_M;
};

RegAlgo(Limit_M_Block);

#endif // LIMIT_M_BLOCK_H
