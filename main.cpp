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

#include <QCoreApplication>

#include <QFile>
#include <QDir>
#include <QString>
#include <QTextStream>
#include <QMultiMap>
#include <QDebug>

#include <iostream>
#include <stdlib.h>
#include <math.h>

#include "configdto.h"

#define OPT_UNKNOWN -1
#define OPT_HELP    0
#define OPT_DEBUG   1
#define OPT_GROUPS  2
#define OPT_IGNMIS  3
#define OPT_COLOR   4
#define OPT_EXCLUDE 100
#define OPT_MERGE   101
#define OPT_INCLUDE 102
#define OPT_QUOTES  103
#define OPT_SRC     104
#define OPT_EXCLINC 105
#define OPT_VALUE   106
#define OPT_SAT     107

#define OPT_PARAM   100

#define GOLDEN_SECTION  137.50309

#define VERSION "v0.9"

void printHelp() {
    QTextStream err(stderr);

    err << "dep-analyser " << VERSION << "(C) dev@puschhof.de\n";
    err << "Released under the terms of the GNU General Public license.\n";
    err << "Graphs #include relationships between every C/C++ source and header file\n";
    err << "under the current directory using graphviz.\n\n";
    err << "Inspired by the perl script cinclude2dot by Darxus@ChaosReigns.com,\n";
    err << "francis@flourish.org\n";
    err << "Download from http://www.flourish.org/cinclude2dot/\n";
    err << "or original version http://www.chaosreigns.com/code/cinclude2dot/\n\n";
    err << "Command line options are:\n\n";
    err << "--debug             Display various debug info\n";
    err << "--exclude           Specify a regular expression of filenames to ignore\n";
    err << "                    for parsing. For example, ignore your test harnesses.\n";
    err << "--exclude-includes  Specify a regular expression for \"#include\"\n";
    err << "                    directives to ignore. For example dependencies to an\n";
    err << "                    optional library.\n";
    err << "--merge             Granularity of the diagram:\n";
    err << "                        file - the default, treats each file as separate\n";
    err << "                        module - merges .c/.cc/.cpp/.cxx and .h/.hpp/.hxx\n";
    err << "                                pairs\n";
    err << "                        directory - merges directories into one node\n";
    err << "--groups            Cluster files or modules into directory groups.\n";
    err << "                    Ignored for \"--merge directory\"\n";
    err << "--help              Display this help page.\n";
    err << "--include           Followed by a comma separated list of include search\n";
    err << "                    paths.\n";
    err << "--quotetypes        Select for parsing the files included by strip quotes\n";
    err << "                    or angle brackets:\n";
    err << "                        both - the default, parse all headers.\n";
    err << "                        angle - include only \"system\" headers included\n";
    err << "                                by anglebrackets (<>)\n";
    err << "                        quote - include only \"user\" headers included by\n";
    err << "                                strip quotes (\"\")\n";
    err << "--src               Followed by a path to the source code, defaults to\n";
    err << "                    current directory.\n";
    err << "--ignoremissing     Assumes missing header files are generated files and\n";
    err << "                    adds them to the dependency list without raising an\n";
    err << "                    error.The dependency filename is taken directly from\n";
    err << "                    the \"#include\" directive without prepending any path\n";
    err << "--colorize          Use colors for the lines. HSV colrs with a predifined\n";
    err << "                    value and saturation and a calculated hue are used.\n";
    err << "--value             Only with \"--colorize\". Sets the value for the line\n";
    err << "                    colors. Values between 0 and 255 are accepted.\n";
    err << "                    Default: 240.\n";
    err << "--sat               Only with \"--colorize\". Sets the saturation for the\n";
    err << "                    line colors. Values between 0 and 255 are accepted.\n";
    err << "                    Default: 192.\n\n";
    err << "Usage:\n";
    err << "    dep-analyser > deps.dot\n";
    err << "    dot -Tpng deps.dot -o deps.png\n";
    err.flush();
    exit(0);
}

void printConfig(const ConfigDTO& config, QTextStream& err) {
    err << "Configuration:\n";
    err << "Source code directory: " << config.srcPath << "\n";
    err << "Merge mode: ";
    switch(config.mergeMode) {
        case MERGE_FILE: err << "file\n"; break;
        case MERGE_MODULE: err << "module\n"; break;
        case MERGE_DIR: err << "directory\n"; break;
    }
    err << "Quote types: ";
    switch(config.quoteType) {
        case QUOTE_BOTH: err << "both\n"; break;
        case QUOTE_ANGLE: err << "angle\n"; break;
        case QUOTE_QUOTE: err << "quote\n"; break;
    }
    err << "Create groups: " << (config.groups ? "yes" : "no") << "\n";
    err << "Ignore missing includes: " << (config.ignoreMissing ? "yes" : "no") << "\n";
    err << "Colorize graph: " << (config.colorize ? "yes" : "no") << "\n";
    if(config.colorize) {
        err << "Value: " << config.value << "\n";
        err << "Saturation: " << config.saturation << "\n";
    }
    if(!config.excludeRegEx.isEmpty()) {
        err << "Exclude: " << config.excludeRegEx;
    }
    if(!config.excludeIncludeRegEx.isEmpty()) {
        err << "Exclude includes: " << config.excludeIncludeRegEx;
    }
    if(!config.includePaths.isEmpty()) {
        err << "Include directories: " << config.includePaths.join("\n\t") << "\n";
    }
    err.flush();
}

