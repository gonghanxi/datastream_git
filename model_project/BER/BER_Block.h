#ifndef BER_BLOCK_H
#define BER_BLOCK_H
#include "BER.h"
#include "Block.h"
#include "SimulationControl.h"

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
using namespace SystemVueModelBuilder;
class SYSTEMVUEMODELBUILDER_API BER_Block : public Block
{
public:
    BER_Block(const std::string& name);
    ~BER_Block() = default;

    bool Run() override;
    bool Setup() override;
    bool Initialize() override;
    bool Done() override;

    bool ModelSetup();
    void SetParameters(int SampleStart = 0, int SampleStop = 1,
                       double TimeStart = 0.0, double TimeStop = 1.0,
                       BER::SelectedStartStopOption StartStopOption = BER::Auto,
                       char* Filename = nullptr);
private:
    BER::SelectedStartStopOption ConvertStringToSelectedStartStopOption(const std::string& value);
    void CopyStringToCharPtr(const std::string& src, char*& dest);
    char* CopyStringToCharPtr(const std::string& src);
    char* combinePathWithJsonSuffix(const fs::path& linkKeyFolder, const char* m_fileName);
    void SetDefaultParameters();

    bool openFileForAppend();
    void cleanup();

    void RunDealData();
    void WriteBitShiftRegisterData(int i);


    std::unique_ptr<BER> m_ber;

    // 修改成员变量
    QFile m_qfile;  // 使用QFile替代std::ofstream
    QTextStream m_stream;

    int StatusUpdatePeriod;
    BER::SelectedStartStopOption m_StartStopOption;
    int m_SampleStart;
    int m_SampleStop;
    double m_TimeStart;
    double m_TimeStop;
    double m_sampleRate;
    char* m_fileName;


    QString m_fullPath;
    unsigned long long Index;
    double sample_time;
    std::ofstream outputFile;
    char* FileName;
    std::string m_UserId;

    //后端需要的写入路径 格式为 /01/xxx.json
    QString m_WritePath;

    SinkControl m_control;
};
RegAlgo(BER_Block);
#endif // BER_BLOCK_H
