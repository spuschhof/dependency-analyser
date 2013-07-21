/***************************************************************************
 *   Copyright (C) 2013 by Sebastian Puschhof                              *
 *   dev@puschhof.de                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef CONFIGDTO_H
#define CONFIGDTO_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QTextStream>

#define MERGE_FILE      0
#define MERGE_MODULE    1
#define MERGE_DIR       2

#define QUOTE_BOTH      0
#define QUOTE_ANGLE     1
#define QUOTE_QUOTE     2

#define PROV_MERGE      0x01
#define PROV_QUOTE      0x02
#define PROV_VALUE      0x04
#define PROV_SAT        0x08

#define OPT_UNKNOWN -1
#define OPT_HELP    0
#define OPT_DEBUG   1
#define OPT_GROUPS  2
#define OPT_IGNMIS  3
#define OPT_COLOR   4
#define OPT_KEEP    5
#define OPT_EXCLUDE 100
#define OPT_MERGE   101
#define OPT_INCLUDE 102
#define OPT_QUOTES  103
#define OPT_SRC     104
#define OPT_EXCLINC 105
#define OPT_VALUE   106
#define OPT_SAT     107
#define OPT_COLOR_NODES 108
#define OPT_CONFIG  109

#define OPT_PARAM   100

class ConfigDTO {
public:
    ConfigDTO();

    bool debug;
    bool groups;
    bool ignoreMissing;
    bool colorize;
    bool keepPaths;
    bool colorNodes;
    int mergeMode;
    int quoteType;
    int value;
    int saturation;
    QString excludeRegEx;
    QString excludeIncludeRegEx;
    QString srcPath;
    QStringList includePaths;
    QMap<int, QString> nodeColorMap;

    unsigned int cmdProvided;

    QString getNodeColor(int dependencies) const;

    static ConfigDTO parseConfigFile(const QString& file, const ConfigDTO& argDTO,
                                     QTextStream& err, bool *error);
};

#endif // CONFIGDTO_H
