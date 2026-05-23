#ifndef DFMODEL_H
#define DFMODEL_H
#pragma once

#include "DFInterface.h"
#include "Block.h"

namespace SystemVueModelBuilder {

class DFInterface;
class DFModel : public Block
{
public:
#ifndef SV_CODE_GEN
    //定义接口函数
    virtual bool DefineInterface(DFInterface& model) = 0;

    // 获取模型类名
    virtual const char* GetModelClassName() const = 0;

    // 检查端口是否已注册
    bool ArePortsRegistered() const;

    // 注册端口方法（在 Initialize 中调用）
    bool RegisterPorts();

#endif
    //定义模型函数
    virtual bool Setup() override;
    virtual bool Initialize() override;
    virtual bool Run() override;
    virtual bool Finalize();
    virtual bool UpdateDynamicParameters();

    DFModel();
    virtual ~DFModel() {}
#ifndef SV_CODE_GEN
    DFInterface * pcInterface;

private:
    bool m_portsRegistered = false;
#endif
};

//定义导出函数
#define ReAlgo(className) \
extern "C" __declspec(dllexport) SystemVueModelBuilder::DFModel* createAlgo() { \
    return new className(); \
} \
extern "C" __declspec(dllexport) const char* getAlgoName() { \
    return #className; \
} \
extern "C" __declspec(dllexport) void* getAlgoInterface() { \
    return nullptr; \
}
}

#endif // DFMODEL_H
