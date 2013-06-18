#ifndef CONFIGDTO_H
#define CONFIGDTO_H

#include <QString>
#include <QStringList>

#define MERGE_FILE      0
#define MERGE_MODULE    1
#define MERGE_DIR       2

#define QUOTE_BOTH      0
#define QUOTE_ANGLE     1
#define QUOTE_QUOTE     2

class ConfigDTO {
public:
    ConfigDTO();

    bool debug;
    bool groups;
    bool ignoreMissing;
    bool colorize;
    int mergeMode;
    int quoteType;
    QString excludeRegEx;
    QString srcPath;
    QStringList includePaths;
};

#endif // CONFIGDTO_H
