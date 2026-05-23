#ifndef COMPRESS_BLOCK_H
#define COMPRESS_BLOCK_H
#include "Compress.h"
#include "Block.h"
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API Compress_Block : public Block
{
public:
    Compress_Block(const std::string& name);
    ~Compress_Block() = default;
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    Compress::SelectedCompressionType ConvertStringToSelectedCompressionType(const std::string& value);
    void SetDefaultParameters();

    std::unique_ptr<Compress> m_Compress;

    Compress::SelectedCompressionType m_CompressionType;
    double m_CompressionK;
    double m_Max;
};
RegAlgo(Compress_Block);

#endif // COMPRESS_BLOCK_H
