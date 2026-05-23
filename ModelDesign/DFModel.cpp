#include "DFModel.h"

using namespace SystemVueModelBuilder;

DFModel::DFModel()
{
#ifndef SV_CODE_GEN
    pcInterface = 0;
    m_portsRegistered = false;
#endif
}

bool DFModel::ArePortsRegistered() const { return m_portsRegistered; }

#ifndef SV_CODE_GEN
bool DFModel::RegisterPorts() {
    qDebug() << "[DFModel] RegisterPorts called for" << GetModelClassName();

    // 如果已经注册过，直接返回
    if (m_portsRegistered) {
        qDebug() << "[DFModel] Ports already registered, skipping";
        return true;
    }

    // 创建一个临时的 DFInterface 来触发 DefineInterface
    try {
        DFInterface interface(GetModelClassName());

        // 调用 DefineInterface（这会执行宏定义中的代码）
        if (!DefineInterface(interface)) {
            qDebug() << "[DFModel] DefineInterface returned false for" << GetModelClassName();
            return false;
        }

        qDebug() << "[DFModel] DefineInterface executed successfully for" << GetModelClassName();

        // 注意：端口注册是在宏定义中完成的（ADD_MODEL_INPUT/ADD_MODEL_OUTPUT）
        // 所以这里不需要额外的代码

        return true;
    } catch (const std::exception& e) {
        qDebug() << "[DFModel] Exception in RegisterPorts for" << GetModelClassName()
                 << ":" << e.what();
        return false;
    }
}
#endif

bool DFModel::Setup() {return true;}

bool DFModel::Initialize()
{
#ifndef SV_CODE_GEN
    // 在 Initialize 中注册端口
    if (!m_portsRegistered) {
        qDebug() << "[DFModel] Registering ports in Initialize() for" << GetModelClassName();

        // 调用 RegisterPorts 方法
        if (!RegisterPorts()) {
            qDebug() << "[DFModel] Failed to register ports for" << GetModelClassName();
            return false;
        }

        m_portsRegistered = true;
        qDebug() << "[DFModel] Ports registered successfully for" << GetModelClassName();
    }
#endif
    return true;
}

bool DFModel::Run() {return true;}

bool DFModel::Finalize() {return true;}

bool DFModel::UpdateDynamicParameters() {return true;}



