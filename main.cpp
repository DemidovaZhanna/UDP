#include <iostream>
#include "src/Network.h"

int main()
{
    std::string IP = "127.0.0.1";
    int port_dst = 8888;
    UDPSocket socket_out;

    UDPSocket socket_in;
    char buffer[460];
    socket_in.Bind(port_dst);
    int ret;

    std::string data = "heeeey";

    for (int i = 0; i < 3; i++)
    {
        socket_out.sendTo(IP, port_dst, data.c_str(), data.size());

        ret = socket_in.tryRecvFrom();

        if (ret == 0)
            std::cout << "no sms"; // сообщений для чтения нет
        else
        {
            socket_in.recvFrom(buffer, sizeof(buffer));
            std::cout << "done!";
        }

    }
    return 0;
}
// файловый дескриптор в заголовочнике!!!