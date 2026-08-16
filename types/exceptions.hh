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
#define YELLOW_ANSII "\033[38;2;253;218;3m"
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

#define LINE_DIAGNOSTIC_CLASS(name, ansii, error_type_str)     class name : public Diagnostic { \
    types::TokenSourceLocation sourceLocation; \
    base::LineCtx LineCtx_v; \
    std::string Message; \
    driver::Stage Stage; \
    public: \
    name(types::TokenSourceLocation sourceLocation, base::LineCtx LineCtx_v, std::string Message, driver::Stage Stage) : sourceLocation(sourceLocation), LineCtx_v(LineCtx_v), Message(std::move(Message)), Stage(Stage) {} \
    types::TokenSourceLocation getSourceLocation() const override { \
        return sourceLocation; \
    } \
    std::string getMessage() const override { \
        std::ostringstream os; \
        os << sourceLocation.FilePath << ":" << sourceLocation.LineNo << ":" << sourceLocation.Col << ": "  \
        << ansii << error_type_str << ": " << CLEAR_ANSII \
        << Message << " (" << to_string_view(Stage) << ")\n" \
        << '\t' << LineCtx_v \
        << '\t'; \
        \
        for(int i = 0; i < sourceLocation.Col-1; i++) os << ' '; \
        os << ansii << '^' << CLEAR_ANSII; \
        \
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


    LINE_DIAGNOSTIC_CLASS(CompilerError, RED_ANSII, "error");
    LINE_DIAGNOSTIC_CLASS(CompilerWarning, YELLOW_ANSII, "warning");

}
}
