// SPDX-License-Identifier: GPL-2.0-or-later
// RFC 6455 over a non-blocking socket, with mbedTLS for wss, and RFC 7692 compression:
// Archipelago's protocol deprecates connections without it. The handshake asks the server
// to start each message afresh, so none needs the window of the one before. What the client
// sends stays uncompressed, which the extension allows, message by message.
#include "ws.h"

#include "gt2ap/discio/gzip.hpp"

#include <mbedtls/base64.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
#include <psa/crypto.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <exception>
#include <random>
#include <span>
#include <vector>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wincrypt.h>
using SocketHandle = SOCKET;
constexpr SocketHandle kNoSocket = INVALID_SOCKET;
#else
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketHandle = int;
constexpr SocketHandle kNoSocket = -1;
#endif

namespace ap {
namespace {

using Clock = std::chrono::steady_clock;

constexpr int kConnectSeconds = 10;
constexpr int kHandshakeSeconds = 10;
constexpr const char *kDefaultPort = "38281";
constexpr std::size_t kMaxMessage = 64 * 1024 * 1024;

enum Opcode { kContinuation = 0, kText = 1, kBinary = 2, kClose = 8, kPing = 9, kPong = 10 };

void closeSocket(SocketHandle handle)
{
#if defined(_WIN32)
    closesocket(handle);
#else
    ::close(handle);
#endif
}

bool wouldBlock()
{
#if defined(_WIN32)
    int e = WSAGetLastError();
    return e == WSAEWOULDBLOCK || e == WSAEINPROGRESS;
#else
    return errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS || errno == EINTR;
#endif
}

void setNonBlocking(SocketHandle handle)
{
#if defined(_WIN32)
    u_long on = 1;
    ioctlsocket(handle, FIONBIO, &on);
#else
    fcntl(handle, F_SETFL, fcntl(handle, F_GETFL, 0) | O_NONBLOCK);
#endif
}

// Whether the socket became ready within timeoutMs.
bool waitFor(SocketHandle handle, bool forWrite, int timeoutMs)
{
    fd_set set, failed;
    FD_ZERO(&set);
    FD_SET(handle, &set);
    // Windows reports a failed connect here, not as writable.
    FD_ZERO(&failed);
    FD_SET(handle, &failed);
    timeval timeout{timeoutMs / 1000, (timeoutMs % 1000) * 1000};
    int count = static_cast<int>(handle) + 1;
    return select(count, forWrite ? nullptr : &set, forWrite ? &set : nullptr, &failed, &timeout) > 0;
}

SocketHandle connectTo(const std::string &host, const std::string &port, std::string &error)
{
    addrinfo hints{};
    addrinfo *found = nullptr;

    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &found) != 0) {
        error = "cannot find " + host;
        return kNoSocket;
    }
    SocketHandle handle = kNoSocket;
    error = "cannot reach " + host + ":" + port;
    for (addrinfo *address = found; address != nullptr; address = address->ai_next) {
        SocketHandle candidate = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (candidate == kNoSocket) {
            continue;
        }
        setNonBlocking(candidate);
        int result = connect(candidate, address->ai_addr, static_cast<int>(address->ai_addrlen));
        if ((result == 0 || wouldBlock()) && waitFor(candidate, true, kConnectSeconds * 1000)) {
            int failure = 0;
            socklen_t size = sizeof(failure);
            getsockopt(candidate, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&failure), &size);
            if (failure == 0) {
                handle = candidate;
                break;
            }
        }
        closeSocket(candidate);
    }
    freeaddrinfo(found);
    return handle;
}

std::string tlsError(int code)
{
    char text[128];

    mbedtls_strerror(code, text, sizeof(text));
    return text;
}

int tlsSend(void *context, const unsigned char *data, std::size_t size)
{
    auto handle = *static_cast<SocketHandle *>(context);
    int sent = send(handle, reinterpret_cast<const char *>(data), static_cast<int>(size), 0);
    if (sent < 0) {
        return wouldBlock() ? MBEDTLS_ERR_SSL_WANT_WRITE : MBEDTLS_ERR_NET_SEND_FAILED;
    }
    return sent;
}

int tlsReceive(void *context, unsigned char *data, std::size_t size)
{
    auto handle = *static_cast<SocketHandle *>(context);
    int got = recv(handle, reinterpret_cast<char *>(data), static_cast<int>(size), 0);
    if (got < 0) {
        return wouldBlock() ? MBEDTLS_ERR_SSL_WANT_READ : MBEDTLS_ERR_NET_RECV_FAILED;
    }
    return got;
}

