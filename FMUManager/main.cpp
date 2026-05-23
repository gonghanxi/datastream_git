#include <QString>
#include <QDebug>

#include "FMUManager.h"

int main(int argc, char *argv[])
{
    //路径
    QString path = "";

    //FMU管理类对象
    FMUManager manager;
    //FMU信息初始化容器
    std::vector<fmuCreateInfo> fmutestinfolst;
    //FMU配置结构体
    //模型名称
    //模型库路径
    //guid
    //fmi类型
    fmuConfig config = {"MathModel",
                        "D:/fmu_test/MathModel/binaries/win64/MathModel.dll",
                       "f0912b3c-4b87-43ed-877f-05faecf1f74f",
                       fmiType(CS)};
    //FMU参数容器
    std::vector<FmuVar> fmuVec1;
    //FMU参数结构体
    //名称
    //索引
    //类型
    //属性
    //初值
    FmuVar var1 = {"par",
                 144,
                 Real,
                 Parameter,
                 double(10)};
    FmuVar var2 = {"u",
                 88,
                 Real,
                 Input,
                 double(0.0)};
    FmuVar var3 = {"y",
                 192,
                 Real,
                 Output,
                 double(0.0)};
    fmuVec1.push_back(var1);
    fmuVec1.push_back(var2);
    fmuVec1.push_back(var3);

    fmuCreateInfo fmuinfo={config, fmuVec1};

    qDebug() << "startValue" << std::get<double>(fmuinfo.fmuVec.at(0).startValue);
    fmutestinfolst.push_back(fmuinfo);

    //FMU加载
    manager.load(fmutestinfolst);
    //仿真步长
    double stepsize = 0.1;
    //仿真次数
    int stepcount = 500;
    //起始时间
    double currrent_time = 0;
    //guid
    QString guid = "f0912b3c-4b87-43ed-877f-05faecf1f74f";
    //输入端口名称组
    std::vector<QString> input_names = {"u"};
    //输入端口初值组
    std::vector<double> values(1.0);
    //输出端口名称组
    std::vector<QString> output_names = {"y"};
    //输出端口结果组
    std::vector<double> result;

    values[0] = currrent_time+10;
    //设置参数/输入端口值 方法
    manager.setReals(guid, input_names, values);
    //执行方法
    manager.dostep(guid, currrent_time, stepsize);
    //获取执行的值 方法
    result = manager.getReals(guid,output_names);

    qDebug()<< result[0];
//    for (int i = 0; i < stepcount; i++)
//    {
//        values[0] = currrent_time;
//        manager.setReals(guid, names, values);

//    }

//    manager.dostep();
    manager.terminate();






    return 0;
}
