#ifndef UTILS_H
#define UTILS_H

#include <cstring>
#include <cstdlib>

#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>


static void clean_error(PError **error) {
    if (error == NULL || *error == NULL)
        return;

    p_error_free(*error);
    *error = NULL;
}

//#define A
//#ifdef A
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
#include <Tcl/tcl.h>
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

static std::string random_string(const int count) {
    std::string output;
    static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < count; ++i) {
        output.push_back(alphanum[rand() % (sizeof(alphanum) - 1)]);
    }

    return output;
}


static int word_filter(const string &input, string &output, const vector<string> &banned_words_lowercase) {
    if (input.empty())
        return 0;

    int removed_cnt = 0;
    istringstream stream(input);
    string word;
    while (getline(stream, word, ' ')) {
//        cout << "word '" << word << "'" << endl;

        if (word.empty()) {
            output.append(" ");
            continue;
        }

        string lower_word(word);
        std::transform(lower_word.begin(), lower_word.end(), lower_word.begin(), ::tolower);

        if (std::find(banned_words_lowercase.begin(), banned_words_lowercase.end(), lower_word) != banned_words_lowercase.end()) {
            removed_cnt++;
//            cout << "AHH banned word '" << word << "'" << endl;
            continue;
        }

        output.append(word);
        if (!stream.eof())
            output.append(" ");
    }
    return removed_cnt;
}


#endif // UTILS_H