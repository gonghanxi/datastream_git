#include "FMUManager.h"
FMUManager* FMUManager::m_instance = nullptr;

FMUManager::FMUManager()
{

}


FMUManager::~FMUManager()
{
    for(auto fmuvar:this->fmumap)
    {
        if (fmuvar)
        {
            fmuvar->terminate();
            fmuvar->free();
        }
    }
    fmumap.clear();
}

FMUManager *FMUManager::getInstance()
{
    if (m_instance == nullptr) {
        m_instance = new FMUManager();
    }
    return m_instance;
}

bool FMUManager::hasInstance(const QString &guid) const
{
    return fmumap.contains(guid);
}


bool FMUManager::load(std::vector<fmuCreateInfo>fmuinfolist)
{
    qDebug() << "[FMUManager] load - fmuinfolist size:" << fmuinfolist.size();
    for(const auto& varinfo: fmuinfolist)
    {
        if(!this->fmumap.contains(varinfo.config.guid))
        {
            qDebug() << "[FMUManager] Creating FMU for guid:" << varinfo.config.guid;
            std::shared_ptr<FMU> fmu = std::make_shared<FMU>(varinfo);

            qDebug() << "[FMUManager] Calling fmu->load()...";
            bool loadOk = fmu->load();
            qDebug() << "[FMUManager] fmu->load() returned:" << loadOk;

            qDebug() << "[FMUManager] Calling fmu->instantiate()...";
            bool instOk = fmu->instantiate();
            qDebug() << "[FMUManager] fmu->instantiate() returned:" << instOk;

            qDebug() << "[FMUManager] Calling fmu->initstartvalue()...";
            bool initOk = fmu->initstartvalue();
            qDebug() << "[FMUManager] fmu->initstartvalue() returned:" << initOk;

            this->fmumap.insert(varinfo.config.guid,fmu);
            qDebug() << "[FMUManager] FMU inserted into map, guid:" << varinfo.config.guid;
        }
    }
    return true;
}


bool FMUManager::terminate()
{
    for(auto& fmu: this->fmumap)
    {
        if (fmu)
        {
            fmu->terminate();
            fmu->free();
        }
    }
    return true;
}


bool FMUManager::dostep(const QString guid,double currentTime, double stepSize)
{
    qDebug() << "[FMUManager] dostep - guid:" << guid << "currentTime:" << currentTime << "stepSize:" << stepSize;
    qDebug() << "[FMUManager] fmumap keys:" << this->fmumap.keys();

    auto it = this->fmumap.find(guid);
    if (it == fmumap.end())
    {
        qDebug() << "[FMUManager] dostep - guid NOT found in map!";
        return false;
    }

    std::shared_ptr<FMU> fmuVarPtr = it.value();
    qDebug() << "[FMUManager] dostep - calling fmu->doStep(), fmu ptr:" << fmuVarPtr.get();
    return fmuVarPtr->doStep(currentTime, stepSize);

//    std::shared_ptr<FMU> p(new FMU())
}


bool FMUManager::setReals(const QString guid, const std::vector<QString>& names, const std::vector<double>& values)
{
    auto it = this->fmumap.find(guid);
    if (it == fmumap.end())
    {
        return false;
    }

    std::shared_ptr<FMU> fmuVarPtr = it.value();
    if (names.size() > 1)
    {
        fmuVarPtr->setReals(names, values);
    }
    else
    {
        fmuVarPtr->setReal(names.at(0), values.at(0));
    }
    return true;
}


std::vector<double> FMUManager::getReals(const QString guid,const std::vector<QString>& names)
{
    auto it = this->fmumap.find(guid);
    std::vector<double> result;
    if (it == fmumap.end())
    {
        return {};
    }

    std::shared_ptr<FMU> fmuVarPtr = it.value();
    if (names.size() > 1)
    {
         result = fmuVarPtr->getReals(names);

    }
    else
    {
        result.push_back(fmuVarPtr->getReal(names.at(0)));
    }
    return result;
}


bool FMUManager::setBooleans(const QString guid,const std::vector<QString>& names, const std::vector<bool>& values)
{
    auto it = this->fmumap.find(guid);
    if (it == fmumap.end())
    {
        return false;
    }

    std::shared_ptr<FMU> fmuVarPtr = it.value();
    if (names.size() > 1)
    {
        fmuVarPtr->setBooleans(names, values);
    }
    else
    {
        fmuVarPtr->setBoolean(names.at(0), values.at(0));
    }
    return true;
}


std::vector<bool> FMUManager::getBooleans(const QString guid,const std::vector<QString>& names)
{
    auto it = this->fmumap.find(guid);
    std::vector<bool> result;
    if (it == fmumap.end())
    {
        return {};
    }

    std::shared_ptr<FMU> fmuVarPtr = it.value();
    if (names.size() > 1)
    {
         result = fmuVarPtr->getBooleans(names);
    }
    else
    {
        result.push_back(fmuVarPtr->getBoolean(names.at(0)));
    }
    return result;
}


bool FMUManager::setIntegers(const QString guid,const std::vector<QString>& names, const std::vector<int>& values)
{
    auto it = this->fmumap.find(guid);
    if (it == fmumap.end())
    {
        return false;
    }

    std::shared_ptr<FMU> fmuVarPtr = it.value();
    if (names.size() > 1)
    {
        fmuVarPtr->setIntegers(names, values);
    }
    else
    {
        fmuVarPtr->setInteger(names.at(0), values.at(0));
    }
    return true;
}


std::vector<int> FMUManager::getIntegers(const QString guid,const std::vector<QString>& names)
{
    auto it = this->fmumap.find(guid);
    std::vector<int> result;
    if (it == fmumap.end())
    {
        return {};
    }

    std::shared_ptr<FMU> fmuVarPtr = it.value();
    if (names.size() > 1)
    {
         result = fmuVarPtr->getIntegers(names);
    }
    else
    {
        result.push_back(fmuVarPtr->getInteger(names.at(0)));
    }
    return result;
}


bool FMUManager::setStrings(const QString guid,const std::vector<QString>& names, const std::vector<QString>& values)
{
    auto it = this->fmumap.find(guid);
    if (it == fmumap.end())
    {
        return false;
    }

    std::shared_ptr<FMU> fmuVarPtr = it.value();
    if (names.size() > 1)
    {
        fmuVarPtr->setStrings(names, values);
    }
    else
    {
        fmuVarPtr->setString(names.at(0), values.at(0));
    }
    return true;
}


std::vector<QString> FMUManager::getStrings(const QString guid,const std::vector<QString>& names)
{
    auto it = this->fmumap.find(guid);
    std::vector<QString> result;
    if (it == fmumap.end())
    {
        return {};
    }

    std::shared_ptr<FMU> fmuVarPtr = it.value();
    if (names.size() > 1)
    {
         result = fmuVarPtr->getStrings(names);
    }
    else
    {
        result.push_back(fmuVarPtr->getString(names.at(0)));
    }
    return result;
}


