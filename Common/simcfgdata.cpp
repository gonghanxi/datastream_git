#include "simcfgdata.h"

SimCfgData::SimCfgData(double durationTime,double step)
{
   this->durationTime=durationTime;
   this->step=step;
//    times.resize(1);
}

double SimCfgData::getDurationTime() const
{
    return durationTime;
}

double SimCfgData::getStep() const
{
    return step;
}

int SimCfgData::getIndx() const
{
    return index;
}

int SimCfgData::getWXPos() const
{
    return wXPos;
}

void SimCfgData::setWXPos(int value)
{
    wXPos = value;
}

int SimCfgData::getWYPos() const
{
    return wYPos;
}

void SimCfgData::setWYPos(int value)
{
    wYPos = value;
}



double SimCfgData::getCurTime() const
{
    if (index<1)
        return times[0];
    return times[index -1];
}

//void SimCfgData::initTime(double value)
//{
//    times[0] = value;
//    index++;
//}

int SimCfgData::getSourceSampleNum() const
{
    return mSourceSampleNum;
}
const std::vector<double> &SimCfgData::getTimes() const
{
    return times;
}
void SimCfgData::setSourceSampleFrq(double sampleFrq)
{
    int num1 = durationTime/step;
    mSourceSampleNum = step*sampleFrq;
    int size =   num1*mSourceSampleNum;
    mSampleFrq =  sampleFrq;
    times.resize(size+1);

}

#include <QDebug>
void SimCfgData::addTimes(double time)
{
    if (index >= times.size())
    {
        qDebug()<<"error SimCfgData::addTimes";
        return;
    }
    times[index] = time;
    index++;
}

