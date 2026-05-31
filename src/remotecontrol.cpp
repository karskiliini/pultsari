#include "remotecontrol.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

RemoteControl::RemoteControl(uint16_t port)
    : serverFd(-1), clientFd(-1), port(port)
{
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind to port " << port << std::endl;
        close(serverFd);
        serverFd = -1;
        return;
    }

    if (listen(serverFd, 1) < 0) {
        std::cerr << "Failed to listen" << std::endl;
        close(serverFd);
        serverFd = -1;
        return;
    }
}

RemoteControl::~RemoteControl()
{
    disconnect();
    if (serverFd >= 0) {
        close(serverFd);
    }
}

bool RemoteControl::waitForConnection()
{
    if (serverFd < 0) return false;

    std::cout << "Waiting for remote connection on port " << port << "..." << std::endl;

    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &addrLen);

    if (clientFd < 0) {
        std::cerr << "Failed to accept connection" << std::endl;
        return false;
    }

    std::cout << "Remote client connected!" << std::endl;
    return true;
}

char RemoteControl::readChar()
{
    if (clientFd < 0) return 0;

    char c = 0;
    ssize_t n = read(clientFd, &c, 1);
    if (n <= 0) {
        disconnect();
        return 0;
    }
    return c;
}

bool RemoteControl::isConnected() const
{
    return clientFd >= 0;
}

void RemoteControl::disconnect()
{
    if (clientFd >= 0) {
        close(clientFd);
        clientFd = -1;
    }
}
