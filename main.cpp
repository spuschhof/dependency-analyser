#include <QCoreApplication>

#include <QFile>
#include <QDir>
#include <QString>
#include <QTextStream>
#include <QMultiMap>
#include <QDebug>

#include <iostream>
#include <stdlib.h>

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

#define OPT_PARAM   100

void printHelp() {
    QTextStream out(stdout);

    out << "Graphs #include relationships between every C/C++ source and header file\n"
           "under the current directory using graphviz.\n\n"
           "Command line options are:\n\n"
           "--debug         Display various debug info\n"
           "--exclude       Specify a regular expression of filenames to ignore\n"
           "                For example, ignore your test harnesses.\n"
           "--merge         Granularity of the diagram:\n"
           "                    file - the default, treats each file as separate\n"
           "                    module - merges .c/.cc/.cpp/.cxx and .h/.hpp/.hxx pairs\n"
           "                    directory - merges directories into one node\n"
           "--groups        Cluster files or modules into directory groups.\n"
           "                Ignored for \"--merge directory\"\n"
           "--help          Display this help page.\n"
           "--include       Followed by a comma separated list of include search\n"
           "                paths.\n"
           "--quotetypes    Select for parsing the files included by strip quotes or \n"
           "                angle brackets:\n"
           "                    both - the default, parse all headers.\n"
           "                    angle - include only \"system\" headers included by\n"
           "                        anglebrackets (<>)\n"
           "                    quote - include only \"user\" headers included by\n"
           "                        strip quotes (\"\")\n"
           "--src           Followed by a path to the source code, defaults to current\n"
           "                directory\n"
           "--ignoremissing Assumes missing header files are generated files and\n"
           "                adds them to the dependency list without raising an error.\n"
           "                The dependency filename is taken directly from the \n"
           "                \"#include\" directive without prepending any path\n"
           "--colorize      Use random colrs for the lines.\n";
    out.flush();
    exit(0);
}

void printConfig(const ConfigDTO& config, QTextStream& err) {
    err << "Configuration:\n"
           "Source code directory: " << config.srcPath << "\n";
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
    if(!config.excludeRegEx.isEmpty()) {
        err << "Exclude: " << config.excludeRegEx;
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

        qDebug() << absolutePath;

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

QString randomColor() {
    int red = qrand() % 256;
    int green = qrand() % 256;
    int blue = qrand() % 256;

    red = (red + 255) / 2;
    green = (green + 255) / 2;
    blue = (blue + 255) / 2;

    return QString("#%1%2%3").arg(QString::number(red, 16)).
            arg(QString::number(green, 16)).arg(QString::number(blue, 16));
}

void printMapping(const QMultiMap<QString, QString>& mapping, QTextStream& out,
                  bool group, bool colorize, const QString& baseDir) {
    QStringList keys = mapping.keys();
    keys.removeDuplicates();

    //Write header
    out << "digraph \"source tree\" {\n"
           "    overlap=scale;\n"
           "    ratio=\"auto\";\n"
           "    fontsize=\"16\";\n"
           "    fontname=\"Helvetica\";\n"
           "    clusterrank=\"local\";\n";

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

            out << "subgraph \"cluster_" << escDir << "\" {\n"
                   "    label=\"" << dir << "\"\n"
                   "    \"" << file << "\"\n"
                   "}\n";
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
            out << " [color=\"" << randomColor() << "\"]";
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
        } else {
            out << "Unknown argument " << opt << "\n";
            out.flush();
            printHelp();
        }

        if(optCode >= OPT_PARAM) {
            if(argc > (i + 1)) {
                optValue = QString::fromUtf8(argv[i + 1]);
                i += 2;
            } else {
                out << "Argument " << opt << " needs a value\n";
                out.flush();
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
                    out << "Unknown merge mode " << optValue << "\n";
                    out.flush();
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
                    out << "Unknown quote type " << optValue << "\n";
                    out.flush();
                    printHelp();
                }
                break;
            default:
                out << "Internal error... This should not have happended\n";
                out.flush();
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
                 config.colorize, config.srcPath);
}
