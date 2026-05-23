#ifndef SIMCFGDATA_H
#define SIMCFGDATA_H

#include <vector>
#include <QVector>
#include <QPointF>
class SimCfgData
{
public:
    SimCfgData(double durationTime,double step);
public:
    //    double getStep()const;
    //    double getDurationTime()const;
    double getDurationTime() const;

    double getStep() const;

    double getCurTime() const;

    //    void setCurTime(double value);

    int getSourceSampleNum() const ;

    void setSourceSampleFrq(double sampleFrq);
    void addTimes(double time);

    //     void advance(double step);
    const std::vector<double> & getTimes() const;

    //     void initTime(double value);
    int getIndx() const;



    int getWXPos() const;
    void setWXPos(int value);

    int getWYPos() const;
    void setWYPos(int value);

private:
    double  durationTime = 0;
    double step = 0.1;
    //    double curTime = 0;;
    double mSampleFrq = 0;
    int index = 0;
    int mSourceSampleNum = 0;

    //    int plotType = 1; //1为曲线，0为阶梯图

    std::vector<double> times;//已经仿真的时间

    int wXPos = 0;
    int wYPos = 0;

    //    QVector<QPointF> & xyData;
};

#endif // SIMCFGDATA_H
