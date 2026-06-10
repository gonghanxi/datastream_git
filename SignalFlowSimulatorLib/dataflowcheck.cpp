#include "dataflowcheck.h"

DataFlowCheck::DataFlowCheck()
{

}

DataFlowCheck::~DataFlowCheck()
{

}

bool DataFlowCheck::portPutTypeCheck(const QString &putTypeStart, const QString &putTypeEnd)
{
    if((putTypeStart=="in"&&putTypeEnd=="in") ||
            (putTypeStart=="out"&&putTypeEnd=="out"))
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool DataFlowCheck::portDataTypeCheck(PortMsg::PortDataType dataTypeStart, PortMsg::PortDataType dataTypeEnd)
{
    //1.matrix不能连非matrix
    if(
            (
                dataTypeStart>=PortMsg::INT && dataTypeStart<=PortMsg::MULTIPLE_VARIANT &&
                dataTypeEnd>=PortMsg::INT_MATRIX && dataTypeEnd<=PortMsg::MULTIPLE_VARIANT_MATRIX
                )
            ||
            (
                dataTypeStart>=PortMsg::INT_MATRIX && dataTypeStart<=PortMsg::MULTIPLE_VARIANT_MATRIX &&
                dataTypeEnd>=PortMsg::INT && dataTypeEnd<=PortMsg::MULTIPLE_VARIANT
                )
            )
    {
        qDebug() << "1.matrix不能连非matrix" << endl;
        return false;
    }

    //2.多不能到单
    if(dataTypeStart>=PortMsg::MULTIPLE_INT&&dataTypeStart<=PortMsg::MULTIPLE_VARIANT
            &&dataTypeEnd>=PortMsg::INT&&dataTypeEnd<=PortMsg::VARIANT)
    {
        qDebug() << "2.多不能到单" << endl;
        return false;
    }

    //3.单-单 单-多
    if(
            //Int 类型作为起点
            //不允许连接Complex Envelope Multiple_Complex Multiple_Envelope
            dataTypeStart==PortMsg::INT&&(
                dataTypeEnd==PortMsg::COMPLEX||
                dataTypeEnd==PortMsg::ENVELOPE||
                dataTypeEnd==PortMsg::MULTIPLE_COMPLEX||
                dataTypeEnd==PortMsg::MULTIPLE_ENVELOPE
                )
            //Complex 类型作为起点
            //允许连接Complex Anytype Variant Multiple_Complex Multiple_Anytype Multiple_Varaint
            || dataTypeStart==PortMsg::COMPLEX&& !(
                dataTypeEnd==PortMsg::COMPLEX||
                dataTypeEnd==PortMsg::ANYTYPE||
                dataTypeEnd==PortMsg::VARIANT||
                dataTypeEnd==PortMsg::MULTIPLE_COMPLEX||
                dataTypeEnd==PortMsg::MULTIPLE_ANYTYPE||
                dataTypeEnd==PortMsg::MULTIPLE_VARIANT
                )
            //Envelope 类型作为起点
            //允许连接Envelope Anytype Variant Multiple_Envelope Multiple_Anytype Multiple_Varaint
            || dataTypeStart==PortMsg::ENVELOPE&& !(
                dataTypeEnd==PortMsg::ENVELOPE||
                dataTypeEnd==PortMsg::ANYTYPE||
                dataTypeEnd==PortMsg::VARIANT||
                dataTypeEnd==PortMsg::MULTIPLE_ENVELOPE||
                dataTypeEnd==PortMsg::MULTIPLE_ANYTYPE||
                dataTypeEnd==PortMsg::MULTIPLE_VARIANT
                )

            //Real 类型作为起点
            //允许连接Real Anytype Variant Multiple_Real Multiple_Anytype Multiple_Varaint
            || dataTypeStart==PortMsg::REAL&& !(
                dataTypeEnd==PortMsg::REAL||
                dataTypeEnd==PortMsg::ANYTYPE||
                dataTypeEnd==PortMsg::VARIANT||
                dataTypeEnd==PortMsg::MULTIPLE_REAL||
                dataTypeEnd==PortMsg::MULTIPLE_ANYTYPE||
                dataTypeEnd==PortMsg::MULTIPLE_VARIANT
                )

            //FixPoint 类型作为起点
            //允许连接FixPoint Real Anytype Variant Multiple_FixPoint Multiple_Real Multiple_Anytype Multiple_Variant
            || dataTypeStart==PortMsg::FIXEDPOINT&& !(
                dataTypeEnd==PortMsg::FIXEDPOINT||
                dataTypeEnd==PortMsg::REAL||
                dataTypeEnd==PortMsg::ANYTYPE||
                dataTypeEnd==PortMsg::VARIANT||
                dataTypeEnd==PortMsg::MULTIPLE_FIXEDPOINT||
                dataTypeEnd==PortMsg::MULTIPLE_REAL||
                dataTypeEnd==PortMsg::MULTIPLE_ANYTYPE||
                dataTypeEnd==PortMsg::MULTIPLE_VARIANT
                )
            )
    {
        qDebug() << "3.单-单 单-多" << endl;
        return false;
    }

    //4.多到多
    if(
            //Multiple_Int 类型作为起点
            //不允许连接Multiple_Complex Multiple_Envelope
            dataTypeStart==PortMsg::MULTIPLE_INT&&(
                dataTypeEnd==PortMsg::MULTIPLE_COMPLEX||
                dataTypeEnd==PortMsg::MULTIPLE_ENVELOPE
                )
            //Multiple_Complex 类型作为起点
            //允许连接Multiple_Complex Multiple_Anytype Multiple_Variant
            || dataTypeStart==PortMsg::MULTIPLE_COMPLEX&&
            dataTypeEnd!=PortMsg::MULTIPLE_COMPLEX&&
            dataTypeEnd!=PortMsg::MULTIPLE_ANYTYPE&&
            dataTypeEnd!=PortMsg::MULTIPLE_VARIANT
            //Multiple_Envelope 类型作为起点
            //允许连接Multiple_Envelope Multiple_Anytype Multiple_Variant
            || dataTypeStart==PortMsg::MULTIPLE_ENVELOPE&&
            dataTypeEnd!=PortMsg::MULTIPLE_ENVELOPE&&
            dataTypeEnd!=PortMsg::MULTIPLE_ANYTYPE&&
            dataTypeEnd!=PortMsg::MULTIPLE_VARIANT
            //Multiple_Real 类型作为起点
            //允许连接Multiple_Real Multiple_Anytype Multiple_Variant
            || dataTypeStart==PortMsg::MULTIPLE_REAL&&
            dataTypeEnd!=PortMsg::MULTIPLE_REAL&&
            dataTypeEnd!=PortMsg::MULTIPLE_ANYTYPE&&
            dataTypeEnd!=PortMsg::MULTIPLE_VARIANT
            //Multiple_FixPoint 类型作为起点
            //允许连接Multiple_FixPoint Multiple_Real Multiple_Anytype Multiple_Variant
            || dataTypeStart==PortMsg::MULTIPLE_FIXEDPOINT&&
            dataTypeEnd!=PortMsg::MULTIPLE_FIXEDPOINT&&
            dataTypeEnd!=PortMsg::MULTIPLE_REAL&&
            dataTypeEnd!=PortMsg::MULTIPLE_ANYTYPE&&
            dataTypeEnd!=PortMsg::MULTIPLE_VARIANT)
    {
        qDebug() << "4.多到多" << endl;
        return false;
    }

    //5.多matrix不能到单matrix
    if(dataTypeStart>=PortMsg::MULTIPLE_INT_MATRIX&&dataTypeStart<=PortMsg::MULTIPLE_VARIANT_MATRIX
            &&dataTypeEnd>=PortMsg::INT_MATRIX&&dataTypeEnd<=PortMsg::VARIANT_MATRIX)
    {
        qDebug() << "5.多matrix不能到单matrix" << endl;
        return false;
    }

    //6.单matrix到单matrix
    if(
            //Matrix_Int 类型作为起点
            //不允许连接Matrix_Complex Matrix_Envelope
            //         Matrix_Multiple_Complex Matrix_Multiple_Envelope
            dataTypeStart==PortMsg::INT_MATRIX&&(
                dataTypeEnd==PortMsg::COMPLEX_MATRIX||
                dataTypeEnd==PortMsg::ENVELOPE_MATRIX||
                dataTypeEnd==PortMsg::MULTIPLE_COMPLEX_MATRIX||
                dataTypeEnd==PortMsg::MULTIPLE_ENVELOPE_MATRIX
                )
            //Matrix_Complex 类型作为起点
            //允许连接Matrix_Complex Matrix_Anytype Matrix_Variant
            //       Matrix_Multiple_Complex Matrix_Multiple_Anytype Matrix_Multiple_Variant
            || dataTypeStart==PortMsg::COMPLEX_MATRIX&&
            dataTypeEnd!=PortMsg::COMPLEX_MATRIX&&
            dataTypeEnd!=PortMsg::ANYTYPE_MATRIX&&
            dataTypeEnd!=PortMsg::VARIANT_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_COMPLEX_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_ANYTYPE_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_VARIANT_MATRIX
            //Matrix_Envelope 类型作为起点
            //允许连接Matrix_Envelope Matrix_Anytype Matrix_Variant
            //       Matrix_Multiple_Envelope Matrix_Multiple_Anytype Matrix_Multiple_Variant
            || dataTypeStart==PortMsg::ENVELOPE_MATRIX&&
            dataTypeEnd!=PortMsg::ENVELOPE_MATRIX&&
            dataTypeEnd!=PortMsg::ANYTYPE_MATRIX&&
            dataTypeEnd!=PortMsg::VARIANT_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_ENVELOPE_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_ANYTYPE_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_VARIANT_MATRIX
            //Matrix_Real 类型作为起点
            //允许连接Matrix_Real Matrix_Anytype Matrix_Variant
            //       Matrix_Multiple_Real Matrix_Multiple_Anytype Matrix_Multiple_Variant
            || dataTypeStart==PortMsg::REAL_MATRIX&&
            dataTypeEnd!=PortMsg::REAL_MATRIX&&
            dataTypeEnd!=PortMsg::ANYTYPE_MATRIX&&
            dataTypeEnd!=PortMsg::VARIANT_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_REAL_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_ANYTYPE_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_VARIANT_MATRIX
            //Matrix_FixPoint 类型作为起点
            //允许连接Matrix_FixPoint Matrix_Real Matrix_Anytype Matrix_Variant
            //       Matrix_Multiple_FixPoint Matrix_Multiple_Real Matrix_Multiple_Anytype Matrix_Multiple_Variant
            || dataTypeStart==PortMsg::FIXEDPOINT_MATRIX&&
            dataTypeEnd!=PortMsg::FIXEDPOINT_MATRIX&&
            dataTypeEnd!=PortMsg::REAL_MATRIX&&
            dataTypeEnd!=PortMsg::ANYTYPE_MATRIX&&
            dataTypeEnd!=PortMsg::VARIANT_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_FIXEDPOINT_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_REAL_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_ANYTYPE_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_VARIANT_MATRIX)
    {
        qDebug() << "6.单matrix到单matrix" << endl;
        return false;
    }

    //7.多matrix到多martrix
    if(
            //Matrix_Multiple_Int 类型作为起点
            //不允许连接Matrix_Multiple_Complex Matrix_Multiple_Envelope
            dataTypeStart==PortMsg::MULTIPLE_INT_MATRIX&&(
                dataTypeEnd==PortMsg::MULTIPLE_COMPLEX_MATRIX||
                dataTypeEnd==PortMsg::MULTIPLE_ENVELOPE_MATRIX
                )
            //Matrix_Multiple_Complex 类型作为起点
            //允许连接Matrix_Multiple_Complex Matrix_Multiple_Anytype Matrix_Multiple_Variant
            || dataTypeStart==PortMsg::MULTIPLE_COMPLEX_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_COMPLEX_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_ANYTYPE_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_VARIANT_MATRIX
            //Matrix_Multiple_Envelope 类型作为起点
            //允许连接Matrix_Multiple_Envelope Matrix_Multiple_Anytype Matrix_Multiple_Variant
            || dataTypeStart==PortMsg::MULTIPLE_ENVELOPE_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_ENVELOPE_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_ANYTYPE_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_VARIANT_MATRIX
            //Matrix_Multiple_Real 类型作为起点
            //允许连接Matrix_Multiple_Real Matrix_Multiple_Anytype Matrix_Multiple_Variant
            || dataTypeStart==PortMsg::MULTIPLE_REAL_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_REAL_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_ANYTYPE_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_VARIANT_MATRIX
            //Matrix_Multiple_FixPoint 类型作为起点
            //允许连接Matrix_Multiple_FixPoint Matrix_Multiple_Real Matrix_Multiple_Anytype Matrix_Multiple_Variant
            || dataTypeStart==PortMsg::MULTIPLE_FIXEDPOINT_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_FIXEDPOINT_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_REAL_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_ANYTYPE_MATRIX&&
            dataTypeEnd!=PortMsg::MULTIPLE_VARIANT_MATRIX
            )
    {
        qDebug() << "7.多matrix到多martrix" << endl;
        return false;
    }

    return true;
}
