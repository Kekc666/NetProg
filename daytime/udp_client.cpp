#include "udp_client.h"

#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <stdexcept>
#include <system_error>

namespace {

std::system_error sys_error(const std::string& what)
{
    return std::system_error(errno, std::generic_category(), what);
}

}

UdpClient::UdpClient(const std::string& host, const std::string& port)
{
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    addrinfo* list = nullptr;
    int rc = getaddrinfo(host.c_str(), port.c_str(), &hints, &list);
    if (rc != 0)
        throw std::runtime_error("не удалось разрешить адрес " + host + ": " +
                                 gai_strerror(rc));

    fd_ = socket(list->ai_family, list->ai_socktype, list->ai_protocol);
    if (fd_ < 0) {
        freeaddrinfo(list);
        throw sys_error("socket");
    }
    // connect() для UDP только запоминает адрес сервера: дальше можно
    // использовать send/recv, а ответы от чужих адресов ядро отбросит.
    if (connect(fd_, list->ai_addr, list->ai_addrlen) < 0) {
        freeaddrinfo(list);
        close(fd_);
        throw sys_error("connect");
    }

    char ip[NI_MAXHOST];
    char serv[NI_MAXSERV];
    getnameinfo(list->ai_addr, list->ai_addrlen, ip, sizeof ip, serv,
                sizeof serv, NI_NUMERICHOST | NI_NUMERICSERV);
    peer_ = std::string(ip) + ":" + serv;
    freeaddrinfo(list);
}

UdpClient::~UdpClient()
{
    if (fd_ >= 0)
        close(fd_);
}

void UdpClient::send(const std::string& data)
{
    if (::send(fd_, data.data(), data.size(), 0) < 0)
        throw sys_error("send");
}

std::string UdpClient::receive(int timeout_ms)
{
    pollfd pfd{fd_, POLLIN, 0};
    int ready = poll(&pfd, 1, timeout_ms);
    if (ready < 0)
        throw sys_error("poll");
    if (ready == 0)
        throw std::runtime_error("сервер " + peer_ + " не ответил за " +
                                 std::to_string(timeout_ms / 1000) + " с");

    char buf[512];
    ssize_t n = recv(fd_, buf, sizeof buf, 0);
    if (n < 0)
        throw sys_error("recv");
    return std::string(buf, n);
}
