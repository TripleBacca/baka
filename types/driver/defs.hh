#pragma once
#include <cassert>
#include <string_view>


namespace baka {
namespace driver {

    enum class Stage {
        LEX = 0,
        PARSE = 1,
        CODEGEN = 2,
    };

    constexpr std::string_view to_string_view(Stage stage) {
        switch (stage) {
            case Stage::LEX:
                return "LEX";
            case Stage::PARSE:
                return "PARSE";
            case Stage::CODEGEN:
                return "CODEGEN";
        }

        assert(false && "add more cases");
    }


    enum class BuildType {
        DEBUG,
        RELEASE,
    };


    constexpr std::string_view to_string_view(BuildType stage) {
        switch (stage) {
            case BuildType::DEBUG:
                return "DEBUG";
            case BuildType::RELEASE:
                return "RELEASE";
        }

        assert(false && "add more cases");
    }

}
}
