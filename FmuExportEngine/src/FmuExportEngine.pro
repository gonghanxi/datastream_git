QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = FmuExportEngine

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        codegenerator.cpp \
        compiler.cpp \
        fmupacker.cpp \
        main.cpp \
        modelparser.cpp

HEADERS += \
    codegenerator.h \
    compiler.h \
    fmi3/fmi3FunctionTypes.h \
    fmi3/fmi3Functions.h \
    fmi3/fmi3PlatformTypes.h \
    fmupacker.h \
    modelparser.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# ==============================================
# 配置编译输出目录：bin/Debug 或 bin/Release
# ==============================================

# 1. 定义根目录（当前pro文件所在目录）
ROOT_DIR = $$PWD

# 2. 输出目录规则：根据构建模式自动区分
CONFIG(debug, debug|release) {
    # Debug 模式
    DESTDIR = $$ROOT_DIR/../bin/Debug         # 可执行文件输出路径
} else {
    # Release 模式
    DESTDIR = $$ROOT_DIR/../bin/Release
}

# 拷贝fmi3文件（跨平台）
win32 {
    # Windows：使用 xcopy（/E 拷贝子目录，/I 目标为目录，/Y 覆盖）
    COPY_CMD = xcopy /E /I /Y $$shell_path($$ROOT_DIR/fmi3) $$shell_path($$DESTDIR/fmi3)
} else {
    # Linux/macOS：使用 cp -rf（强制递归覆盖）
    COPY_CMD = cp -rf $$ROOT_DIR/fmi3 $$DESTDIR/
}
# 在链接（构建）完成后执行拷贝
QMAKE_POST_LINK += $$COPY_CMD
