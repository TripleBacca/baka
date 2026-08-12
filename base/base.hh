#pragma once
#include <filesystem>


namespace baka {
namespace base {

    bool ValidateFile(std::filesystem::path path);
    size_t FileSize(std::filesystem::path path);

    // ascii.cc
    bool isValidStringChar(char c);
    bool isSimpleEscapeSequenceChar(char c);
    char SimpleEscapeSequence(char suffix);
    bool isValidKeywordNonDigit(char c);
    bool isValidOperatorChar(char c);

}
}
