#pragma once
#include "line_index.hh"
#include <cassert>
#include <cstddef>
#include <deque>
#include <stdexcept>
#include <string_view>


namespace baka {
namespace base {

    LineIndex::LineIndex(std::string_view SourceCode) : SourceCode(SourceCode), lineRunning(false), lineIdx(0) {
        LineIndexId = LineIndexIdCt++;
    }

    void LineIndex::startLine(size_t startIdx) {
        lineRunning = true;
        lineStartIdx = startIdx;
    }

    void LineIndex::endLine(size_t endIdx) {
        if(!lineRunning) {
            throw std::runtime_error("endLine called without startLine");
        }

        assert(lineStartIdx <= endIdx);

        LineInfos.emplace_back(lineStartIdx, endIdx);
        lineRunning = false;
        lineIdx++;
    }

    size_t LineIndex::CurrentLineIdx() const {
        return lineIdx;
    }

    LineCtx LineIndex::CurrentLine() {
        return LineCtx{lineIdx, this};
    }


    std::string_view LineIndex::GetLine(size_t lineIdx) const {
        const LineInfo& li = LineInfos.at(lineIdx);
        return SourceCode.substr(li.StartIdx, li.EndIdx - li.StartIdx + 1);
    }


    std::string_view LineIndex::GetSourceCode() const {
        return SourceCode;
    }

    std::string_view LineIndex::GetLine(const LineCtx& lCtx) const {
        assert(lCtx.LineIndexPtr == this);

        const LineInfo& li = LineInfos.at(lCtx.LineIdx);
        return SourceCode.substr(li.StartIdx, li.EndIdx - li.StartIdx);
    }


}
}
