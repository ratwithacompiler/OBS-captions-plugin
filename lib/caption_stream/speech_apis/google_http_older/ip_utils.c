static void clean_error(PError **error) {
    if (error == NULL || *error == NULL)
        return;

    p_error_free(*error);
    *error = NULL;
}

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")


static const char *host_to_ip(const char *hostname) {
    struct hostent *remoteHost = gethostbyname(hostname);
    if (remoteHost == nullptr) {
        return nullptr;
    }

    if (remoteHost->h_addrtype == AF_INET)
    {
        if (remoteHost->h_addr_list[0] != 0) {
            struct in_addr addr;
            addr.s_addr = *(u_long *) remoteHost->h_addr_list[0];
            return inet_ntoa(addr);
        }
    }
    return nullptr;
}

#else

#include <netdb.h>
#include <arpa/inet.h>

static const char *host_to_ip(const char *hostname) {
    struct hostent *hp = gethostbyname(hostname);
    if (hp == nullptr) {
        return nullptr;
    }

    struct in_addr addr;
    memcpy((char *) &addr, hp->h_addr_list[0], sizeof(addr));

    return inet_ntoa(addr);
}

#endif
