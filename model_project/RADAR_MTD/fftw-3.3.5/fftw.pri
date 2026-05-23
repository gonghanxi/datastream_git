INCLUDEPATH += $$PWD

# 只添加头文件，不添加源文件（源文件已在 ModelDesign.pri 中添加）
HEADERS += \
    $$PWD/LapackMat.h \
    $$PWD/LapackMatBase.h

INCLUDEPATH += $$PWD/include

# Windows平台配置
win32 {
    LIBS += -L"$$PWD/lib"
    LIBS += -llibfftw3-3
    LIBS += -llibfftw3f-3
    LIBS += -llibfftw3l-3
    INCLUDEPATH += $$PWD/lib
}

# Linux平台配置
unix:!macx {
    # 注意：这里不再包含 openBlas.pri，因为 ModelDesign.pri 已经包含了
    # 只链接 FFTW 库
    LIBS += -lfftw3
    LIBS += -lfftw3f
    LIBS += -lfftw3l

    # 设置RPATH为$ORIGIN
    QMAKE_LFLAGS += -Wl,-rpath,\'\$$ORIGIN\'

    # 复制OpenBLAS库到输出目录（可选）
    OPENBLAS_LIB_SRC = /usr/lib64/libopenblas.so
    !exists($$OPENBLAS_LIB_SRC) {
        OPENBLAS_LIB_SRC = /usr/lib/x86_64-linux-gnu/libopenblas.so
    }
    !exists($$OPENBLAS_LIB_SRC) {
        OPENBLAS_LIB_SRC = /usr/lib/libopenblas.so
    }

    exists($$OPENBLAS_LIB_SRC) {
        QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$OPENBLAS_LIB_SRC) $$shell_path($$OUT_PWD) $$escape_expand(\\n\\t)
        message("OpenBLAS library will be copied from: $$OPENBLAS_LIB_SRC")
    }
}

HEADERS += \
    $$PWD/FFTPre.h

# 不添加源文件！源文件已在 ModelDesign.pri 中添加
# SOURCES += $$PWD/FFTPre.cpp
