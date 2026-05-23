#ifndef VARIABLE_H
#define VARIABLE_H

#include <QString>
#include <QMap>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>

//变量结构体
struct Variable {
    QString name;           // 变量名
    QString dataType;       // 数据类型
    QString defaultValue;   // 默认值
    QString unit;          // 单位
    QString unitType;      // 单位类型
    QString constraint;    // 约束
    QString desc;          // 描述
    QString id;           // 变量ID
    bool disp;            // 是否显示

    Variable() : disp(true) {}

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["name"] = name;
        obj["dataType"] = dataType;
        obj["defaultValue"] = defaultValue;
        obj["unit"] = unit;
        obj["unitType"] = unitType;
        obj["constraint"] = constraint;
        obj["desc"] = desc;
        obj["id"] = id;
        obj["disp"] = disp;
        return obj;
    }
    static Variable fromJson(const QJsonObject& obj)
    {
        Variable var;
        var.name = obj["name"].toString();
        var.dataType = obj["dataType"].toString();
        var.defaultValue = obj["defaultValue"].toString();
        var.unit = obj["unit"].toString();
        var.unitType = obj["unitType"].toString();
        var.constraint = obj["constraint"].toString();
        var.desc = obj["desc"].toString();
        var.id = obj["id"].toString();
        var.disp = obj["disp"].toBool(true);
        return var;
    }
};

#endif // VARIABLE_H
