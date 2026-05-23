#include "MATLAB_Script_Block.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>

MATLAB_Script_Block::MATLAB_Script_Block(const std::string& name)
    :Block(name)
{
    ep =engOpen(NULL);

}

MATLAB_Script_Block::~MATLAB_Script_Block()
{
    engClose(ep);
}

bool MATLAB_Script_Block::Setup()
{
    Block::Setup();
    return true;
}

// MATLAB_Script_Block.cpp
bool MATLAB_Script_Block::Run()
{

    std::cout << "MATLAB_Script_Block Run start"<<std::endl;
    if(!ep)
        return false;

    QMap<int, PortMsg> ports=Block::getPortsMsg();

    QMap<QString,mxArray *> inputs;
    for(auto port : ports) {
        if(port.putType == "in") {
            QString portName=QString("%1_%2").arg(getInstanceName().c_str()).arg(port.name);
            //portName=port.name;
            std::cout << "MATLAB_Script_Block Run portName:"<<portName.toStdString()<<std::endl;

            //            auto inputPortData = ReadInputData<std::complex<double>>(portName.toStdString());

            if(port.dataType==PortMsg::REAL||port.dataType==PortMsg::INT)
            {
                auto inputPortData = ReadInputData<double>(port.name.toStdString());
                std::cout << "MATLAB_Script_Block Run inputPortData size:"<<inputPortData.size()<<std::endl;
                if (!inputPortData.empty())
                {
                    mxArray *inputArray = mxCreateDoubleMatrix(inputPortData.size(), 1, mxREAL);
                    inputs[port.name]=inputArray;
                    mxDouble *inputData = mxGetDoubles(inputArray);
                    qDebug() << QString("MATLAB_Script_Block Run set input double data[%1]:%2").arg(port.name).arg(inputPortData.size());
                    for (int i = 0; i < inputPortData.size(); i++) {
                        inputData[i] = inputPortData[i];
                    }
                }
            }else if(port.dataType==PortMsg::COMPLEX)
            {
                auto inputPortData = ReadInputData<std::complex<double>>(port.name.toStdString());
                if (!inputPortData.empty())
                {
                    mxArray *inputArray = mxCreateDoubleMatrix(inputPortData.size(), 1, mxCOMPLEX);
                    inputs[port.name]=inputArray;
                    mxComplexDouble *inputData = mxGetComplexDoubles(inputArray);
                    qDebug() << QString("MATLAB_Script_Block Run set input complex data[%1]:%2").arg(port.name).arg(inputPortData.size());
                    for (int i = 0; i < inputPortData.size(); i++) {
                        inputData[i].real = inputPortData[i].real();
                        inputData[i].imag = inputPortData[i].imag();
                    }
                }
            }else if(port.dataType==PortMsg::INT_MATRIX||port.dataType==PortMsg::REAL_MATRIX)
            {
//                inline size_t NumRows() const
//                {
//                    return Size(0);
//                }

//                /// Return the number of columns
//                inline size_t NumColumns() const
//                {
//                    return Size(1);
//                }
                auto inputPortData = ReadInputData<DoubleMatrix>(port.name.toStdString());
                std::cout << "MATLAB_Script_Block Run inputPortData size:"<<inputPortData.size()<<std::endl;
                if (!inputPortData.empty())
                {
                    mxArray *cell = mxCreateCellMatrix(inputPortData.size(), 1);
                    inputs[port.name]=cell;

                    //mxDouble *inputData = mxGetDoubles(inputArray);
                    //qDebug() << QString("MATLAB_Script_Block Run set input DoubleMatrix data[%1]:%2").arg(port.name).arg(inputPortData.size());
                    for (int i = 0; i < inputPortData.size(); i++) {
                        Matrix<double> matVue = inputPortData[i];
                        int rows=matVue.NumRows();
                        int cols=matVue.NumColumns();
                        mxArray *mat = mxCreateDoubleMatrix(rows, cols, mxREAL);

                        double *p = mxGetPr(mat);
                        for(int r = 0; r < rows; r++){
                            for(int c = 0; c < cols; c++){
                                p[r + c*rows] = matVue(0,0);
                            }
                        }

                        mxSetCell(cell, i, mat);
                    }
                }
            }else if(port.dataType==PortMsg::COMPLEX_MATRIX)
            {
                auto inputPortData = ReadInputData<DComplexMatrix>(port.name.toStdString());
                //std::cout << "MATLAB_Script_Block Run inputPortData size:"<<inputPortData.size()<<std::endl;
                if (!inputPortData.empty())
                {
//                    mxArray *inputArray = mxCreateDoubleMatrix(inputPortData.size(), 1, mxREAL);
//                    inputs[port.name]=inputArray;
//                    mxDouble *inputData = mxGetDoubles(inputArray);
//                    qDebug() << QString("MATLAB_Script_Block Run set input DComplexMatrix data[%1]:%2").arg(port.name).arg(inputPortData.size());
//                    for (int i = 0; i < inputPortData.size(); i++) {
//                        inputData[i] = inputPortData[i];
//                    }
                }
            }
        }
        else if(port.putType == "out") {

        }
    }
    QString appPath = QCoreApplication::applicationDirPath();
    QString folderPath = appPath + "/m";
    engEvalString(ep, QString("addpath('%1')").arg(folderPath).toStdString().c_str());
    qDebug() << QString("MATLAB_Script_Block Run2");
    for(auto key:inputs.keys())
    {
        engPutVariable(ep, key.toStdString().c_str(),inputs[key]);
    }
    qDebug() << QString("MATLAB_Script_Block Run3");
    int ret=engEvalString(ep, QString("%1;").arg(callStr).toStdString().c_str());//output=M1_runfc(1)
    for(auto port : ports) {
        if(port.putType == "in") {

        }
        else if(port.putType == "out") {
            qDebug() << QString("MATLAB_Script_Block Run get out value:%1").arg(port.name);
            //QString portName=QString("%1_%2").arg(getInstanceName().c_str()).arg(port.name);
            mxArray *outputArray = engGetVariable(ep, port.name.toStdString().c_str());
            if(outputArray)
            {
                qDebug() << QString("MATLAB_Script_Block Run5");
                if(port.dataType==PortMsg::REAL||port.dataType==PortMsg::INT)
                {
                    mxDouble *resultData = mxGetDoubles(outputArray);

                    int resultDataSize = mxGetNumberOfElements(outputArray);
                    std::cout << "MATLAB_Script_Block Run resultDataSize size:"<<resultDataSize<<std::endl;
                    std::vector<double> outputData;

                    for (int i = 0; i < resultDataSize; ++i)
                    {
                        outputData.push_back(resultData[i]);
                    }
                    qDebug() << QString("MATLAB_Script_Block Run get output double data[%1]:%2").arg(port.name).arg(outputData.size());
                    WriteOutputData(port.name.toStdString().c_str(), outputData);
                }else if(port.dataType==PortMsg::COMPLEX)
                {
                    mxComplexDouble *resultData = mxGetComplexDoubles(outputArray);

                    int resultDataSize = mxGetNumberOfElements(outputArray);
                    std::vector<std::complex<double>> outputData;

                    for (int i = 0; i < resultDataSize; ++i)
                    {
                        outputData.push_back({resultData[i].real,resultData[i].imag});
                    }
                    qDebug() << QString("MATLAB_Script_Block Run get output complex data[%1]:%2").arg(port.name).arg(outputData.size());
                    WriteOutputData(port.name.toStdString().c_str(), outputData);
                }else if(port.dataType==PortMsg::INT_MATRIX||port.dataType==PortMsg::REAL_MATRIX)
                {
                    mxDouble *resultData = mxGetDoubles(outputArray);

                    int resultDataSize = mxGetNumberOfElements(outputArray);
                    std::cout << "MATLAB_Script_Block Run resultDataSize size:"<<resultDataSize<<std::endl;
                    std::vector<double> outputData;

                    for (int i = 0; i < resultDataSize; ++i)
                    {
                        outputData.push_back(resultData[i]);
                    }
                    qDebug() << QString("MATLAB_Script_Block Run get output REAL_MATRIX data[%1]:%2").arg(port.name).arg(outputData.size());
                    WriteOutputData(port.name.toStdString().c_str(), outputData);
                }else if(port.dataType==PortMsg::COMPLEX_MATRIX)
                {
                    mxDouble *resultData = mxGetDoubles(outputArray);

                    int resultDataSize = mxGetNumberOfElements(outputArray);
                    std::cout << "MATLAB_Script_Block Run resultDataSize size:"<<resultDataSize<<std::endl;
                    std::vector<double> outputData;

                    for (int i = 0; i < resultDataSize; ++i)
                    {
                        outputData.push_back(resultData[i]);
                    }
                    qDebug() << QString("MATLAB_Script_Block Run get output COMPLEX_MATRIX data[%1]:%2").arg(port.name).arg(outputData.size());
                    WriteOutputData(port.name.toStdString().c_str(), outputData);
                }
                qDebug() << QString("MATLAB_Script_Block Run7");
                //                WriteOutputData(portName.toStdString().c_str(), outputData);

                mxDestroyArray(outputArray);
            }
        }
    }
    qDebug() << QString("MATLAB_Script_Block Run8");
    for(auto v:inputs.values())
    {
        mxDestroyArray(v);
    }
    std::cout << "MATLAB_Script_Block Run end"<<std::endl;

    //    std::string inputPortName = GetInputPortName(0);
    //    inputData = ReadInputData<double>(inputPortName);

    //    if (inputData.empty()) {
    //        return false;
    //    }

    //    double m_gain;
    //    m_gain = std::stod(getParameter("Gain").Value);
    //    for(size_t i = 0; i < inputData.size(); i++) {
    //        double outputSample = m_gain * inputData[i];
    //        outputData.push_back(outputSample);
    //    }



    //    qDebug() << "MATLAB_Script_Block: " << outputData.size();
    //    // 写入输出
    //    std::string outputPortName = GetOutputPortName(0);

    //    WriteOutputData(outputPortName, outputData);
    return true;
}

