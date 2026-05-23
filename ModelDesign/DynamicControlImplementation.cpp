#include "DynamicControlImplementation.h"



SystemVueModelBuilder::DynamicControlImplementation::DynamicControlImplementation()
    :m_pModel(nullptr)
{
    //初始化
}

SystemVueModelBuilder::DynamicControlImplementation::DynamicControlImplementation(const SystemVueModelBuilder::DynamicControlImplementation &other)
    :m_pModel(other.m_pModel)
{
    //初始化

}

SystemVueModelBuilder::DynamicControlImplementation &SystemVueModelBuilder::DynamicControlImplementation::operator=(const SystemVueModelBuilder::DynamicControlImplementation &other)
{
    //赋值操作符
    if(this != &other) {
        m_pModel = other.m_pModel;
    }
    return *this;
}

bool SystemVueModelBuilder::DynamicControlImplementation::Initialize(DFModel *model)
{
    //初始化
    if(!m_pModel) return false;

    m_pModel = model;

    return true;
}

void SystemVueModelBuilder::DynamicControlImplementation::StopControl()
{
    //停止控制

}


