INCLUDEPATH += $$PWD
#DEPENDPATH += $$PWD

DEFINES += ADD_
DEFINES += HAVE_LAPACK_CONFIG_H
DEFINES += LAPACK_COMPLEX_STRUCTURE

HEADERS += \
    $$PWD/LapackMat.h \
    $$PWD/LapackMatBase.h

SOURCES += \
    $$PWD/LapackMat.cpp \
    $$PWD/LapackMatBase.cpp


# windows
win32{
    LIBS += -L"$$PWD/lib/x64_release/openblas/bin"
    LIBS += -L"$$PWD/lib/x64_release/openblas/lib"
    LIBS +=  -llibopenblas
    #LIBS +=  -lLAPACK
    INCLUDEPATH +=  $$PWD/lib/x64_release/openblas/include

}

unix{
    LIBS += -lopenblas
}