void parseDir(const ConfigDTO& config, QMultiMap<QString, QString>& mapping,
              const QList<QDir>& includeDirs, const QString& path, QTextStream& err) {
    QDir current(path);

    if(config.debug) {
        err << "parse directory " << path << "\n";
        err.flush();
    }

    //Get subdirectories
    QStringList subDirs = current.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach(const QString& subDir, subDirs) {
        parseDir(config, mapping, includeDirs, current.absoluteFilePath(subDir), err);
    }

    //GetFiles
    QStringList filter;
    filter << "*.c" << "*.cc" << "*.cpp" << "*.cxx" << "*.h" << "*.hpp" << "*.hxx";
    QStringList files = current.entryList(filter, QDir::Files | QDir::NoDotAndDotDot
                                          | QDir::Readable);

    foreach(const QString& file, files) {
        QString absolutePath = current.absoluteFilePath(file);
        QRegExp exclude(config.excludeRegEx);
        QRegExp excludeIncl(config.excludeIncludeRegEx);
        if(config.excludeRegEx.isEmpty() || exclude.indexIn(absolutePath) == -1) {
            if(config.debug) {
                err << "Analyse file " << absolutePath << "\n";
                err.flush();
            }
            QFile currentFile(absolutePath);
            bool open = currentFile.open(QIODevice::ReadOnly | QIODevice::Text);
            if(!open) {
                err << "Could not read " << absolutePath << "\n";
                err.flush();
            } else {
                while(!currentFile.atEnd()) {
                    QString line = currentFile.readLine();
                    if(line.startsWith("#include ")) {
                        line = line.right(line.length() - 9);
                        if(!(line.startsWith("<") && config.quoteType == QUOTE_QUOTE)
                                && !(line.startsWith("\"")
                                     && config.quoteType == QUOTE_ANGLE)) {
                            QRegExp sep("[>\"]");
                            bool exists = false;
                            line = line.right(line.length() - 1);
                            line = line.section(sep, 0, 0);
                            QString includePath = line;

                            if(config.excludeIncludeRegEx.isEmpty()
                                    || excludeIncl.indexIn(includePath) == -1) {
                                //Check if file exists
                                if(QFile::exists(current.absoluteFilePath(line))) {
                                    exists = true;
                                    includePath = current.absoluteFilePath(line);
                                } else {
                                    for(int i = 0; i < includeDirs.count() && !exists;
                                        i++) {
                                        if(QFile::exists(includeDirs.at(i).
                                                         absoluteFilePath(line))) {
                                            exists = true;
                                            includePath = includeDirs.at(i).
                                                    absoluteFilePath(line);
                                        }
                                    }
                                }

                                if(config.ignoreMissing) {
                                    exists = true;
                                }

                                if(exists) {
                                    mapping.insert(absolutePath, includePath);
                                } else {
                                    err << "Could not find include " << includePath
                                        << " from " << absolutePath << "\n";
                                    err.flush();
                                }
                            } else if(config.debug) {
                                err << "Ignoring include " << includePath << "\n";
                                err.flush();
                            }
                        }
                    }
                }
            }
        } else if(config.debug) {
            err << "Excluding file " << absolutePath << "\n";
            err.flush();
        }
    }
}

QMultiMap<QString, QString> parseSource(const ConfigDTO& config, QTextStream& err) {
    QMultiMap<QString, QString> result;
    QList<QDir> includeDirs;

    //Create include dirs
    includeDirs << QDir(config.srcPath);
    foreach(const QString& inclDir, config.includePaths) {
        includeDirs << QDir(inclDir);
    }
    //Check include dirs
    foreach(const QDir& dir, includeDirs) {
        if(!dir.exists()) {
            err << dir.absolutePath() << " does not exists\n";
        } else if(config.debug) {
            err << dir.absolutePath() << " exists\n";
        }
        err.flush();
    }

    parseDir(config, result, includeDirs, config.srcPath, err);

    return result;
}

