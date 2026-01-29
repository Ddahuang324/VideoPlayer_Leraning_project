#include "http/CHttpParser.h"
#include <algorithm>
#include <cctype>

namespace yibo {

Result<size_t, Error> CHttpParser::Parser(BufferView data) {
    m_data = data;
    size_t total_parsed = 0;

    while (m_state != ParseState::Complete && m_state != ParseState::Error) {
        BufferView remaining = data.substr(total_parsed);

        switch (m_state) {
            case ParseState::RequestLine: {
                auto result = ParseRequestLine(remaining);
                if (result.IsErr()) return result;
                total_parsed += result.Value();
                m_state = ParseState::Headers;
                break;
            }
            case ParseState::Headers: {
                auto result = ParseHeaders(remaining);
                if (result.IsErr()) return result;
                total_parsed += result.Value();

                auto content_length = Header("Content-Length");
                if (content_length.empty()) {
                    m_state = ParseState::Complete;
                } else {
                    m_state = ParseState::Body;
                    m_body_start = total_parsed;
                }
                break;
            }
            case ParseState::Body: {
                auto result = ParseBody(remaining);
                if (result.IsErr()) return result;
                total_parsed += result.Value();
                m_state = ParseState::Complete;
                break;
            }
            default:
                break;
        }
    }

    return Result<size_t, Error>::Ok(total_parsed);
}

Result<size_t, Error> CHttpParser::ParseRequestLine(BufferView data) {
    size_t pos = data.find("\r\n");
    if (pos == BufferView::npos) {
        return Result<size_t, Error>::Err(Error(ErrorCode::InvalidArgument, "Incomplete request line"));
    }

    BufferView line = data.substr(0, pos);

    size_t first_space = line.find(' ');
    if (first_space == BufferView::npos) {
        return Result<size_t, Error>::Err(Error(ErrorCode::InvalidArgument, "Invalid request line"));
    }

    BufferView method_str = line.substr(0, first_space);
    if (method_str == "GET") m_method = HttpMethod::HTTP_GET;
    else if (method_str == "POST") m_method = HttpMethod::HTTP_POST;
    else if (method_str == "PUT") m_method = HttpMethod::HTTP_PUT;
    else if (method_str == "DELETE") m_method = HttpMethod::HTTP_DELETE;
    else if (method_str == "HEAD") m_method = HttpMethod::HTTP_HEAD;
    else if (method_str == "OPTIONS") m_method = HttpMethod::HTTP_OPTIONS;
    else m_method = HttpMethod::HTTP_UNKNOWN;

    size_t second_space = line.find(' ', first_space + 1);
    if (second_space == BufferView::npos) {
        return Result<size_t, Error>::Err(Error(ErrorCode::InvalidArgument, "Invalid request line"));
    }

    m_url_start = first_space + 1;
    m_url_end = second_space;
    m_version_start = second_space + 1;
    m_version_end = line.size();

    return Result<size_t, Error>::Ok(pos + 2);//跳过/r/n
}

Result<size_t, Error> CHttpParser::ParseHeaders(BufferView data) {
    size_t pos = 0;

    while (true) {
        size_t line_end = data.find("\r\n", pos);
        if (line_end == BufferView::npos) {
            return Result<size_t, Error>::Err(Error(ErrorCode::InvalidArgument, "Incomplete headers"));
        }

        // 空行表示头部结束
        if (line_end == pos) {
            return Result<size_t, Error>::Ok(pos + 2);
        }

        BufferView line = data.substr(pos, line_end - pos);
        size_t colon = line.find(':');
        if (colon == BufferView::npos) {
            return Result<size_t, Error>::Err(Error(ErrorCode::InvalidArgument, "Invalid header"));
        }

        BufferView key = line.substr(0, colon);
        BufferView value = line.substr(colon + 1);

        while (!value.empty() && std::isspace(value.front())) {
            value.remove_prefix(1);//去掉前导空格
        }
        while (!value.empty() && std::isspace(value.back())) {
            value.remove_suffix(1);//去掉尾部空格
        }

        m_headers[Buffer(key)] = value;
        pos = line_end + 2;
    }
}

Result<size_t, Error> CHttpParser::ParseBody(BufferView data) {
    auto content_length_str = Header("Content-Length");
    if (content_length_str.empty()) {
        return Result<size_t, Error>::Ok(0);
    }

    size_t content_length = 0;
    for (char c : content_length_str) {
        //确保Content-Length是十进制数字
        if (!std::isdigit(c)) {
            return Result<size_t, Error>::Err(Error(ErrorCode::InvalidArgument, "Invalid Content-Length"));
        }
        //把字符串数字恢复为整数 ：如“123” = 0*10 + 1 = 1 -> 1*10+2  =12 -> 12*10+3 =123
        content_length = content_length * 10 + (c - '0');
    }

    if (data.size() < content_length) {
        return Result<size_t, Error>::Err(Error(ErrorCode::InvalidArgument, "Incomplete body"));
    }

    m_body_end = m_body_start + content_length;

    return Result<size_t, Error>::Ok(content_length);
}

BufferView CHttpParser::Url() const {
    return m_data.substr(m_url_start, m_url_end - m_url_start);
}

BufferView CHttpParser::Version() const {
    return m_data.substr(m_version_start, m_version_end - m_version_start);
}

BufferView CHttpParser::Header(BufferView key) const {
    auto it = m_headers.find(Buffer(key));
    if (it != m_headers.end()) {
        return it->second;
    }
    return BufferView();
}

BufferView CHttpParser::Body() const {
    if (m_body_start == 0 && m_body_end == 0) {
        return BufferView();
    }
    return m_data.substr(m_body_start, m_body_end - m_body_start);
}

} // namespace yibo
