//
// Created by Rat on 2019-06-09.
//

#include "TcpConnection.h"

#include "utils.h"
#include "log.h"

#define READ_BUFFER_SIZE 2048

//testing
#define SOCKET_RECV_BUFFER_SIZE 4096
#define SOCKET_SEND_BUFFER_SIZE 1024

TcpConnection::TcpConnection(string hostname, uint port)
        : hostname(hostname), port(port) {
    debug_log("TcpConnection!!!!!!!!!!");
}

void TcpConnection::connect(uint timeoutMs) {
    if (started)
        throw ConnectError("connection already started, wuttt");

    started = true;

    const char *ip_str = host_to_ip(hostname.c_str());

    if (ip_str == nullptr)
        throw ConnectError("couldn't resolve hostname");

    ip_address = ip_str;
    debug_log("address %s %s", ip_address.c_str(), ip_str);

    // Construct address for server.  Since the server is assumed to be on the same machine for the sake of this program, the address is loopback, but typically this would be an external address.
    if ((p_address = p_socket_address_new(ip_address.c_str(), port)) == nullptr)
        throw ConnectError("couldn't create address");


    // Create p_socket
    if ((p_socket = p_socket_new(P_SOCKET_FAMILY_INET, P_SOCKET_TYPE_STREAM, P_SOCKET_PROTOCOL_TCP, nullptr)) == nullptr) {
        throw ConnectError("couldn't create p_socket");
    }

//    info_log("blocking: %d\n", p_socket_get_blocking(p_socket));
    if (timeoutMs) {
//        debug_log("timeout: %d", p_socket_get_timeout(p_socket));
        p_socket_set_timeout(p_socket, timeoutMs);
//        debug_log("timeout: %d", p_socket_get_timeout(p_socket));
    }


//    p_socket_set_buffer_size(p_socket, P_SOCKET_DIRECTION_RCV, 1024, nullptr);
//    p_socket_set_buffer_size(p_socket, P_SOCKET_DIRECTION_RCV, SOCKET_RECV_BUFFER_SIZE, nullptr);
//    p_socket_set_buffer_size(p_socket, P_SOCKET_DIRECTION_SND, SOCKET_SEND_BUFFER_SIZE, nullptr);
//    info_log("buffer size", );

    // Connect to server.
    if (!p_socket_connect(p_socket, p_address, nullptr)) {
        throw ConnectError("couldn't connect to server");
    }

    debug_log("Connected!!!!!!!!!");
    connected = true;
}


bool TcpConnection::is_connected() {
    return connected;
}

bool TcpConnection::is_dead() {
    return dead;
}

void TcpConnection::close() {
    if (p_socket != nullptr) {
        debug_log("freeing p_socket");
        p_socket_close(p_socket, nullptr);
        p_socket = nullptr;
    }

    if (p_address != nullptr) {
        debug_log("freeing addrress");
        p_socket_address_free(p_address);
        p_address = nullptr;
    }
}

TcpConnection::~TcpConnection() {
    debug_log("~TcpConnection decons()");
    close();
}

void TcpConnection::set_timeout(uint timeoutMs) {
    if (!p_socket)
        return;

    p_socket_set_timeout(p_socket, timeoutMs);
}

bool TcpConnection::send_all(const char *buffer, const int bytes) {
    if (!p_socket || !bytes)
        return false;

    int left = bytes;
    while (left > 0) {
        int just_sent = p_socket_send(p_socket, buffer + bytes - left, left, nullptr);
        if (just_sent == -1)
            return false;

        left -= just_sent;
    }
    return true;
}

int TcpConnection::receive_at_most(char *buffer, int bytes) {
    return p_socket_receive(p_socket, buffer, bytes, nullptr);
}

int TcpConnection::receive_exactly(string &buffer, int bytes) {
    int read = 0;
    int cur_read = 0;
    char chunk[READ_BUFFER_SIZE];
    while (read < bytes) {
        int needed = bytes - read;
        if ((cur_read = p_socket_receive(p_socket, chunk, needed, nullptr)) == -1)
            return -1;

        read += cur_read;
        buffer.append(chunk, cur_read);
    }

    return read;
}

int TcpConnection::receive_at_least(string &buffer, int bytes) {
    int read = 0;
    int cur_read = 0;
    char chunk[READ_BUFFER_SIZE];
    while (read < bytes) {
        if ((cur_read = p_socket_receive(p_socket, chunk, READ_BUFFER_SIZE, nullptr)) == -1)
            return -1;

        read += cur_read;
        buffer.append(chunk, cur_read);
    }

    return read;
}