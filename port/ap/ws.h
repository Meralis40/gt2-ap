// SPDX-License-Identifier: GPL-2.0-or-later
// A websocket client, plain or over TLS, for one thread.
#pragma once

#include <memory>
#include <string>

namespace ap {

class WebSocket {
public:
    WebSocket();
    ~WebSocket();

    // address is [ws://|wss://]host[:port]. With no scheme it tries TLS, then plain, as
    // Archipelago's clients do. Returns false with the reason in error.
    bool open(const std::string &address, std::string &error);
    void close();
    bool sendText(const std::string &text);
    // Waits up to timeoutMs for a text message: 1 with one in message, 0 with none yet,
    // -1 when the connection is gone.
    int receive(std::string &message, int timeoutMs);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace ap
