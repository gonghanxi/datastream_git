#include "SimulationControl.h"
#include "SinkControlImplementation.h"
#include "DynamicControlImplementation.h"


// #ifndef SV_CODE_GEN
    SystemVueModelBuilder::SinkControl::SinkControl()
    {
        m_iFiringCount = 0;
        m_iStartIndex = 0;
        m_iStopIndex = 0;
        bFinish = false;
        //初始化
        m_pImplementation = new SinkControlImplementation();
    }

    SystemVueModelBuilder::SinkControl::~SinkControl()
    {
        delete m_pImplementation;
    }

    bool SystemVueModelBuilder::SinkControl::Initialize(DFModel *pModel, unsigned long long iStartSample, unsigned long long iStopSample)
    {
        m_iStartIndex = iStartSample;
        m_iStopIndex = iStopSample;
        m_iFiringCount = 0;
        bFinish = false;

        if (iStopSample <= iStartSample)
        {
            return false;
        }

        return true;
    }

    bool SystemVueModelBuilder::SinkControl::Initialize(DFModel *pModel, double dStartTime, double dStopTime, double dTimeStep, double dFirstTimeStamp)
    {
        // 根据时间和时间步长计算采样点索引
        if (dTimeStep <= 0.0 || dStopTime <= dStartTime)
        {
            return false;
        }

        unsigned long long startSample = (unsigned long long)((dStartTime - dFirstTimeStamp) / dTimeStep + 0.5);
        unsigned long long stopSample = (unsigned long long)((dStopTime - dFirstTimeStamp) / dTimeStep + 0.5);

        if (dStartTime < dFirstTimeStamp)
        {
            startSample = 0;
        }

        return Initialize(pModel, startSample, stopSample);
    }

    bool SystemVueModelBuilder::SinkControl::CollectData()
    {
        // 如果已经完成收集，返回false
        if (bFinish)
        {
            return false;
        }

        m_iFiringCount++;

        // 判断是否在收集范围内
        if (m_iFiringCount >= m_iStartIndex && m_iFiringCount <= m_iStopIndex)
        {
            return true;
        }
        else if (m_iFiringCount > m_iStopIndex)
        {
            // 超出范围，标记完成
            bFinish = true;
        }

        return false;

    }

    void SystemVueModelBuilder::SinkControl::StopControl()
    {
        //停止控制
        bFinish = true;
    }

    unsigned long long SystemVueModelBuilder::SinkControl::GetNumPoints()
    {
        if (m_iStopIndex > m_iStartIndex)
        {
            return m_iStopIndex - m_iStartIndex;
        }
        return 0;
    }

    SystemVueModelBuilder::DynamicControl::DynamicControl()
    {
        //初始化
        m_pImplementation = new DynamicControlImplementation();
    }

    SystemVueModelBuilder::DynamicControl::~DynamicControl()
    {
        delete m_pImplementation;
    }

    bool SystemVueModelBuilder::DynamicControl::Initialize(DFModel *model)
    {
        //初始化
        return m_pImplementation->Initialize(model);
    }

    void SystemVueModelBuilder::DynamicControl::StopControl()
    {
        //停止控制
        return m_pImplementation->StopControl();
    }

//#endif

