#include "http/UrlParser.h"
#include <cctype>

namespace yibo {

BufferView UrlParser::operator[](BufferView key) {
    if (!m_parsed) {
        ParseQuery();
        m_parsed = true;
    }

    auto it = m_params.find(Buffer(key));
    if (it != m_params.end()) {
        return it->second;
    }
    return BufferView();
}

BufferView UrlParser::Path() const {
    size_t query_pos = m_url.find('?');
    if (query_pos == BufferView::npos) {
        return m_url;
    }
    return m_url.substr(0, query_pos);
}

BufferView UrlParser::Query() const {
    size_t query_pos = m_url.find('?');
    if (query_pos == BufferView::npos) {
        return BufferView();
    }
    return m_url.substr(query_pos + 1);
}

void UrlParser::ParseQuery() {
    BufferView query = Query();
    if (query.empty()) {
        return;
    }

    size_t pos = 0;
    while (pos < query.size()) {
        size_t amp_pos = query.find('&', pos);
        if (amp_pos == BufferView::npos) {
            amp_pos = query.size();
        }

        BufferView param = query.substr(pos, amp_pos - pos);
        size_t eq_pos = param.find('=');

        if (eq_pos != BufferView::npos) {
            BufferView key = param.substr(0, eq_pos);
            BufferView value = param.substr(eq_pos + 1);
            m_params[Buffer(key)] = value;
        }

        pos = amp_pos + 1;
    }
}

Buffer UrlParser::UrlDecode(BufferView encoded) {
    Buffer decoded;
    decoded.reserve(encoded.size());

    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            char high = encoded[i + 1];
            char low = encoded[i + 2];

            if (std::isxdigit(high) && std::isxdigit(low)) {
                int value = 0;
                if (high >= '0' && high <= '9') value = (high - '0') << 4;
                else if (high >= 'A' && high <= 'F') value = (high - 'A' + 10) << 4;
                else if (high >= 'a' && high <= 'f') value = (high - 'a' + 10) << 4;

                if (low >= '0' && low <= '9') value |= (low - '0');
                else if (low >= 'A' && low <= 'F') value |= (low - 'A' + 10);
                else if (low >= 'a' && low <= 'f') value |= (low - 'a' + 10);

                decoded += static_cast<char>(value);
                i += 2;
            } else {
                decoded += encoded[i];
            }
        } else if (encoded[i] == '+') {
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }

    return decoded;
}

} // namespace yibo
