#ifndef REMOTECONTROL_HPP
#define REMOTECONTROL_HPP

#include <cstdint>

class RemoteControl {
public:
    explicit RemoteControl(uint16_t port = 9999);
    ~RemoteControl();

    RemoteControl(const RemoteControl&) = delete;
    RemoteControl& operator=(const RemoteControl&) = delete;

    bool waitForConnection();
    char readChar();
    bool isConnected() const;
    void disconnect();

private:
    int serverFd;
    int clientFd;
    uint16_t port;
};

#endif
