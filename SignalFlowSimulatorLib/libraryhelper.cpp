#include "libraryhelper.h"
#include "../Common/LogExport.h"
#include <QFileInfo>

LibraryHelper::LibraryHelper()
{
}

LibraryHelper::LibraryHelper(const QString& fileName)
{
    QFileInfo fileInfo(fileName);
    if (!fileInfo.exists()) {
        LOG_ERROR("Library file not found:", fileName.toStdString());
        return;
    }

    mLib = new QLibrary(fileName);

#ifdef linux
    mLib->setLoadHints(QLibrary::ResolveAllSymbolsHint);
#endif

    if (mLib->load()) {
        createFunction = (CreateFunction)mLib->resolve("createAlgorithm");
        if (!createFunction) {
            LOG_ERROR("Failed to resolve createAlgorithm from:", fileName.toStdString());
        }
    } else {
        LOG_ERROR("Failed to load library:", fileName.toStdString());
        LOG_ERROR("Error:", mLib->errorString().toStdString());
    }
}

LibraryHelper::~LibraryHelper()
{
    if (mLib) {
        if (mLib->isLoaded()) {
            mLib->unload();
        }
        delete mLib;
    }
}

Block* LibraryHelper::create()
{
    if (createFunction) {
        return createFunction();
    } else {
        return NULL;
    }
}
