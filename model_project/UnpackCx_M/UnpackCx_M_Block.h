#ifndef UNPACKCX_M_BLOCK_H
#define UNPACKCX_M_BLOCK_H
#include "Block.h"
#include "UnpackCx_M.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API UnpackCx_M_Block : public Block
{
public:
    UnpackCx_M_Block(const std::string& name);
    ~UnpackCx_M_Block() = default;

    bool Setup() override;
    bool Initialize() override;
    bool Run() override;

    void SetParameters();
private:
    UnpackCx_M::SelectedFormat ConvertStringToSelectedFormat(const std::string& value);
    void SetDefaultParameters();

    std::unique_ptr<UnpackCx_M> m_Unpack_M;

    int m_NumRows;
    int m_NumCols;
    UnpackCx_M::SelectedFormat m_Format;
};
RegAlgo(UnpackCx_M_Block);

#endif // UNPACKCX_M_BLOCK_H
