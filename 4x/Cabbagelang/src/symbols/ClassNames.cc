#include <symbols/ClassNames.h>

namespace symbols{

ClassNames::ClassNames(){
    insert("String", "builtin", "<builtin>", 0, 0, 0);
    insert("Exception", "builtin", "<builtin>", 0, 0, 0);
    insert("Generic", "builtin", "<builtin>", 0, 0, 0);
}

void ClassNames::insert(const std::string& className, const std::string& package, const char* filename, int line, int column, int access) {
    if (classNames.find(className) != classNames.end()) {
        compilation_error(filename, line, column, "Class %s already defined", className.data());
    } else {
        classInfo info = {package, access};
        classNames.insert({className, info});
    }
}

void ClassNames::quote(const std::string& className) {
    if(classNames.find(className) == classNames.end())
        quotedClassNames.insert(className);
}

bool ClassNames::lookup(const std::string& className) {
    if(classNames.find(className) != classNames.end()) return true;
    return quotedClassNames.find(className) != quotedClassNames.end();
}

void ClassNames::check_quoted(const char* filename){
    for(std::string elem : quotedClassNames)
        if(classNames.find(elem) == classNames.end())
            compilation_error(filename, 0, 0, "Quoted class %s not declared", elem.data());
}

} // namespace symbols