QMultiMap<QString, QString> mergeModules(QMultiMap<QString, QString> mapping) {
    QMultiMap<QString, QString> result;

    QMapIterator<QString, QString> iterator(mapping);
    while(iterator.hasNext()) {
        iterator.next();
        QString key = iterator.key().section('.', 0, -2);
        QString value = iterator.value().section('/', 0, -2,
                                                 QString::SectionIncludeTrailingSep);
        QString file = iterator.value().section('/', -1, -1);
        if(file.contains('.')) {
            file = file.section('.', 0, -2);
        }
        value += file;
        if(key.compare(value) != 0 && !result.values(key).contains(value)) {
            result.insert(key, value);
        }
    }

    return result;
}

QMultiMap<QString, QString> mergeDirectories(QMultiMap<QString, QString> mapping) {
    QMultiMap<QString, QString> result;

    QMapIterator<QString, QString> iterator(mapping);
    while(iterator.hasNext()) {
        iterator.next();
        QString key = iterator.key().section('/', 0, -2);
        QString value = iterator.value().section('/', 0, -2);
        if(key.compare(value) != 0 && !result.values(key).contains(value)) {
            result.insert(key, value);
        }
    }

    return result;
}

QString removeBaseDir(QString path, const QString& baseDir) {
    if(path.startsWith(baseDir)) {
        path = path.remove(baseDir.section('/', 0, -2));
        if(path.startsWith("/")) {
            path = path.right(path.length() - 1);
        }
    }

    return path;
}

QString nextColor(int saturation, int value) {
    static double hue = 0.0;
    int r, g, b;

    if (saturation == 0) {
        r = value;
        g = value;
        b = value;
    } else {
        unsigned int region, remainder, p, q, t;
        int iHue = 0;

        hue += GOLDEN_SECTION;
        iHue = (int) floor(fmod(hue, 360.0));

        region = iHue / 60;
        remainder = (iHue - (region * 60)) * 6;

        p = (value * (255 - saturation)) / 256;
        q = (value * (255 - ((saturation * remainder) / 256))) / 256;
        t = (value * (255 - ((saturation * (255 - remainder)) / 256))) / 256;

        switch (region) {
            case 0:
                r = value; g = t; b = p;
                break;
            case 1:
                r = q; g = value; b = p;
                break;
            case 2:
                r = p; g = value; b = t;
                break;
            case 3:
                r = p; g = q; b = value;
                break;
            case 4:
                r = t; g = p; b = value;
                break;
            default:
                r = value; g = p; b = q;
                break;
        }
    }

    return QString("#%1%2%3").arg(QString::number(r, 16)).
            arg(QString::number(g, 16)).arg(QString::number(b, 16));
}

void printMapping(const QMultiMap<QString, QString>& mapping, QTextStream& out,
                  bool group, bool colorize, int saturation, int value,
                  const QString& baseDir) {
    QStringList keys = mapping.keys();
    keys.removeDuplicates();

    //Write header
    out << "digraph \"source tree\" {\n";
    out << "    overlap=scale;\n";
    out << "    ratio=\"auto\";\n";
    out << "    fontsize=\"16\";\n";
    out << "    fontname=\"Helvetica\";\n";
    out << "    clusterrank=\"local\";\n";

    if(group) {
        QStringList allNodes = keys;
        allNodes << mapping.values();
        allNodes.removeDuplicates();
        foreach(const QString& key, allNodes) {
            QString dir = key;
            QString file = key;
            QString escDir;
            dir = removeBaseDir(dir.section('/', 0, -2), baseDir);
            file = file.section('/', -1, -1);
            escDir = dir;
            escDir = escDir.replace('/', "_");

            out << "subgraph \"cluster_" << escDir << "\" {\n";
            out << "    label=\"" << dir << "\"\n";
            out << "    \"" << file << "\"\n";
            out << "}\n";
        }
    }

    foreach(const QString& key, keys) {
        QStringList values = mapping.values(key);
        QString name = removeBaseDir(key, baseDir);
        if(group) {
            name = name.section('/', -1);
        }
        out << "    \"" << name << "\" -> { ";

        foreach(const QString& value, values) {
            QString include = removeBaseDir(value, baseDir);
            if(group) {
                include = include.section('/', -1);
            }
            out << "\"" << include << "\" ";
        }

        out << "}";

        if(colorize) {
            out << " [color=\"" << nextColor(saturation, value) << "\"]";
        }

        out << "\n";
    }

    out << "}\n";
    out.flush();
}

