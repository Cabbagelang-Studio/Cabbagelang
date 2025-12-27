#ifndef UTILS_EXCEPTIONS_H
#define UTILS_EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace utils {

class InvalidExpressionException : public std::runtime_error {
    public:
        explicit InvalidExpressionException(const std::string &message);
};

class NotClassDefinitionException : public std::runtime_error {
    public:
        explicit NotClassDefinitionException(const std::string &message);
};

class NotImportException : public std::runtime_error {
    public:
        explicit NotImportException(const std::string &message);
};

class NotQuoteException : public std::runtime_error {
    public:
        explicit NotQuoteException(const std::string &message);
};

class NotCastException : public std::runtime_error {
    public:
        explicit NotCastException(const std::string &message);
};

class NotVariableException : public std::runtime_error {
    public:
        explicit NotVariableException(const std::string &message);
};

class NotFunctionException : public std::runtime_error {
    public:
        explicit NotFunctionException(const std::string &message);
};

class SyntaxErrorException : public std::runtime_error {
    public:
        explicit SyntaxErrorException(const std::string &message);
};

} // namespace utils

#endif // UTILS_EXCEPTIONS_H