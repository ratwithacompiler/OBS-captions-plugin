#ifndef CPPTESTING_TCPCONNECTION_H
#define CPPTESTING_TCPCONNECTION_H


#include <plibsys.h>
#include <iostream>

typedef unsigned int uint;
using namespace std;


class ConnectError : public std::exception {
    string error_message;
public:
    explicit ConnectError(const char *error_message) : error_message(error_message) {

    }

    const char *what() const throw() override {
        return error_message.c_str();
    }
};


class TcpConnection {

    string hostname;
    const uint port;
    string ip_address;
    PSocketAddress *p_address = nullptr;
    PSocket *p_socket = nullptr;
    bool started = false;
    bool dead = false;
    bool connected = false;

public:

    explicit TcpConnection(string hostname, uint port);

    void connect(uint timeoutMs);

    void set_timeout(uint timeoutMs);

    bool send_all(const char *buffer, const int bytes);

    int receive_at_most(char *buffer, int bytes);

    int receive_at_least(string &buffer, int bytes);

    int receive_exactly(string &buffer, int bytes);

    bool is_connected();

    bool is_dead();

    void close();

    ~TcpConnection();
};

#endif //CPPTESTING_TCPCONNECTION_H
