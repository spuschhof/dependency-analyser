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
#include "configdto.h"

#include <QDir>
#include <QFile>

ConfigDTO::ConfigDTO() {
    this->debug = false;
    this->groups = false;
    this->ignoreMissing = false;
    this->colorize = false;
    this->keepPaths = false;
    this->colorNodes = false;
    this->mergeMode = MERGE_FILE;
    this->quoteType = QUOTE_BOTH;
    this->value = 128;
    this->saturation = 128;

#ifdef Q_OS_LINUX
    this->includePaths << "/usr/include";
    this->includePaths << "/usr/local/include";
#endif

    this->srcPath = QDir::currentPath();

    this->cmdProvided = 0;
}

QString ConfigDTO::getNodeColor(int dependencies) const {
    QList<int> thresholds = this->nodeColorMap.keys();
    int index = thresholds.count() - 1;

    while(index > 0 && thresholds.at(index) > dependencies) {
        index--;
    }

    if((index == 0 && dependencies < thresholds.at(index)) || index < 0) {
        return "white";
    }

    return this->nodeColorMap.value(thresholds.at(index));
}

ConfigDTO ConfigDTO::parseConfigFile(const QString &file, const ConfigDTO &argDTO,
                                     QTextStream &err, bool* error) {
    if(error) {
        *error = false;
    }
    ConfigDTO parsedDTO;

    if(QFile::exists(file)) {
        //TODO: Implement configration file parsing
    } else {
        if(error) {
            *error = true;
        }
        err << "Configuration file " << file << " does not exists.\n";
        err.flush();
    }

    return parsedDTO;
}
