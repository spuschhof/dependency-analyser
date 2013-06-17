#include "configdto.h"

#include <QDir>

ConfigDTO::ConfigDTO() {
    this->debug = false;
    this->groups = false;
    this->paths = false;
    this->ignoreMissing = false;
    this->mergeMode = MERGE_FILE;
    this->quoteType = QUOTE_BOTH;

#ifdef Q_OS_LINUX
    this->includePaths << "/usr/include";
    this->includePaths << "/usr/local/include";
#endif

    this->srcPath = QDir::currentPath();
}
