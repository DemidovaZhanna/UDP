#ifndef UDP_NETWORK_H
#define UDP_NETWORK_H

#if defined(_WIN32)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <system_error>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cerrno>
#endif

#include <string>
#include <iostream>

#if defined(_WIN32)
#define ISVALIDSOCKET (s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET (s) closesocket(s)
#define GETSOCKETERRNO () (WSAGetLastError())
#else
#define SOCKET int
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define GETSOCKETERRNO() (errno)
#endif

const char *get_error_text ()
{
    #if defined(_WIN32)
        static char message[256] = {0};
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
            0, WSAGetLastError(), 0, message, 256, 0);
        char *nl = strrchr(message, '\n');
        if (nl) *nl = 0;
        return message;
    #else
        return hstrerror(errno);
    #endif
}

#if defined(_WIN32)
class WSASession
{
  public:
    WSASession()
    {
      int ret = WSAStartup(MAKEWORD(2, 2), &data);
      if (ret != 0)
        throw std::system_error(WSAGetLastError(), std::system_category(), "WSAStartup Failed");
    }
    ~WSASession()
    {
      WSACleanup();
    }

  private:
    WSAData data;
};
#endif

class UDPSocket
{
public:
    UDPSocket ()
    {
        _sockfd_in = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        _sockfd_out = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (!ISVALIDSOCKET(_sockfd_in) || !ISVALIDSOCKET(_sockfd_out))
        {
            printf("Last error was %d : %s\n", GETSOCKETERRNO(), get_error_text());
        }
    }

    void sendTo (const std::string& address, unsigned short port, const char* buffer, int len, int flags = 0)
    {
        sockaddr_in add;
        add.sin_family = AF_INET;
        add.sin_addr.s_addr = inet_addr(address.c_str());
        //add.sin_addr.s_addr = inet_pton(AF_INET, "127.0.0.1", address.c_str());
        add.sin_port = htons(port);

        #if defined(_WIN32)
            int ret = sendto(_sockfd_out, buffer, len, flags, reinterpret_cast<SOCKADDR *>(&add), sizeof(add));
        #else
            ssize_t ret = sendto(_sockfd_out, static_cast<const void *> (buffer), len, flags, reinterpret_cast<const sockaddr *>(&add), sizeof(add));
        #endif

        if (ret < 0)
            printf("Sendto failed: %d : %s\n", GETSOCKETERRNO(), get_error_text());
    }

    void sendTo (sockaddr_in& address, const char* buffer, int len, int flags = 0)
    {
        #if defined(_WIN32)
            int ret = sendto(_sockfd_out, buffer, len, flags, reinterpret_cast<SOCKADDR *>(&address), sizeof(address));
        #else
            ssize_t ret = sendto(_sockfd_out, static_cast<const void *> (buffer), len, flags, reinterpret_cast<const sockaddr *>(&address), sizeof(address));
        #endif

        if (ret < 0)
            printf("Sendto failed: %d : %s\n", GETSOCKETERRNO(), get_error_text());
    }

    int tryRecvFrom ()
    {
        FD_ZERO(&read_fds);
        FD_SET(_sockfd_in, &read_fds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        int ret = select(std::max(_sockfd_in, _sockfd_out) + 1,  &read_fds, nullptr, nullptr, &tv);
        // как узнать последний открытый файловый дескриптор, если он должен быть наибольшим в любом из трех наборов + 1

        if (ret < 0)
        {
            printf("TryRecvFrom (select) failed: %d : %s\n", GETSOCKETERRNO(), get_error_text());
            exit(1); // ошибка
        }

        return ret;
    }

    sockaddr_in recvFrom(char* buffer, int len, int flags = 0)
    {
        if ((FD_ISSET(_sockfd_in, &read_fds)))
        {
            sockaddr_in from;
            int size = sizeof(from);

            #if defined(_WIN32)
                int ret = recvfrom(_sockfd, buffer, len, flags, reinterpret_cast<SOCKADDR *>(&from), &size);
            #else
                ssize_t ret = recvfrom(_sockfd_in, static_cast<void *> (buffer), len, flags,
                                       reinterpret_cast<sockaddr *>(&from), reinterpret_cast<socklen_t *>(&size));
            #endif

            if (ret < 0)
            {
                printf("Recvfrom failed: %d : %s\n", GETSOCKETERRNO(), get_error_text());
                exit(1);
            }

            // make the buffer zero terminated
            buffer[ret] = 0;
            return from;
        }

    }

    void Bind(unsigned short port)
    {
        sockaddr_in add;
        add.sin_family = AF_INET;
        add.sin_addr.s_addr = INADDR_ANY;
        add.sin_port = htons(port);

        int ret = bind(_sockfd_in, reinterpret_cast<const sockaddr *>(&add), sizeof(add));
        if (ret < 0)
        {
            printf("Bind failed: %d : %s\n", GETSOCKETERRNO(), get_error_text());
            exit(1);
        }
    }

    ~UDPSocket()
    {
        CLOSESOCKET(_sockfd_in);
        CLOSESOCKET(_sockfd_out);
    }

private:
    SOCKET _sockfd_in;
    SOCKET _sockfd_out;
    fd_set read_fds;
};

#endif //UDP_NETWORK_H