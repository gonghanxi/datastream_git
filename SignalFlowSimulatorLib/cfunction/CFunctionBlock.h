#ifndef CFUNCTIONBLOCK_H
#define CFUNCTIONBLOCK_H

#include "Block.h"
#include "CFunctionModelInfo.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

//using namespace SystemVueModelBuilder;
namespace SystemVueModelBuilder {

class CFunctionBlock : public Block {
public:
    explicit CFunctionBlock(const std::string& name);
    ~CFunctionBlock() override;

    // ========== 配置接口 ==========
    void setCFunctionConfig(const QString& instanceName, int cmpId);
    void setConfigData(const CFunctionConfigData& configData);
    void setEquations(const QString& equations);
    void setGeneratedJsonPath(const QString& path);
    void setSimuParams(const SimuParameter& params);

    // 端口和参数
    void addPortInfo(const PortMsg& port);
    void addParameterInfo(const QString& name, const QString& value);

    // ========== Block生命周期 ==========
    bool Initialize() override;
    bool Setup() override;
    bool Run() override;
    bool Stop() override;
    bool Done() override;
    bool Flush() override;

private:
    // 核心执行逻辑
    bool executeCFunction();

    // 更新cfunction.json的input字段（写入当前输入数据）
    bool updateJsonInput();

    // 调用外部CFunction小引擎（暂定：调用可执行程序并传参json路径）
    bool invokeEngine(const QString& jsonPath);

    // 读取cfunction.json的output字段并写入输出端口
    bool readAndWriteOutput();

    // ========== 成员变量 ==========
    QString m_instanceName;
    int m_cmpId = -1;
    CFunctionConfigData m_configData;
    QString m_equations;
    QString m_generatedJsonPath;    // cfunction.json绝对路径
    SimuParameter m_simuParams;

    // 端口信息缓存
    struct PortInfo {
        QString name;
        QString putType;    // "in" / "out"
        PortMsg::PortDataType dataType;
        int id = -1;
    };
    std::vector<PortInfo> m_portInfos;

    // 参数信息缓存
    std::map<std::string, QString> m_parameterValues;
};

}
#endif // CFUNCTIONBLOCK_H
