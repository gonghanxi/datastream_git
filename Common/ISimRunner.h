#ifndef ISIMRUNNER_H
#define ISIMRUNNER_H

#include "ILogWriter.h"
#include "SimuParameter.h"
#include "simcfgdata.h"
#include <string>

class ISimRunner{
public:

    virtual bool start()=0;
    virtual bool run()=0;


    virtual void setWriter(ILogWriter* write)=0;
    //virtual Port* getPort(std::string instanceName,std::string portId)=0;
    virtual void setSimCfg(SimCfgData* data)=0;
    virtual SystemVueModelBuilder::SimuParameter getSimulationParameters() const = 0;
    virtual ~ISimRunner(){}

    //暂停
    virtual void pause() = 0;
    //复位
    virtual void resume() = 0;
    //停止
    virtual void requestStop() = 0;
};
//typedef ISimRunner* (*CreateSimFunction)(QString, QStringList, QString);
// 使用const char* (UTF-8编码) 作为接口
typedef ISimRunner* (*CreateSimFunction)(const char* appPath,
                                         const char** linkFiles,
                                         int fileCount,
                                         const char* outPutPath);

//typedef ISimRunner* (*CreateSimFunction)(std::string);
#endif // ISIMRUNNER_H