int main(int argc, char *argv[]) {
    QTextStream out(stdout);
    QTextStream err(stderr);
    ConfigDTO config;
    QMultiMap<QString, QString> mapping;

    //Parse command line options
    for(int i = 1; i < argc;) {
        QString opt = QString::fromUtf8(argv[i]);
        QString optValue;
        int optCode = OPT_UNKNOWN;
        bool converted = true;

        if(opt.compare("--debug") == 0) {
            optCode = OPT_DEBUG;
        } else if(opt.compare("--exclude") == 0) {
            optCode = OPT_EXCLUDE;
        } else if(opt.compare("--merge") == 0) {
            optCode = OPT_MERGE;
        } else if(opt.compare("--groups") == 0) {
            optCode = OPT_GROUPS;
        } else if(opt.compare("--help") == 0) {
            optCode = OPT_HELP;
            printHelp();
        } else if(opt.compare("--include") == 0) {
            optCode = OPT_INCLUDE;
        } else if(opt.compare("--quotetypes") == 0) {
            optCode = OPT_QUOTES;
        } else if(opt.compare("--src") == 0) {
            optCode = OPT_SRC;
        } else if(opt.compare("--ignoremissing") == 0) {
            optCode = OPT_IGNMIS;
        } else if(opt.compare("--colorize") == 0) {
            optCode = OPT_COLOR;
        } else if(opt.compare("--exclude-includes") == 0) {
            optCode = OPT_EXCLINC;
        } else if(opt.compare("--value") == 0) {
            optCode = OPT_VALUE;
        } else if(opt.compare("--sat") == 0) {
            optCode = OPT_SAT;
        } else {
            err << "Unknown argument " << opt << "\n";
            err.flush();
            printHelp();
        }

        if(optCode >= OPT_PARAM) {
            if(argc > (i + 1)) {
                optValue = QString::fromUtf8(argv[i + 1]);
                i += 2;
            } else {
                err << "Argument " << opt << " needs a value\n";
                err.flush();
                printHelp();
            }
        } else {
            i++;
        }

        switch(optCode) {
            case OPT_DEBUG: config.debug = true; break;
            case OPT_GROUPS: config.groups = true; break;
            case OPT_IGNMIS: config.ignoreMissing = true; break;
            case OPT_COLOR: config.colorize = true; break;
            case OPT_EXCLUDE: config.excludeRegEx = optValue; break;
            case OPT_EXCLINC: config.excludeIncludeRegEx = optValue; break;
            case OPT_SRC: {
                    QDir srcDir(optValue);
                    config.srcPath = srcDir.absolutePath();
                }
                break;
            case OPT_INCLUDE:
                config.includePaths << optValue.split(",", QString::SkipEmptyParts);
                break;
            case OPT_MERGE:
                if(optValue.compare("file") == 0) {
                    config.mergeMode = MERGE_FILE;
                } else if(optValue.compare("module") == 0) {
                    config.mergeMode = MERGE_MODULE;
                } else if(optValue.compare("directory") == 0) {
                    config.mergeMode = MERGE_DIR;
                } else {
                    err << "Unknown merge mode " << optValue << "\n";
                    err.flush();
                    printHelp();
                }
                break;
            case OPT_QUOTES:
                if(optValue.compare("both") == 0) {
                    config.quoteType = QUOTE_BOTH;
                } else if(optValue.compare("angle") == 0) {
                    config.quoteType = QUOTE_ANGLE;
                } else if(optValue.compare("quote") == 0) {
                    config.quoteType = QUOTE_QUOTE;
                } else {
                    err << "Unknown quote type " << optValue << "\n";
                    err.flush();
                    printHelp();
                }
                break;
            case OPT_VALUE:
                config.value = optValue.toInt(&converted);
                if(!converted || config.value < 0 || config.value > 255) {
                    err << "Illegal value for value: " << optValue << "\n";
                    err.flush();
                    printHelp();
                }
                break;
            case OPT_SAT:
                config.saturation = optValue.toInt(&converted);
                if(!converted || config.saturation < 0 || config.saturation > 255) {
                    err << "Illegal value for saturation: " << optValue << "\n";
                    err.flush();
                    printHelp();
                }
                break;
            default:
                err << "Internal error... This should not have happended\n";
                err.flush();
                printHelp();
        }
    }

    if(config.debug) {
        printConfig(config, err);
    }

    mapping = parseSource(config, err);

    if(config.mergeMode == MERGE_MODULE) {
        mapping = mergeModules(mapping);
    } else if(config.mergeMode == MERGE_DIR) {
        mapping = mergeDirectories(mapping);
    }

    if(config.debug) {
        //print the mapping
        QStringList keys = mapping.keys();
        keys.removeDuplicates();
        foreach(const QString& key, keys) {
            err << key << ":\n";
            QStringList values = mapping.values(key);
            foreach(const QString& value, values) {
                err << "\t" << value << "\n";
            }
        }
        err.flush();
    }

    printMapping(mapping, out, (config.groups && config.mergeMode != MERGE_DIR),
                 config.colorize, config.saturation, config.value, config.srcPath);
}
