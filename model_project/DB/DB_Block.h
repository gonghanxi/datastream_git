#ifndef DB_BLOCK_H
#define DB_BLOCK_H

#include "Block.h"
#include "DB.h"

using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API DB_Block : public Block
{
public:
    DB_Block(const std::string& name);
    ~DB_Block() = default;

    bool Setup() override;
    bool Run() override;
    bool Initialize() override;

    void SetParameters();
private:
    DB::DbTypeEnum ConvertStringToDbTypeEnum(const std::string& value);
    void SetDefaultParamters();

    std::unique_ptr<DB> m_DB;

    double     m_Min;
    DB::DbTypeEnum m_DbType;
};
RegAlgo(DB_Block);
#endif // DB_BLOCK_H
