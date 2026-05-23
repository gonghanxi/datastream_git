#ifndef DFERRORHANDLER_H
#define DFERRORHANDLER_H


namespace SystemVueModelBuilder {
    class DFModel;
    class DFErrorHandler
    {
    public:
        ///将错误发布到错误窗格和模拟日志
        static void PostError (DFModel *pModel, const char* pcError);
        ///向错误窗格和模拟日志发布警告
        static void PostWarning (DFModel *pModel, const char* pcWarning);
        ///向错误窗格和模拟日志发布信息消息
        static void PostInfo (DFModel *pModel, const char* pcMessage);
        ///向模拟日志发送
        static void PostLog (DFModel *pModel, const char* pcMessage);
        ///向模拟状态窗口发送
        static void PostProgress (DFModel *pModel, const char* pcMessage);
        ///清除发送到模拟状态窗口的消息
        static void ClearProgress (DFModel *pModel);
        ///模型停止响应时返回true
        static bool StopRequested();

        static bool ErrorOccurred();
    };
    }
#endif // DFERRORHANDLER_H
