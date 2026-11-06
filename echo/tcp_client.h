#pragma once

#include <cstddef>
#include <string>

// TCP-клиент: соединение устанавливается в конструкторе с ограничением
// по времени и закрывается в деструкторе.
class TcpClient {
public:
    TcpClient(const std::string& host, const std::string& port, int timeout_ms);
    ~TcpClient();

    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    void send_all(const std::string& data);
    std::string recv_exactly(std::size_t size);
    std::string peer() const { return peer_; }

private:
    void wait(short events, const char* what);

    int fd_ = -1;
    int timeout_ms_;
    std::string peer_;
};
