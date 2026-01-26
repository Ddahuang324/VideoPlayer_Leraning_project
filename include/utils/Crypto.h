#pragma once

#include "common/Public.h"

namespace yibo {

class Crypto {
public:
    static Buffer MD5(const Buffer& text);
};

} // namespace yibo
