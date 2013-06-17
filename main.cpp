#include <QCoreApplication>

#include <QFile>
#include <QDir>
#include <QString>
#include <QTextStream>

#include <iostream>

#include "configdto.h"

#define OPT_UNKNOWN -1
#define OPT_HELP    0
#define OPT_DEBUG   1
#define OPT_GROUPS  2
#define OPT_PATHS   3
#define OPT_IGNMIS  4
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
           "--groups        Cluster files or modules into directory groups\n"
           "--help          Display this help page.\n"
           "--include       Followed by a comma separated list of include search\n"
           "                paths.\n"
           "--paths         Leaves relative paths in displayed filenames.\n"
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
           "                \"#include\" directive without prepending any path\n";
    out.flush();
    exit(0);
}

void printConfig(const ConfigDTO& config) {
    QTextStream out(stderr);

    out << "Configuration:\n"
           "Source code directory: " << config.srcPath << "\n";
    out << "Merge mode: ";
    switch(config.mergeMode) {
        case MERGE_FILE: out << "file\n"; break;
        case MERGE_MODULE: out << "module\n"; break;
        case MERGE_DIR: out << "directory\n"; break;
    }
    out << "Quote types: ";
    switch(config.quoteType) {
        case QUOTE_BOTH: out << "both\n"; break;
        case QUOTE_ANGLE: out << "angle\n"; break;
        case QUOTE_QUOTE: out << "quote\n"; break;
    }
    out << "Create groups: " << (config.groups ? "yes" : "no") << "\n";
    out << "Display relative paths: " << (config.paths ? "yes" : "no") << "\n";
    out << "Ignore missing includes: " << (config.ignoreMissing ? "yes" : "no") << "\n";
    if(!config.excludeRegEx.isEmpty()) {
        out << "Exclude: " << config.excludeRegEx;
    }
    if(!config.includePaths.isEmpty()) {
        out << "Include directories: " << config.includePaths.join("\n\t") << "\n";
    }
}

 parseSource(const ConfigDTO& config) {

}

int main(int argc, char *argv[]) {
    QTextStream out(stdout);
    ConfigDTO config;

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
        } else if(opt.compare("--paths") == 0) {
            optCode = OPT_PATHS;
        } else if(opt.compare("--quotetypes") == 0) {
            optCode = OPT_QUOTES;
        } else if(opt.compare("--src") == 0) {
            optCode = OPT_SRC;
        } else if(opt.compare("--ignoremissing") == 0) {
            optCode = OPT_IGNMIS;
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
            case OPT_PATHS: config.paths = true; break;
            case OPT_IGNMIS: config.ignoreMissing = true; break;
            case OPT_EXCLUDE: config.excludeRegEx = optValue; break;
            case OPT_SRC: config.srcPath = optValue; break;
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
        printConfig(config);
    }
}