void loadSystemRoots(mbedtls_x509_crt *roots)
{
#if defined(_WIN32)
    HCERTSTORE store = CertOpenSystemStoreA(0, "ROOT");
    if (store == nullptr) {
        return;
    }
    const CERT_CONTEXT *cert = nullptr;
    while ((cert = CertEnumCertificatesInStore(store, cert)) != nullptr) {
        mbedtls_x509_crt_parse_der(roots, cert->pbCertEncoded, cert->cbCertEncoded);
    }
    CertCloseStore(store, 0);
#else
    mbedtls_x509_crt_parse_path(roots, "/etc/ssl/certs");
#endif
}

struct Address {
    bool tryTls = true, tryPlain = true;
    std::string host, port;
};

Address parseAddress(std::string text)
{
    Address out;

    if (text.rfind("wss://", 0) == 0) {
        out.tryPlain = false;
        text.erase(0, 6);
    } else if (text.rfind("ws://", 0) == 0) {
        out.tryTls = false;
        text.erase(0, 5);
    }
    text.erase(text.find_last_not_of("/ ") + 1);
    std::size_t colon = text.rfind(':');
    out.host = text.substr(0, colon);
    out.port = colon == std::string::npos ? kDefaultPort : text.substr(colon + 1);
    return out;
}

}  // namespace

struct WebSocket::Impl {
    SocketHandle handle = kNoSocket;
    bool tls = false;
    bool tlsReady = false;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config config;
    mbedtls_ctr_drbg_context random;
    mbedtls_entropy_context entropy;
    mbedtls_x509_crt roots;
    std::vector<std::uint8_t> input;
    std::string pending;  // a message arriving in fragments
    bool pendingCompressed = false;
    bool compression = false;
    std::mt19937 maskRandom{std::random_device{}()};

    void reset()
    {
        if (tlsReady) {
            if (handle != kNoSocket) {
                mbedtls_ssl_close_notify(&ssl);
            }
            mbedtls_ssl_free(&ssl);
            mbedtls_ssl_config_free(&config);
            mbedtls_ctr_drbg_free(&random);
            mbedtls_entropy_free(&entropy);
            mbedtls_x509_crt_free(&roots);
            tlsReady = false;
        }
        if (handle != kNoSocket) {
            closeSocket(handle);
            handle = kNoSocket;
        }
        input.clear();
        pending.clear();
        pendingCompressed = false;
        compression = false;
    }

    bool startTls(const std::string &host, std::string &error)
    {
        psa_crypto_init();
        mbedtls_ssl_init(&ssl);
        mbedtls_ssl_config_init(&config);
        mbedtls_ctr_drbg_init(&random);
        mbedtls_entropy_init(&entropy);
        mbedtls_x509_crt_init(&roots);
        tlsReady = true;
        int result = mbedtls_ctr_drbg_seed(&random, mbedtls_entropy_func, &entropy, nullptr, 0);
        if (result == 0) {
            result = mbedtls_ssl_config_defaults(&config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                                 MBEDTLS_SSL_PRESET_DEFAULT);
        }
        if (result != 0) {
            error = tlsError(result);
            return false;
        }
        loadSystemRoots(&roots);
        mbedtls_ssl_conf_ca_chain(&config, &roots, nullptr);
        mbedtls_ssl_conf_authmode(&config, MBEDTLS_SSL_VERIFY_REQUIRED);
        mbedtls_ssl_conf_rng(&config, mbedtls_ctr_drbg_random, &random);
        result = mbedtls_ssl_setup(&ssl, &config);
        if (result == 0) {
            result = mbedtls_ssl_set_hostname(&ssl, host.c_str());
        }
        if (result != 0) {
            error = tlsError(result);
            return false;
        }
        mbedtls_ssl_set_bio(&ssl, &handle, tlsSend, tlsReceive, nullptr);
        auto deadline = Clock::now() + std::chrono::seconds(kHandshakeSeconds);
        for (;;) {
            result = mbedtls_ssl_handshake(&ssl);
            if (result == 0) {
                return true;
            }
            if (result != MBEDTLS_ERR_SSL_WANT_READ && result != MBEDTLS_ERR_SSL_WANT_WRITE) {
                error = tlsError(result);
                return false;
            }
            if (Clock::now() > deadline) {
                error = "the TLS handshake timed out";
                return false;
            }
            waitFor(handle, result == MBEDTLS_ERR_SSL_WANT_WRITE, 100);
        }
    }

