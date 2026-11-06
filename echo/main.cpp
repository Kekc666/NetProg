#include "tcp_client.h"

#include <getopt.h>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Options {
    std::string server;
    std::string port = "7";
    int timeout = 5;
    std::vector<std::string> messages;
};

void usage(const char* prog)
{
    std::cout << "Клиент службы echo (TCP)\n"
              << "Использование: " << prog
              << " -s <сервер> [-p <порт>] [-t <секунды>] <сообщение>...\n"
              << "  -s, --server   адрес или имя сервера (обязательный)\n"
              << "  -p, --port     порт службы, по умолчанию 7\n"
              << "  -t, --timeout  ожидание ответа, 1-60 с, по умолчанию 5\n"
              << "  -h, --help     эта справка\n"
              << "Каждое сообщение отправляется отдельно, ответ сверяется\n"
              << "с отправленным.\n";
}

int to_int(const std::string& s, int lo, int hi, const std::string& name)
{
    std::size_t pos = 0;
    int value = 0;
    try {
        value = std::stoi(s, &pos);
    } catch (const std::exception&) {
        pos = 0;
    }
    if (pos != s.size() || value < lo || value > hi)
        throw std::invalid_argument(name + " должен быть числом от " +
                                    std::to_string(lo) + " до " +
                                    std::to_string(hi));
    return value;
}

// Возвращает false, если нужно только показать справку.
bool parse(int argc, char* argv[], Options& opt)
{
    const option longopts[] = {
        {"server", required_argument, nullptr, 's'},
        {"port", required_argument, nullptr, 'p'},
        {"timeout", required_argument, nullptr, 't'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0},
    };
    if (argc == 1)
        return false;
    opterr = 0;
    int c;
    while ((c = getopt_long(argc, argv, ":s:p:t:h", longopts, nullptr)) != -1) {
        switch (c) {
        case 's':
            opt.server = optarg;
            break;
        case 'p':
            opt.port = std::to_string(to_int(optarg, 1, 65535, "порт"));
            break;
        case 't':
            opt.timeout = to_int(optarg, 1, 60, "таймаут");
            break;
        case 'h':
            return false;
        case ':':
            throw std::invalid_argument("у параметра нет значения");
        default:
            throw std::invalid_argument("неизвестный параметр");
        }
    }
    for (int i = optind; i < argc; ++i)
        opt.messages.emplace_back(argv[i]);
    if (opt.server.empty())
        throw std::invalid_argument("не задан сервер (-s)");
    if (opt.messages.empty())
        throw std::invalid_argument("не задано ни одного сообщения");
    return true;
}

}

int main(int argc, char* argv[])
{
    try {
        Options opt;
        if (!parse(argc, argv, opt)) {
            usage(argv[0]);
            return 0;
        }
        TcpClient client(opt.server, opt.port, opt.timeout * 1000);
        std::cout << "Соединение с " << client.peer() << " установлено\n";

        std::size_t matched = 0;
        for (const std::string& msg : opt.messages) {
            client.send_all(msg + "\n");
            std::string reply = client.recv_exactly(msg.size() + 1);
            reply.pop_back();
            std::cout << "> " << msg << "\n< " << reply << '\n';
            if (reply == msg)
                ++matched;
        }
        std::cout << "Совпало " << matched << " из " << opt.messages.size()
                  << '\n';
        return matched == opt.messages.size() ? 0 : 1;
    } catch (const std::invalid_argument& e) {
        std::cerr << "Ошибка в параметрах: " << e.what() << '\n'
                  << "Справка: " << argv[0] << " --help\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << '\n';
        return 1;
    }
}
