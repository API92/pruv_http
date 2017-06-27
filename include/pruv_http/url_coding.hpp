/*
 * Copyright (C) Andrey Pikas
 */

#pragma once

#include <cstddef>
#include <string_view>

namespace pruv {
namespace http {

bool is_url_encoded(std::string_view url);
char * url_decode(char *url, size_t len);

} // namespace http
} // namespace pruv
