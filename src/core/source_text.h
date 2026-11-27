#pragma once
#include <string_view>

namespace fpga::core {
// The Windows include stream normalizes CRLF while in-memory roots retain
// their original bytes. Compare exact text modulo that host line convention.
inline bool sameLineEndingText(std::string_view left,std::string_view right) {
    std::size_t l=0,r=0;
    const auto next=[](std::string_view text,std::size_t& offset) {
        if(offset<text.size() && text[offset]=='\r' &&
           (offset+1==text.size() || text[offset+1]=='\n')) ++offset;
        if(offset==text.size()) return -1;
        return static_cast<int>(static_cast<unsigned char>(text[offset++]));
    };
    while(true) {
        const auto a=next(left,l),b=next(right,r);
        if(a!=b) return false;
        if(a==-1) return true;
    }
}
} // namespace fpga::core
