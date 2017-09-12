#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <memory>

#include "libevent_headers.h"
#include "tcpconnection.h"

class Timer;

class TcpClient {
  public:
    TcpClient(struct event_base *base, const char *name, int checkInterval);
    ~TcpClient();

  public:
    bool connect(const std::string& serverAdress, int port);

    int send(const unsigned char *buffer, int size);

    void close();

    TcpConnPtr getConn();

    void setHeartBeat(bool isSendHeartBeat, int sendSeonds, int invaildSeconds);

    void setConnectionCallback(const ConnectionCallBack& cb) {
        connectionCallback_ = cb;
    }

    void setMessageCallback(MessageCallBack cb) {
        messageCallback_ = cb;
    }

    void setCloseCallback(CloseCallBack cb) {
        closeCallback_ = cb;
    }

    void setHBCallback(HeartBeatCallBack cb) {
        HBCallback_ = cb;
    }

  private:
    void newConnection();

  private:
    struct event_base* base_;
    const std::string name_;
    bool connect_;
    int checkInterval_;

    TcpConnPtr connection_;
    std::unique_ptr<Timer> timer_;

    ConnectionCallBack connectionCallback_;
    MessageCallBack messageCallback_;
    CloseCallBack closeCallback_;
    HeartBeatCallBack HBCallback_;

    bool sendHeartBeat_;
    int heartBeatInterval_;
    int invaildInterval_;
};

#endif // TCPCLIENT_H
