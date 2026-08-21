#define IP_STR_BUFFER_SIZE 64

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#endif

static bool host_to_ip(const char *hostname, char *ip_out, size_t ip_out_size) {
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, NULL, &hints, &result) != 0 || result == NULL)
        return false;

    bool ok = false;
    for (struct addrinfo *entry = result; entry != NULL; entry = entry->ai_next) {
        if (entry->ai_family != AF_INET || entry->ai_addr == NULL)
            continue;

        if (getnameinfo(entry->ai_addr, (socklen_t) entry->ai_addrlen, ip_out, (socklen_t) ip_out_size,
                        NULL, 0, NI_NUMERICHOST) == 0)
            ok = true;
        break;
    }

    freeaddrinfo(result);
    return ok;
}
