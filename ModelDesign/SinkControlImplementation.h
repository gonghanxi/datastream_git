#ifndef SINKCONTROLIMPLEMENTATION_H
#define SINKCONTROLIMPLEMENTATION_H
//#ifndef SV_CODE_GEN

#include "SimulationControl.h"
#include "DFModel.h"

namespace SystemVueModelBuilder {
class SinkControlImplementation
{
public:
    //SinkControl实现类
    SinkControlImplementation();
    ~SinkControlImplementation() = default;

    // 深拷贝
    SinkControlImplementation(const SinkControlImplementation& other);
    SinkControlImplementation& operator=(const SinkControlImplementation& other);

    // 允许移动
    SinkControlImplementation(SinkControlImplementation&&) = default;
    SinkControlImplementation& operator=(SinkControlImplementation&&) = default;

    //初始化仿真软件设置的参数
    bool Initialize(DFModel* pModel, unsigned long long iStartSample, unsigned long long iStopSample);
    bool Initialize(DFModel* pModel,double dStartTime,double dStopTime, double dTimeStep, double dFirstTimeStamp);
    //判断是否收集数据
    bool CollectData();
    void StopControl();
    unsigned long long GetNumPoints();
private:
    DFModel* m_pModel;

    //样本索引方式参数
    unsigned long long m_iStartSample;
    unsigned long long m_iStopSample;

    //时间方式参数
    double m_dStartTime;
    double m_dStopTime;
    double m_dTimeStep;
    double m_dFirstTimeStamp;

    //
    unsigned long long m_iFiringCount;
    unsigned long long m_iStartIndex;
    unsigned long long m_iStopIndex;
    bool m_bFinish;
};

}

#endif // SINKCONTROLIMPLEMENTATION_H
