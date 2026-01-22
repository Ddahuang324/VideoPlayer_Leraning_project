#pragma once

#include "common/Public.h"
#include "common/Result.h"
#include "common/Error.h"
#include <unordered_map>

namespace yibo {

enum class HttpMethod {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_HEAD,
    HTTP_OPTIONS,
    HTTP_UNKNOWN
};

class CHttpParser {
public:
    CHttpParser() = default;

    Result<size_t, Error> Parser(BufferView data);

    HttpMethod Method() const { return m_method; }
    BufferView Url() const;
    BufferView Version() const;
    BufferView Header(BufferView key) const;
    BufferView Body() const;

    Buffer UrlCopy() const { return Buffer(Url()); }
    Buffer HeaderCopy(BufferView key) const { return Buffer(Header(key)); }

    bool IsComplete() const { return m_state == ParseState::Complete; }

private:
    enum class ParseState {
        RequestLine,
        Headers,
        Body,
        Complete,
        Error
    };

    BufferView m_data;
    ParseState m_state = ParseState::RequestLine;
    HttpMethod m_method = HttpMethod::HTTP_UNKNOWN;

    size_t m_url_start = 0;
    size_t m_url_end = 0;
    size_t m_version_start = 0;
    size_t m_version_end = 0;
    size_t m_body_start = 0;
    size_t m_body_end = 0;

    std::unordered_map<Buffer, BufferView> m_headers;

    Result<size_t, Error> ParseRequestLine(BufferView data);
    Result<size_t, Error> ParseHeaders(BufferView data);
    Result<size_t, Error> ParseBody(BufferView data);
};

} // namespace yibo
