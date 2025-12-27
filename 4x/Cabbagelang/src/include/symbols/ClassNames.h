#ifndef SYMBOLS_CLASSNAMES_H
#define SYMBOLS_CLASSNAMES_H

#include <log/log.h>

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace symbols{

struct classInfo {
    std::string package;
    int access;
};

class ClassNames {
    public:
        ClassNames();
        void insert(const std::string& className, const std::string& package, const char* filename, int line, int column, int access);
        void quote(const std::string& className);
        bool lookup(const std::string& className);

        void check_quoted(const char* filename);
        std::unordered_set<std::string> quotedClassNames;
        std::unordered_map<std::string, classInfo> classNames;
};

} // namespace symbols

#endif // SYMBOLS_CLASSNAMES_H
