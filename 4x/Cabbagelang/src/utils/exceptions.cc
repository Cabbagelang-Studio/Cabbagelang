#include <utils/exceptions.h>

namespace utils {

InvalidExpressionException::InvalidExpressionException(const std::string &message) : std::runtime_error(message) {}

NotClassDefinitionException::NotClassDefinitionException(const std::string &message) : std::runtime_error(message) {}

NotImportException::NotImportException(const std::string &message) : std::runtime_error(message) {}

NotQuoteException::NotQuoteException(const std::string &message) : std::runtime_error(message) {}

NotCastException::NotCastException(const std::string &message) : std::runtime_error(message) {}

NotVariableException::NotVariableException(const std::string &message) : std::runtime_error(message) {}

NotFunctionException::NotFunctionException(const std::string &message) : std::runtime_error(message) {}

SyntaxErrorException::SyntaxErrorException(const std::string &message) : std::runtime_error(message) {}
}