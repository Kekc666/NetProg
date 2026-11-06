#include "tcp_client.h"

#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <stdexcept>
#include <system_error>

namespace {

std::system_error sys_error(int code, const std::string& what)
{
    return std::system_error(code, std::generic_category(), what);
}

}

TcpClient::TcpClient(const std::string& host, const std::string& port,
                     int timeout_ms)
    : timeout_ms_(timeout_ms)
{
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* list = nullptr;
    int rc = getaddrinfo(host.c_str(), port.c_str(), &hints, &list);
    if (rc != 0)
        throw std::runtime_error("не удалось разрешить адрес " + host + ": " +
                                 gai_strerror(rc));

    char ip[NI_MAXHOST];
    char serv[NI_MAXSERV];
    getnameinfo(list->ai_addr, list->ai_addrlen, ip, sizeof ip, serv,
                sizeof serv, NI_NUMERICHOST | NI_NUMERICSERV);
    peer_ = std::string(ip) + ":" + serv;

    fd_ = socket(list->ai_family, list->ai_socktype, list->ai_protocol);
    if (fd_ < 0) {
        freeaddrinfo(list);
        throw sys_error(errno, "socket");
    }

    // Неблокирующий connect: ждём завершения через poll не дольше таймаута.
    fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL) | O_NONBLOCK);
    rc = connect(fd_, list->ai_addr, list->ai_addrlen);
    int err = errno;
    freeaddrinfo(list);
    try {
        if (rc < 0 && err != EINPROGRESS)
            throw sys_error(err, "connect " + peer_);
        wait(POLLOUT, "соединение");
        socklen_t len = sizeof err;
        getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &len);
        if (err != 0)
            throw sys_error(err, "connect " + peer_);
    } catch (...) {
        close(fd_);
        throw;
    }
}

TcpClient::~TcpClient()
{
    if (fd_ >= 0)
        close(fd_);
}

void TcpClient::wait(short events, const char* what)
{
    pollfd pfd{fd_, events, 0};
    int ready = poll(&pfd, 1, timeout_ms_);
    if (ready < 0)
        throw sys_error(errno, "poll");
    if (ready == 0)
        throw std::runtime_error(std::string(what) + ": сервер " + peer_ +
                                 " не ответил за " +
                                 std::to_string(timeout_ms_ / 1000) + " с");
}

void TcpClient::send_all(const std::string& data)
{
    std::size_t sent = 0;
    while (sent < data.size()) {
        wait(POLLOUT, "отправка");
        ssize_t n = ::send(fd_, data.data() + sent, data.size() - sent,
                           MSG_NOSIGNAL);
        if (n < 0 && errno != EAGAIN)
            throw sys_error(errno, "send");
        if (n > 0)
            sent += n;
    }
}

std::string TcpClient::recv_exactly(std::size_t size)
{
    std::string result;
    char buf[1024];
    while (result.size() < size) {
        wait(POLLIN, "приём");
        std::size_t want = size - result.size();
        ssize_t n = recv(fd_, buf, want < sizeof buf ? want : sizeof buf, 0);
        if (n == 0)
            throw std::runtime_error("сервер закрыл соединение");
        if (n < 0 && errno != EAGAIN)
            throw sys_error(errno, "recv");
        if (n > 0)
            result.append(buf, n);
    }
    return result;
}
