#pragma once

#include "common/Public.h"
#include <unordered_map>

namespace yibo {

class UrlParser {
public:
    explicit UrlParser(BufferView url) : m_url(url) {}

    BufferView operator[](BufferView key);
    BufferView Path() const;
    BufferView Query() const;

private:
    BufferView m_url;
    std::unordered_map<Buffer, BufferView> m_params;
    bool m_parsed = false;

    void ParseQuery();
    Buffer UrlDecode(BufferView encoded);
};

} // namespace yibo
