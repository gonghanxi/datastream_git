#ifndef DIRICHLET_BLOCK_H
#define DIRICHLET_BLOCK_H

#include "Block.h"
#include "Dirichlet.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Dirichlet_Block : public Block
{
public:
    Dirichlet_Block(const std::string& name);
    ~Dirichlet_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    void SetDefaultParamters();
    double dirichlet_sample(double omega_rad, int Nval);

    std::unique_ptr<Dirichlet> m_Dirichlet;

    int m_N;
    int m_DomainFlag;
    int m_NormalizeFlag;
    int m_InputMapping;
};
RegAlgo(Dirichlet_Block);
#endif // DIRICHLET_BLOCK_H
