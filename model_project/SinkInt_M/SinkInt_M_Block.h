#ifndef SINKINT_M_BLOCK_H
#define SINKINT_M_BLOCK_H
#include "SinkInt_M.h"
#include "Block.h"
using namespace SystemVueModelBuilder;

#ifdef __has_include
    #if __has_include(<filesystem>)
        #include <filesystem>
        namespace fs = std::filesystem;
    #elif __has_include(<experimental/filesystem>)
        #include <experimental/filesystem>
        namespace fs = std::experimental::filesystem;
    #else
        #error "Missing filesystem support"
    #endif
#else
    // 对于不支持 __has_include 的编译器
    #if defined(_WIN32) || defined(__cpp_lib_filesystem)
        #include <filesystem>
        namespace fs = std::filesystem;
    #else
        #include <experimental/filesystem>
        namespace fs = std::experimental::filesystem;
    #endif
#endif

class SYSTEMVUEMODELBUILDER_API SinkInt_M_Block : public Block
{
public:
    SinkInt_M_Block(const std::string& name);
    ~SinkInt_M_Block();
    bool Setup() override;
    bool Run() override;
    bool Initialize() override;
    bool Done() override;

    void SetParameters();

private:

    SinkInt_M::SelectedStartStopOption ConvertStringToSelected(const std::string& value);
    void CopyStringToCharPtr(const std::string& src, char*& dest);
    char* CopyStringToCharPtr(const std::string& src);
    char* combinePathWithJsonSuffix(const fs::path& linkKeyFolder, const char* m_fileName);
    void SetDefaultParameters();

    bool openFileForAppend();
    void cleanup();

    void RunDealData();
    void WriteBitShiftRegisterData(int i);

    //SinkTime格式修改
    QString formatSinkTime(double timeValue) const;
    double roundToPrecision(double value, int decimals) const;

    // 修改成员变量
    QFile m_qfile;  // 使用QFile替代std::ofstream
    QTextStream m_stream;

    std::unique_ptr<SinkInt_M> m_sink;

    SinkInt_M::SelectedStartStopOption m_StartStopOption;
    int m_SampleStart;
    int m_SampleStop;
    double m_TimeStart;
    double m_TimeStop;
    double m_sampleRate;
    char* m_fileName;


    QString m_fullPath;
    unsigned long long Index;
    double sample_time;


    size_t m_iBuffer;
    IntMatrix* m_pdBuffer;
    int numCols;
    int numRows;

    std::ofstream outputFile;
    char* FileName;
    std::string m_UserId;

    //后端需要的写入路径 格式为 /01/xxx.json
    QString m_WritePath;

};

RegAlgo(SinkInt_M_Block);
#endif // SINKINT_M_BLOCK_H
