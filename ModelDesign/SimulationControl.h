#ifndef SIMULATIONCONTROL_H
#define SIMULATIONCONTROL_H
#pragma once


namespace SystemVueModelBuilder {
    class DFModel;
#ifndef SV_CODE_GEN
    class SinkControlImplementation;
    class DynamicControlImplementation;
#endif
    class SinkControl
    {
    public:
        //终端控制
        SinkControl();
        ~SinkControl();

        //初始化仿真软件设置的参数
        bool Initialize(DFModel* pModel, unsigned long long iStartSample, unsigned long long iStopSample);
        bool Initialize(DFModel* pModel,double dStartTime,double dStopTime, double dTimeStep, double dFirstTimeStamp);
        //判断是否收集数据
        bool CollectData();
        void StopControl();
        unsigned long long GetNumPoints();
    private:

        SinkControlImplementation* m_pImplementation;
        unsigned long long m_iFiringCount;
        unsigned long long m_iStartIndex;
        unsigned long long m_iStopIndex;
        bool bFinish;
};
    class DynamicControl
    {
    public:
        //动态控制
        DynamicControl();
        ~DynamicControl();

        //初始化仿真软件设置的参数
        bool Initialize(DFModel* model);
        //判断是否收集数据
        void StopControl();
    private:
#ifndef SV_CODE_GEN
        DynamicControlImplementation* m_pImplementation;
#endif
    };
    }
#endif // SIMULATIONCONTROL_H
