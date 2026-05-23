#include "SinkControlImplementation.h"





SystemVueModelBuilder::SinkControlImplementation::SinkControlImplementation()
    :m_pModel(nullptr),
      m_iStartSample(0),
      m_iStopSample(0),
      m_dStartTime(0.0),
      m_dStopTime(0.0),
      m_dTimeStep(0.0),
      m_dFirstTimeStamp(0.0),
      m_iFiringCount(0),
      m_iStartIndex(0),
      m_iStopIndex(0),
      m_bFinish(false)
{
    //初始化
}

SystemVueModelBuilder::SinkControlImplementation::SinkControlImplementation(const SystemVueModelBuilder::SinkControlImplementation &other)
    : m_pModel(other.m_pModel)  // 注意：这里只是复制指针，不是深拷贝！
    , m_iStartSample(other.m_iStartSample)
    , m_iStopSample(other.m_iStopSample)
    , m_dStartTime(other.m_dStartTime)
    , m_dStopTime(other.m_dStopTime)
    , m_dTimeStep(other.m_dTimeStep)
    , m_dFirstTimeStamp(other.m_dFirstTimeStamp)
    , m_iFiringCount(other.m_iFiringCount)
    , m_iStartIndex(other.m_iStartIndex)
    , m_iStopIndex(other.m_iStopIndex)
    , m_bFinish(other.m_bFinish)
{
    //初始化
}

SystemVueModelBuilder::SinkControlImplementation &SystemVueModelBuilder::SinkControlImplementation::operator=(const SystemVueModelBuilder::SinkControlImplementation &other)
{
    //赋值操作符
    if (this != &other) {
        m_pModel = other.m_pModel;  // 注意：只是复制指针
        m_iStartSample = other.m_iStartSample;
        m_iStopSample = other.m_iStopSample;
        m_dStartTime = other.m_dStartTime;
        m_dStopTime = other.m_dStopTime;
        m_dTimeStep = other.m_dTimeStep;
        m_dFirstTimeStamp = other.m_dFirstTimeStamp;
        m_iFiringCount = other.m_iFiringCount;
        m_iStartIndex = other.m_iStartIndex;
        m_iStopIndex = other.m_iStopIndex;
        m_bFinish = other.m_bFinish;
    }
    return *this;
}

bool SystemVueModelBuilder::SinkControlImplementation::Initialize(DFModel *pModel, unsigned long long iStartSample, unsigned long long iStopSample)
{
    //初始化
    if(!pModel) return false;
    m_pModel = pModel;
    m_iStartSample = iStartSample;
    m_iStopSample = iStopSample;
    m_iStartIndex = iStartSample;
    m_iStopIndex = iStopSample;
    m_iFiringCount = 0;
    m_bFinish = false;

    m_dStartTime = 0.0;
    m_dStopTime = 0.0;
    m_dTimeStep = 1.0;
    m_dFirstTimeStamp = 0.0;

    return true;
}

bool SystemVueModelBuilder::SinkControlImplementation::Initialize(DFModel *pModel, double dStartTime, double dStopTime, double dTimeStep, double dFirstTimeStamp)
{
    //初始化
    if(!pModel) return false;
    m_pModel = pModel;
    m_dStartTime = dStartTime;
    m_dStopTime = dStopTime;
    m_dTimeStep = dTimeStep;
    m_dFirstTimeStamp = dFirstTimeStamp;

    m_iStartSample = static_cast<unsigned long long>((dStartTime - dFirstTimeStamp) / dTimeStep);
    m_iStopSample = static_cast<unsigned long long>((dStopTime - dFirstTimeStamp) / dTimeStep);
    m_iStartIndex = m_iStartSample;
    m_iStopIndex = m_iStopSample;
    m_iFiringCount = 0;
    m_bFinish = false;

    return true;
}

bool SystemVueModelBuilder::SinkControlImplementation::CollectData()
{
    //收集数据
    if(!m_pModel || m_bFinish) return false;

    m_iFiringCount++;
    if(m_iFiringCount >= (m_iStopIndex - m_iStartIndex + 1)) {
        m_bFinish = true;
    }
    return true;
}

void SystemVueModelBuilder::SinkControlImplementation::StopControl()
{
    //停止控制
    m_bFinish = true;
}

unsigned long long SystemVueModelBuilder::SinkControlImplementation::GetNumPoints()
{
    //获取数量
    return m_iStopIndex - m_iStartIndex + 1;
}


