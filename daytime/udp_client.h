#pragma once

#include <string>

// UDP-клиент: сокет создаётся в конструкторе и закрывается в деструкторе.
class UdpClient {
public:
    UdpClient(const std::string& host, const std::string& port);
    ~UdpClient();

    UdpClient(const UdpClient&) = delete;
    UdpClient& operator=(const UdpClient&) = delete;

    void send(const std::string& data);
    std::string receive(int timeout_ms);
    std::string peer() const { return peer_; }

private:
    int fd_ = -1;
    std::string peer_;
};