bool MATLAB_Script_Block::Initialize()
{
    qDebug() << "MATLAB_Script_Block Initialize start";
    QString appPath = QCoreApplication::applicationDirPath();
    QString folderPath = appPath + "/m";
    QDir dir(folderPath);
    if(!dir.exists())
    {
        dir.mkdir(folderPath);
    }
    qDebug() << "appPath: " << appPath;
    std::string Equations = getParameter("Equations").Value;

    if(!Equations.empty())
    {
        QMap<int, PortMsg> ports=getPortsMsg();
        qDebug() << "MATLAB_Script_Block Initialize ports size: " << ports.size();
        QStringList inputs;
        QStringList outputs;
        int i = 0;
        int j = 0;
        for(auto port : ports) {
            qDebug() << "MATLAB_Script_Block Initialize dataType: " << port.dataType;
            if(port.putType == "in") {
                if(port.dataType==PortMsg::REAL||port.dataType==PortMsg::INT)
                {
                    DoubleCircularBuffer *a=new DoubleCircularBuffer;
                    AddInputPort(port.name.toStdString(),*a,port.portRate,DataType::CIRCULAR_BUFFER_DOUBLE);

                     qDebug() << "MATLAB_Script_Block Initialize add input double port Name: " << QString::fromStdString(GetInputPortName(i));
                }else if(port.dataType==PortMsg::COMPLEX)
                {
                    DComplexCircularBuffer *a=new DComplexCircularBuffer;
                    AddInputPort(port.name.toStdString(),*a,port.portRate,DataType::CIRCULAR_BUFFER_DCOMPLEX);
                    qDebug() << "MATLAB_Script_Block Initialize add input complex port Name: " << QString::fromStdString(GetInputPortName(i));
                }else if(port.dataType==PortMsg::INT_MATRIX||port.dataType==PortMsg::REAL_MATRIX)
                {
                    DoubleMatrixCircularBuffer *a=new DoubleMatrixCircularBuffer;
                    AddInputPort(port.name.toStdString(),*a,port.portRate,DataType::MATRIX_DOUBLE);
                    qDebug() << "MATLAB_Script_Block Initialize add input MATRIX port Name: " << QString::fromStdString(GetInputPortName(i));

                }else if(port.dataType==PortMsg::COMPLEX_MATRIX)
                {
                    DComplexMatrixCircularBuffer *a=new DComplexMatrixCircularBuffer;
                    AddInputPort(port.name.toStdString(),*a,port.portRate,DataType::MATRIX_DCOMPLEX);
                    qDebug() << "MATLAB_Script_Block Initialize add input MATRIX_DCOMPLEX port Name: " << QString::fromStdString(GetInputPortName(i));

                }
                inputs.append(port.name);

                i++;
            }
            else if(port.putType == "out") {
                if(port.dataType==PortMsg::REAL||port.dataType==PortMsg::INT)
                {
                    DoubleCircularBuffer *a=new DoubleCircularBuffer;
                    AddOutputPort(port.name.toStdString(),*a,port.portRate,DataType::CIRCULAR_BUFFER_DOUBLE);
                   qDebug() << "MATLAB_Script_Block Initialize add output double port Name: " << QString::fromStdString(GetOutputPortName(j));
                }else if(port.dataType==PortMsg::COMPLEX)
                {
                    DComplexCircularBuffer *a=new DComplexCircularBuffer;
                    AddOutputPort(port.name.toStdString(),*a,port.portRate,DataType::CIRCULAR_BUFFER_DCOMPLEX);
                    qDebug() << "MATLAB_Script_Block Initialize add output complex port Name: " << QString::fromStdString(GetOutputPortName(j));
                }else if(port.dataType==PortMsg::INT_MATRIX||port.dataType==PortMsg::REAL_MATRIX)
                {
                    DoubleMatrixCircularBuffer *a=new DoubleMatrixCircularBuffer;
                    AddOutputPort(port.name.toStdString(),*a,port.portRate,DataType::MATRIX_DOUBLE);
                }else if(port.dataType==PortMsg::COMPLEX_MATRIX)
                {
                    DComplexMatrixCircularBuffer *a=new DComplexMatrixCircularBuffer;
                    AddInputPort(port.name.toStdString(),*a,port.portRate,DataType::MATRIX_DCOMPLEX);
                    qDebug() << "MATLAB_Script_Block Initialize add output MATRIX_DCOMPLEX port Name: " << QString::fromStdString(GetInputPortName(i));

                }
                outputs.append(port.name);
                //qDebug() << "MATLAB_Script_Block Initialize outport Name: " << QString::fromStdString(GetOutputPortName(j));
                j++;
            }

        }
        //qDebug() << "MATLAB_Script_Block Initialize ports size1: " << ports.size();
        std::map<std::string,Parameter> allparameters=getAllParameter();
        for(auto e:allparameters)
        {
            std::string Name=e.second.Name;

            if(Name!="Equations")
            {
                inputs.append(Name.c_str());
                double value=std::stod(e.second.Value);
                engPutVariable(ep, Name.c_str(), mxCreateDoubleScalar(value));
                qDebug() << QString("MATLAB_Script_Block Initialize add param:%1=%2").arg(Name.c_str()).arg(value);
            }

        }
        //qDebug() << "MATLAB_Script_Block Initialize ports size2: " << ports.size();
        QString mStr="function ";
        callStr="";
        if(outputs.size()==1)
        {
            callStr.append(outputs[0]);
        }else if(outputs.size()>1)
        {
            callStr.append("[");
            callStr.append(outputs.join(","));
            callStr.append("]");
        }
        callStr.append(QString("=%1_runfc").arg(getInstanceName().c_str()));
        callStr.append("(");
        callStr.append(inputs.join(","));
        callStr.append(")");
        mStr.append(callStr);
        mStr.append("\n");
        mStr.append(Equations.c_str());
        mStr.append("\n");
        mStr.append("end\n");

        QString filePath=QString("%1/%2_runfc.m").arg(folderPath).arg(getInstanceName().c_str());
        //qDebug() << "MATLAB_Script_Block ports Initialize filePath: " << filePath;
        //qDebug() << "MATLAB_Script_Block ports Initialize Script: " << mStr;
        QFile file(filePath);
        // 2. 以“只写+文本模式”打开文件（WriteOnly：只写，Text：自动处理换行符）
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "文件打开失败：" << file.errorString();
            return false;
        }
        //qDebug() << "MATLAB_Script_Block Initialize ports size3: " << ports.size();
        // 3. 创建 QTextStream 用于文本写入（简化编码和换行处理）
        QTextStream out(&file);
        // 设置编码（推荐 UTF-8，避免中文乱码）
        out.setCodec("UTF-8");
        // 写入内容
        out << mStr;

        // 4. 手动关闭文件（也可依赖 QFile 析构自动关闭，建议显式关闭）
        file.close();

    }


    m_addCx = std::make_unique<MATLAB_Script>();
    SetBlockType(Block::BlockType::PROCESSOR);

    qDebug() <<"......................................................................................."<< QString::fromStdString(Equations);
    SetDefaultParameters();

    SetParameters();
    qDebug() << "MATLAB_Script_Block Initialize end";
    return true;
}

void MATLAB_Script_Block::SetParameters()
{

}

void MATLAB_Script_Block::SetDefaultParameters()
{

}