    bool writeAll(const void *data, std::size_t size)
    {
        auto *bytes = static_cast<const std::uint8_t *>(data);
        auto deadline = Clock::now() + std::chrono::seconds(kHandshakeSeconds);

        while (size > 0) {
            int sent;
            bool forWrite = true;
            if (tls) {
                sent = mbedtls_ssl_write(&ssl, bytes, size);
                if (sent == MBEDTLS_ERR_SSL_WANT_READ) {
                    forWrite = false;
                }
                if (sent == MBEDTLS_ERR_SSL_WANT_READ || sent == MBEDTLS_ERR_SSL_WANT_WRITE) {
                    sent = 0;
                }
            } else {
                sent = send(handle, reinterpret_cast<const char *>(bytes), static_cast<int>(size), 0);
                if (sent < 0 && wouldBlock()) {
                    sent = 0;
                }
            }
            if (sent < 0 || Clock::now() > deadline) {
                return false;
            }
            bytes += sent;
            size -= static_cast<std::size_t>(sent);
            if (sent == 0) {
                waitFor(handle, forWrite, 100);
            }
        }
        return true;
    }

    // Appends what can be read now to input. False when the connection has ended.
    bool readSome()
    {
        std::uint8_t chunk[16384];

        for (;;) {
            int got;
            if (tls) {
                got = mbedtls_ssl_read(&ssl, chunk, sizeof(chunk));
                if (got == MBEDTLS_ERR_SSL_WANT_READ || got == MBEDTLS_ERR_SSL_WANT_WRITE) {
                    return true;
                }
                if (got == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET) {
                    continue;
                }
            } else {
                got = recv(handle, reinterpret_cast<char *>(chunk), sizeof(chunk), 0);
                if (got < 0 && wouldBlock()) {
                    return true;
                }
            }
            if (got <= 0) {
                return false;
            }
            input.insert(input.end(), chunk, chunk + got);
        }
    }

