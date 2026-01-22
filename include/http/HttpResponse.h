#pragma once

#include "common/Public.h"
#include <unordered_map>

namespace yibo {

class HttpResponse {
public:
    HttpResponse() = default;

    HttpResponse& SetStatus(int code, BufferView message);
    HttpResponse& AddHeader(BufferView key, BufferView value);
    HttpResponse& SetBody(BufferView body);

    Buffer Build() const;

private:
    int m_status_code = 200;
    Buffer m_status_message = "OK";
    std::unordered_map<Buffer, Buffer> m_headers;
    Buffer m_body;
};

} // namespace yibo
