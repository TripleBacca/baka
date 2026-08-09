#pragma once

#include "types/token/token.hh"
#include <cstdio>
#include <exception>
#include <sstream>
#include <string>


#define WHAT const char* what() const noexcept override

#define GEN_ERROR_CLASS(name) class name : public std::exception { \
    public: \
        std::string msg; \
        name(std::string msg) : msg(std::move(msg)) {} \
        const char* what() const noexcept override { \
            return msg.c_str(); \
        } \
    } \

#define LINE_INFO_ERROR_CLASS(name) class name : std::exception { \
    std::string msg; \
    types::TokenSourceLocation SourceLocation; \
    mutable std::string scratch; \
    \
    public: \
        \
    name(std::string msg, types::TokenSourceLocation SourceLocation) : msg(std::move(msg)), SourceLocation(SourceLocation) {} \
    \
        const char* what() const noexcept override { \
            std::ostringstream otpt; \
            otpt.clear(); \
            otpt << SourceLocation.FilePath << ": " << SourceLocation.LineNo << ":" << SourceLocation.Col << ": " << msg; \
            scratch = otpt.str(); \
            \
            return scratch.c_str(); \
        } \
    }; \



namespace baka {
namespace exceptions {

    GEN_ERROR_CLASS(DriverError);
    LINE_INFO_ERROR_CLASS(LexerError);
    LINE_INFO_ERROR_CLASS(ParserError);
    LINE_INFO_ERROR_CLASS(CodegenError);
    LINE_INFO_ERROR_CLASS(OptimizationPassError);

}
}
