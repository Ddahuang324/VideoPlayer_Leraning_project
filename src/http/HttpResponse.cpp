#include "http/HttpResponse.h"

namespace yibo {

HttpResponse& HttpResponse::SetStatus(int code, BufferView message) {
    m_status_code = code;
    m_status_message = Buffer(message);
    return *this;
}

HttpResponse& HttpResponse::AddHeader(BufferView key, BufferView value) {
    m_headers[Buffer(key)] = Buffer(value);
    return *this;
}

HttpResponse& HttpResponse::SetBody(BufferView body) {
    m_body = Buffer(body);
    AddHeader("Content-Length", std::to_string(body.size()));
    return *this;
}

Buffer HttpResponse::Build() const {
    size_t estimated_size = 512 + m_body.size();
    Buffer response;
    response.reserve(estimated_size);

    response += "HTTP/1.1 ";
    response += std::to_string(m_status_code);
    response += " ";
    response += m_status_message;
    response += "\r\n";

    for (const auto& [key, value] : m_headers) {
        response += key;
        response += ": ";
        response += value;
        response += "\r\n";
    }

    response += "\r\n";
    response += m_body;

    return response;
}

} // namespace yibo
