#ifndef DYNAMICCONTROLIMPLEMENTATION_H
#define DYNAMICCONTROLIMPLEMENTATION_H
#include "SimulationControl.h"
#include "DFModel.h"

namespace SystemVueModelBuilder {
    class DynamicControlImplementation
    {
    public:
        //动态控制实现类，目前没有实现
        DynamicControlImplementation();
        ~DynamicControlImplementation() = default;

        // 禁用拷贝
        DynamicControlImplementation(const DynamicControlImplementation& other);
        DynamicControlImplementation& operator=(const DynamicControlImplementation& other);

        // 允许移动
        DynamicControlImplementation(DynamicControlImplementation&&) = default;
        DynamicControlImplementation& operator=(DynamicControlImplementation&&) = default;

        bool Initialize(DFModel* model);
        void StopControl();
    private:
        DFModel* m_pModel;
};
    }
#endif // DYNAMICCONTROLIMPLEMENTATION_H
