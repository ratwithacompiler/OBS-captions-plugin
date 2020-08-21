/******************************************************************************
Copyright (C) 2019 by <rat.with.a.compiler@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

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

    bool is_connected();

    bool is_dead();

    void close();

    ~TcpConnection();
};

#endif //CPPTESTING_TCPCONNECTION_H
