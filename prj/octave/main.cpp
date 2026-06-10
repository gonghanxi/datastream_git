#include "mainwindow.h"
#include <QApplication>

#include "OctaveClient.h"
#include "OctaveServer.h"


int main3(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

#include <stdexcept>



// main_server.cpp
int main22() {
    try {
        OctaveServer server;
        server.run();  // 阻塞运行
    } catch (const std::exception& e) {
        std::cout << "xxxxxx" << e.what() << std::endl;
    }

    return 0;
}

//#include <cstring>


//strncpy(t, "Hello World", sizeof(t) - 1);  // 最多复制29个字符
//t[sizeof(t) - 1] = '\0';  // 确保以null结尾
//main_client.cpp
//char t[30];
#include "OctaveRun.h"
#include "test.h"
int main() {

//        main22();
//        return 0;
//    // 测试1: 基本数据类型
//    OctaveClient client(89);;
//    client.removeShmServer();
//    client.waitRemoveShmServer();
//    return 0;
    std::cout << "\n1. 测试基本数据类型..." << std::endl;
    if (test_complex_matrix()) {
        std::cout << "   ✓ 基本数据类型测试通过" << std::endl;
    } else {
        std::cout << "   ✗ 基本数据类型测试失败" << std::endl;
//        all_passed = false;
    }
    return 0;
    std::vector<std::complex<double>> v;
    v.resize(4);
    v[0]=(std::complex<double>(0.0,0.0));
    v[1]=(std::complex<double>(1.0,1.0));
    v[2]=(std::complex<double>(2.0,2.0));
    v[3]=(std::complex<double>(3.0,3.0));

    int colNum = 2;
    int rowNum = 2;
    int row = 0;
    int col = 1;
    int index = col*rowNum+row;
//    v.push

//     v.push_back();

    ComplexMatrix m(2,2);
    std::memcpy(m.fortran_vec(), v.data(), 4*sizeof(std::complex<double>));
    std::cout<<m(0,0).real()<<" "<<m(0,0).imag()<<std::endl;
    std::cout<<m(0,1).real()<<" "<<m(0,1).imag()<<std::endl;
    std::cout<<m(1,0).real()<<" "<<m(1,0).imag()<<std::endl;
    std::cout<<m(1,1).real()<<" "<<m(1,1).imag()<<std::endl;

//    ComplexMatrix m;

//    OctaveRun run;
//    main23();
//    return 0;
//    OctaveClient client(88);
//    for (int i=0; i<10; i++)
//        client.sendData(11);

//    while(1)
//    {

//    }

//    client.sendData();
//    OctaveClientInfo info;
//    info.cmpId = 8888;

//    strncpy(info.shmName, "Hello World\0", sizeof(info.shmName) - 1);

//    std::cout<<info.shmName;

//    client.sendInitInfo(info);

//    client.sendBaseInfo()

    // 发送大量数据
//    std::vector<char> large_data(10* 1024 * 1024, 'A'); // 100MB数据
//    client.send_large_data(large_data.data(), large_data.size());

//    // 接收结果
//    std::vector<char> result_buffer(1024 * 1024);
//    while (true) {
//        size_t received = client.receive_results(result_buffer.data(), result_buffer.size());
//        if (received > 0) {
//            // 处理结果
//            std::cout << "收到 " << received << " 字节结果" << std::endl;
//        } else {
//            break;
//        }
//    }

    return 0;
}
