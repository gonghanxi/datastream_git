#ifndef OSF_BLOCK_H
#define OSF_BLOCK_H

#include "Block.h"
#include "OSF.h"
#include <memory>

// 使用 SYSTEMVUEMODELBUILDER_API 导出 Block 类
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API OSF_Block : public SystemVueModelBuilder::Block
{
public:
    OSF_Block(const std::string& name);
    ~OSF_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

private:
    void SetDefaultParameters();
    void SetParameters();
    bool ValidateParameters();

    // 成员变量
    int m_n;
    int m_percentile;

    std::unique_ptr<SystemVueModelBuilder::OSF> m_OSF;
};

RegAlgo(OSF_Block);

#endif // OSF_BLOCK_H
