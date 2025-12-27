#include <utils/str_handle.h>

namespace utils {

std::stringstream strio;

std::string typeToString(llvm::Type* type) {
    if (!type) {
        return "nullptr"; // Handle null types
    }

    std::string typeStr;
    llvm::raw_string_ostream rso(typeStr); // Use raw_string_ostream to capture the type string
    type->print(rso); // Print the type to the string stream
    return rso.str(); // Return the captured string
}

std::string splitFirst(std::string str, std::string delim) {
    size_t pos = str.find(delim);
    if (pos != std::string::npos) {
        return str.substr(0, pos);
    }
    return str;
}

}