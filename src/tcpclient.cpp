#include "tcpclient.h"

#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "timer.h"
#include "logging.h"

TcpClient::TcpClient(event_base *base, const char *name, int checkInterval)
    :base_(base),
     name_(name),
     connect_(false),
     checkInterval_(checkInterval),
     connection_(new TcpConnection(base_, -1, name)) {

}

TcpClient::~TcpClient() {

}

bool TcpClient::connect(const std::string &serverAdress, int port) {
    struct sockaddr_in tSockAddr;
    memset(&tSockAddr, 0, sizeof(tSockAddr));
    tSockAddr.sin_family = AF_INET;
    tSockAddr.sin_addr.s_addr = inet_addr(serverAdress.c_str());
    tSockAddr.sin_port = htons(port);

    if( bufferevent_socket_connect(connection_->getBev(), (struct sockaddr*)&tSockAddr, sizeof(tSockAddr)) < 0) {
        connection_->close();
        return false;
    }

    connect_ = true;
    newConnection();

    timer_.reset(new Timer(base_, checkInterval_, [this, tSockAddr]() {
        if(!connect_) {
            log_warn("tcpclient connection failed, and begin to retry");
            connection_.reset(new TcpConnection(base_, -1, name_));
            if( bufferevent_socket_connect(connection_->getBev(), (struct sockaddr*)&tSockAddr, sizeof(tSockAddr)) < 0) {
                connection_->close();
                return;
            } else {
                connect_ = true;
                newConnection();
            }
        } else {
            if(time(NULL) - connection_->getActiveTime() > invaildInterval_) {
                connection_->close();

                log_warn("tcpclient connection timeout, and begin to retry");
                connection_.reset(new TcpConnection(base_, -1, name_));
                if( bufferevent_socket_connect(connection_->getBev(), (struct sockaddr*)&tSockAddr, sizeof(tSockAddr)) < 0) {
                    connection_->close();
                    return;
                } else {
                    connect_ = true;
                    newConnection();
                }
            }
        }
    }));

    return true;
}

int TcpClient::send(const unsigned char *buffer, int size) {
    if(!connect_) {
        log_warn("tcp %s client connection has closed", name_.c_str());
        return 0;
    }
    return connection_->send(buffer, size);
}

void TcpClient::close() {
    connection_->close();
}

TcpConnPtr TcpClient::getConn() {
    return connection_;
}

void TcpClient::setHeartBeat(bool isSendHeartBeat, int sendSeonds, int invaildSeconds) {
    sendHeartBeat_ = isSendHeartBeat;
    heartBeatInterval_ = sendSeonds;
    invaildInterval_ = invaildSeconds;
}

void TcpClient::newConnection() {
    connection_->setConnectionCallback(connectionCallback_);
    connection_->setMessageCallback(messageCallback_);
    connection_->setCloseCallback([this](const TcpConnPtr& conn) {
        connect_ = false;
        conn->setState(TcpConnection::kDisconnected);
        if(closeCallback_) {
            closeCallback_(conn);
        }
    });
    connection_->setHeartBeatOpt(sendHeartBeat_, heartBeatInterval_);
    connection_->setHBCallback([this](const TcpConnPtr& conn) {
        if(HBCallback_) {
            HBCallback_(conn);
        }
    });
}
