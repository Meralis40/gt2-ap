// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdexcept>
#include <string>

namespace gt2ap::discio {

// Thrown for malformed or unsupported disc images and I/O failures.
class DiscError : public std::runtime_error {
public:
    explicit DiscError(const std::string& message) : std::runtime_error(message) {}
};

} // namespace gt2ap::discio
