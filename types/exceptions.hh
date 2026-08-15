#pragma once

#include "driver/defs.hh"
#include "line_index.hh"
#include "types/token/token.hh"
#include <cstdio>
#include <exception>
#include <sstream>
#include <string>
#include <string_view>

#define RED_ANSII "\033[38;2;255;0;0m"
#define YELLOW_ANSII "\033[38;2;255;255;0m"
#define CLEAR_ANSII "\033[0m"

#define WHAT const char* what() const noexcept override

#define GEN_ERROR_CLASS(name) class name : public std::exception { \
    public: \
        std::string msg; \
        name(std::string msg) : msg(std::move(msg)) {} \
        const char* what() const noexcept override { \
            return msg.c_str(); \
        } \
    } \

#define LINE_DIAGNOSTIC_CLASS(name, ansii)     class name : public Diagnostic { \
    types::TokenSourceLocation sourceLocation; \
    base::LineCtx LineCtx_v; \
    std::string Message; \
    public: \
    name(types::TokenSourceLocation sourceLocation, base::LineCtx LineCtx_v, std::string Message) : sourceLocation(sourceLocation), LineCtx_v(LineCtx_v), Message(Message) {} \
    types::TokenSourceLocation getSourceLocation() const override { \
        return sourceLocation; \
    } \
    std::string getMessage() const override { \
        std::ostringstream os; \
        os << sourceLocation.FilePath << sourceLocation.LineNo << ":" << sourceLocation.Col << ": " << LineCtx_v << '\n' \
            << '\t' << ansii << Message << CLEAR_ANSII << '\n'; \
        return os.str(); \
    } \
} \

// TODO: abandon c++ exceptions cuz cringe
namespace baka {
namespace exceptions {

    GEN_ERROR_CLASS(DriverError);


    class Diagnostic {
        public:
        virtual types::TokenSourceLocation getSourceLocation() const = 0;
        virtual std::string getMessage() const = 0;
    };


    LINE_DIAGNOSTIC_CLASS(CompilerError, RED_ANSII);
    LINE_DIAGNOSTIC_CLASS(CompilerWarning, YELLOW_ANSII);

}
}
