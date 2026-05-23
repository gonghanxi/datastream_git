#ifndef FMUMANAGER_H
#define FMUMANAGER_H

#include <memory>
#include "fmu.h"

class FMUManager
{
public:

    FMUManager();
    ~FMUManager();

    // 获取单例实例
    static FMUManager* getInstance();
    bool hasInstance(const QString& guid) const;

    /**
     * @brief 初始化
     * @param fmuinfolist fmuCreateInfo结构体
     */
    bool load(std::vector<fmuCreateInfo>fmuinfolist);
    /**
     * @brief 销毁
     */
    bool terminate();
    /**
     * @brief 执行
     */
    bool dostep(const QString guid,double currentTime, double stepSize);

    /**
     * @brief 设置和获取参数
     */
    bool setReals(const QString guid, const std::vector<QString>& names, const std::vector<double>& values);
    std::vector<double> getReals(const QString guid,const std::vector<QString>& names);
    bool setBooleans(const QString guid,const std::vector<QString>& names, const std::vector<bool>& values);
    std::vector<bool> getBooleans(const QString guid,const std::vector<QString>& names);
    bool setIntegers(const QString guid,const std::vector<QString>& names, const std::vector<int>& values);
    std::vector<int> getIntegers(const QString guid,const std::vector<QString>& names);
    bool setStrings(const QString guid,const std::vector<QString>& names, const std::vector<QString>& values);
    std::vector<QString> getStrings(const QString guid,const std::vector<QString>& names);




private:
    static FMUManager* m_instance;
    QHash<QString, std::shared_ptr<FMU>> fmumap;


};

#endif // FMUMANAGER_H
