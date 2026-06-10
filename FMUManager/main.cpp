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
    fmuConfig config = {"Model44",
                        "C:/Users/shi/Desktop/matlab_git/GWDataFlowSimulator/build/fmu/debug/Model44.dll",
                       "f4ae3756-59d3-4b6b-a61e-afb90f2c605a",
                       fmiType(CS)};
    //FMU参数容器
    std::vector<FmuVar> fmuVec1;
    //FMU参数结构体
    //名称
    //索引
    //类型
    //属性
    //初值
    FmuVar var1 = {"outport[1]",
                 64,
                 Real,
                 Output,
                 double(0.0)};
    FmuVar var2 = {"outport[2]",
                 128,
                 Real,
                 Output,
                 double(0.0)};
    FmuVar var3 = {"outport[3]",
                 192,
                 Real,
                 Output,
                 double(0.0)};
    FmuVar var4 = {"outport[4]",
                 256,
                 Real,
                 Output,
                 double(0.0)};
    FmuVar var5 = {"outport[5]",
                 320,
                 Real,
                 Output,
                 double(0.0)};
    FmuVar var6 = {"outport[6]",
                 384,
                 Real,
                 Output,
                 double(0.0)};
//    FmuVar var3 = {"y",
//                 256,
//                 Real,
//                 Output,
//                 double(0.0)};
    fmuVec1.push_back(var1);
    fmuVec1.push_back(var2);
    fmuVec1.push_back(var3);
    fmuVec1.push_back(var4);
    fmuVec1.push_back(var5);
    fmuVec1.push_back(var6);

    fmuCreateInfo fmuinfo={config, fmuVec1};

    qDebug() << "startValue" << std::get<double>(fmuinfo.fmuVec.at(0).startValue);
    fmutestinfolst.push_back(fmuinfo);

    //FMU加载
    manager.load(fmutestinfolst);
    //仿真步长
    double stepsize = 0.01;
    //仿真次数
    int stepcount = 500;
    //起始时间
    double currrent_time = 0;
    //guid
    QString guid = "f4ae3756-59d3-4b6b-a61e-afb90f2c605a";
    //输入端口名称组
//    std::vector<QString> input_names = {"u"};
    //输入端口初值组
    std::vector<double> values(1.0);
    //输出端口名称组
    std::vector<QString> output_names = {"outport[1]","outport[2]","outport[3]","outport[4]","outport[5]","outport[6]"};
    //输出端口结果组
    std::vector<double> result;




    //执行方法
    for(size_t i = 0; i < stepcount; i++) {
        //设置参数/输入端口值 方法

//        manager.setReals(guid, input_names, values);
        bool stepOk = manager.dostep(guid, currrent_time, stepsize);
        //获取执行的值 方法
        if (stepOk) {
            result = manager.getReals(guid,output_names);
        } else {
            qDebug() << "doStep failed at time" << currrent_time;
            result = {};
        }
        currrent_time += stepsize;
        qDebug()<< "current time : " << currrent_time;
        qDebug()<< "result size: " <<result.size();
        qDebug()<< "result[0]: "<<result[0];
        qDebug()<< "result[1]: "<<result[1];
        qDebug()<< "result[2]: "<<result[2];
        qDebug()<< "result[3]: "<<result[3];
        qDebug()<< "result[4]: "<<result[4];
        qDebug()<< "result[5]: "<<result[5];
    }




//    for (int i = 0; i < stepcount; i++)
//    {
//        values[0] = currrent_time;
//        manager.setReals(guid, names, values);

//    }

//    manager.dostep();
    manager.terminate();






    return 0;
}
