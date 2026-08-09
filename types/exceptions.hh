#pragma once

#include <exception>
#include <string>


#define WHAT const char* what() const noexcept override

#define GEN_ERROR_CLASS(name) class name : public std::exception { \
    public: \
        std::string msg; \
        name(const std::string& msg) : msg(msg) {} \
        const char* what() const noexcept override { \
            return msg.c_str(); \
        } \
    } \



namespace baka {
namespace exceptions {

    GEN_ERROR_CLASS(DriverError);
    GEN_ERROR_CLASS(LexerError);
    GEN_ERROR_CLASS(ParserError);
    GEN_ERROR_CLASS(CodegenError);
    GEN_ERROR_CLASS(OptimizationPassError);

}
}
