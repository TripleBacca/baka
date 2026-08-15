#pragma once
#include <atomic>
#include <cassert>
#include <cstddef>
#include <deque>
#include <memory>
#include <string_view>

namespace {
    struct LineInfo {
        size_t StartIdx;
        size_t EndIdx; // inclusive
    };
}

namespace baka {
namespace base {
    inline static std::atomic<int> LineIndexIdCt = 0;

    struct LineCtx;

    class LineIndex {
        int LineIndexId;

        std::string_view SourceCode;
        std::deque<LineInfo> LineInfos; // need cuz dont wanna invalidate shit

        bool lineRunning;
        size_t lineStartIdx;
        size_t lineIdx;

        public:

        LineIndex(std::string_view SourceCode);

        void startLine(size_t startIdx);
        void endLine(size_t endIdx);

        size_t CurrentLineIdx() const;
        LineCtx CurrentLine();

        std::string_view GetLine(size_t lineIdx) const;
        std::string_view GetSourceCode() const;

        private:
            std::string_view GetLine(const LineCtx& lCtx) const;
        friend LineCtx;
    };


    struct LineCtx {
        size_t LineIdx;
        LineIndex* LineIndexPtr; // make sure it came from same line index

        std::string_view Display() {
            return LineIndexPtr->GetLine(*this);
        }
    };
}
}