    bool handshake(const std::string &host, const std::string &port, std::string &error)
    {
        std::uint8_t key[16];
        unsigned char encoded[32];
        std::size_t encodedSize = 0;

        for (auto &byte : key) {
            byte = static_cast<std::uint8_t>(maskRandom());
        }
        mbedtls_base64_encode(encoded, sizeof(encoded), &encodedSize, key, sizeof(key));
        std::string request = "GET / HTTP/1.1\r\nHost: " + host + ":" + port +
                              "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: " +
                              std::string(reinterpret_cast<char *>(encoded), encodedSize) +
                              "\r\nSec-WebSocket-Version: 13"
                              "\r\nSec-WebSocket-Extensions: permessage-deflate; server_no_context_takeover; "
                              "client_no_context_takeover\r\n\r\n";
        if (!writeAll(request.data(), request.size())) {
            error = "the connection closed";
            return false;
        }
        auto deadline = Clock::now() + std::chrono::seconds(kHandshakeSeconds);
        static const char kEnd[] = "\r\n\r\n";
        for (;;) {
            if (!readSome()) {
                error = "the connection closed";
                return false;
            }
            auto end = std::search(input.begin(), input.end(), kEnd, kEnd + 4);
            if (end != input.end()) {
                std::string head(input.begin(), end);
                input.erase(input.begin(), end + 4);
                if (head.rfind("HTTP/1.1 101", 0) != 0) {
                    error = "the server refused the websocket: " + head.substr(0, head.find('\r'));
                    return false;
                }
                std::transform(head.begin(), head.end(), head.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                compression = head.find("permessage-deflate") != std::string::npos;
                if (compression && head.find("server_no_context_takeover") == std::string::npos) {
                    error = "the server keeps its compression window between messages";
                    return false;
                }
                return true;
            }
            if (Clock::now() > deadline) {
                error = "the websocket handshake timed out";
                return false;
            }
            waitFor(handle, false, 100);
        }
    }

    bool sendFrame(int opcode, const void *data, std::size_t size)
    {
        std::vector<std::uint8_t> frame;
        std::uint32_t mask = maskRandom();
        auto *maskBytes = reinterpret_cast<std::uint8_t *>(&mask);

        frame.reserve(size + 14);
        frame.push_back(static_cast<std::uint8_t>(0x80 | opcode));
        if (size < 126) {
            frame.push_back(static_cast<std::uint8_t>(0x80 | size));
        } else if (size < 65536) {
            frame.push_back(0x80 | 126);
            frame.push_back(static_cast<std::uint8_t>(size >> 8));
            frame.push_back(static_cast<std::uint8_t>(size));
        } else {
            frame.push_back(0x80 | 127);
            for (int shift = 56; shift >= 0; shift -= 8) {
                frame.push_back(static_cast<std::uint8_t>(static_cast<std::uint64_t>(size) >> shift));
            }
        }
        frame.insert(frame.end(), maskBytes, maskBytes + 4);
        auto *bytes = static_cast<const std::uint8_t *>(data);
        for (std::size_t i = 0; i < size; i++) {
            frame.push_back(bytes[i] ^ maskBytes[i & 3]);
        }
        return writeAll(frame.data(), frame.size());
    }

    // A compressed message is deflate data cut after a flush, with the flush's last four
    // bytes dropped. Those end an empty stored block; a last one after them ends the stream.
    bool inflateMessage(std::string &text)
    {
        static const char kEnd[] = {0, 0, char(0xff), char(0xff), 1, 0, 0, char(0xff), char(0xff)};

        text.append(kEnd, sizeof(kEnd));
        try {
            auto out = gt2ap::discio::inflate(std::span(reinterpret_cast<const std::uint8_t *>(text.data()), text.size()));
            text.assign(reinterpret_cast<const char *>(out.data()), out.size());
            return true;
        } catch (const std::exception &) {
            return false;
        }
    }

    // Takes a whole frame off input. 1 with a complete message, 0 for more, -1 to close.
    int nextMessage(std::string &message)
    {
        for (;;) {
            if (input.size() < 2) {
                return 0;
            }
            bool final = input[0] & 0x80;
            bool compressed = input[0] & 0x40;
            int opcode = input[0] & 0xf;
            bool masked = input[1] & 0x80;
            std::uint64_t size = input[1] & 0x7f;
            std::size_t head = 2;
            if (size == 126) {
                if (input.size() < 4) {
                    return 0;
                }
                size = static_cast<std::uint64_t>(input[2]) << 8 | input[3];
                head = 4;
            } else if (size == 127) {
                if (input.size() < 10) {
                    return 0;
                }
                size = 0;
                for (int i = 0; i < 8; i++) {
                    size = size << 8 | input[2 + i];
                }
                head = 10;
            }
            if (masked || size > kMaxMessage) {
                return -1;  // servers don't mask
            }
            if (input.size() < head + size) {
                return 0;
            }
            auto *payload = reinterpret_cast<const char *>(input.data() + head);
            std::size_t length = static_cast<std::size_t>(size);
            int result = 0;
            switch (opcode) {
            case kText:
            case kBinary:
            case kContinuation:
                if (opcode != kContinuation) {
                    pendingCompressed = compressed;
                }
                pending.append(payload, length);
                if (final) {
                    if (pendingCompressed && !inflateMessage(pending)) {
                        return -1;
                    }
                    message.swap(pending);
                    pending.clear();
                    result = 1;
                }
                break;
            case kPing:
                if (!sendFrame(kPong, payload, length)) {
                    return -1;
                }
                break;
            case kClose:
                return -1;
            default:
                break;
            }
            input.erase(input.begin(), input.begin() + static_cast<std::ptrdiff_t>(head + length));
            if (result != 0) {
                return result;
            }
        }
    }

    bool tryOpen(const std::string &host, const std::string &port, bool useTls, std::string &error)
    {
        reset();
        tls = useTls;
        handle = connectTo(host, port, error);
        if (handle == kNoSocket) {
            return false;
        }
        if (useTls && !startTls(host, error)) {
            reset();
            return false;
        }
        if (!handshake(host, port, error)) {
            reset();
            return false;
        }
        return true;
    }
};

WebSocket::WebSocket() : impl_(std::make_unique<Impl>())
{
#if defined(_WIN32)
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);
#endif
}

WebSocket::~WebSocket()
{
    impl_->reset();
}

bool WebSocket::open(const std::string &address, std::string &error)
{
    Address target = parseAddress(address);

    if (target.host.empty()) {
        error = "no server address";
        return false;
    }
    if (target.tryTls && impl_->tryOpen(target.host, target.port, true, error)) {
        return true;
    }
    if (target.tryPlain) {
        return impl_->tryOpen(target.host, target.port, false, error);
    }
    return false;
}

void WebSocket::close()
{
    impl_->reset();
}

bool WebSocket::sendText(const std::string &text)
{
    return impl_->handle != kNoSocket && impl_->sendFrame(kText, text.data(), text.size());
}

int WebSocket::receive(std::string &message, int timeoutMs)
{
    Impl &s = *impl_;

    if (s.handle == kNoSocket) {
        return -1;
    }
    int result = s.nextMessage(message);
    if (result != 0) {
        return result;
    }
    if (!(s.tls && mbedtls_ssl_get_bytes_avail(&s.ssl) > 0)) {
        waitFor(s.handle, false, timeoutMs);
    }
    if (!s.readSome()) {
        result = s.nextMessage(message);
        return result != 0 ? result : -1;
    }
    return s.nextMessage(message);
}

}  // namespace ap
