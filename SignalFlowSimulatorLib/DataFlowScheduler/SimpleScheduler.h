#ifndef SIMPLESCHEDULER_H
#define SIMPLESCHEDULER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>

#include "Block.h"
#include "signalflowlinksort.h"
#include "DataStreamVerification.h"

class SimpleScheduler
{
public:
    SimpleScheduler();
    ~SimpleScheduler();
    
    // 调度接口
    bool schedule(const QString& linkKey, 
                  QVector<Block*> blocks,
                  std::shared_ptr<DataStreamVerification> verificationSystem,
                  const SimuParameter& simuParams = SimuParameter());
    
    // 控制接口
    void pause();
    void resume();
    void requestStop();
    bool isPaused() const;
    
    // 设置暂停控制成员
    void setPauseControls(QAtomicInt* paused, 
                          QAtomicInt* stopRequested,
                          QMutex* pauseMutex,
                          QWaitCondition* pauseCond);
    
private:
    // 私有调度方法
    bool simpleSchedulerImpl(const QString& linkKey, 
                             QVector<Block*> blocks,
                             std::shared_ptr<DataStreamVerification> verificationSystem,
                             const SimuParameter& simuParams);
    
    // 进度计算方法
    int calculateMaxProcessCount(QVector<Block*> blocks, 
                                 const QString& linkKey, 
                                 int sourceCount);
    double calculateCumulativeSamplingRate(Block* source, 
                                           Block* sink, 
                                           QVector<Block*> blocks);
    double getBlockProcessingRatio(Block* block);
    Block* getBlockByReader(BufferReader* reader, 
                            QVector<Block*> blocks);
    double calculateSamplingRateProgress(QVector<Block*> blocks,
                                         const std::map<std::string, int>& sinkProcessCount,
                                         int maxProcessCount);
    double calculateSingleSinkProgress(QVector<Block*> blocks,
                                       const std::string& sinkName,
                                       int sinkCount,
                                       int maxProcessCount);
    
    // 块处理函数
    bool processSourceBlock(Block* block, 
                            int& processCount, 
                            int maxProcessCount);
    bool processSinkBlock(Block* block);
    bool processProcessorBlock(Block* block);
    int generalWork(Block* currentBlock);
    
    // 拓扑排序器
    SignalFlowLinkSort m_topologySorter;
    
    // 暂停控制成员（指针引用，避免数据复制）
    QAtomicInt* m_paused = nullptr;           // 暂停标志: 0=运行, 1=暂停
    QAtomicInt* m_stopRequested = nullptr;    // 停止请求标志: 0=运行, 1=请求停止
    QMutex* m_pauseMutex = nullptr;          // 暂停互斥锁
    QWaitCondition* m_pauseCond = nullptr;   // 暂停条件变量
};
#endif // SIMPLESCHEDULER_H