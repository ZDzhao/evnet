#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <functional>
#include <memory>

#include "libevent_headers.h"

#include "util.h"

class TcpServer;
class TcpClient;
class TcpConnection;

typedef std::shared_ptr<TcpConnection>                                                   TcpConnPtr;
typedef std::weak_ptr<TcpConnection>                                                     TcpConnWeakPtr;
typedef std::function<void(const TcpConnPtr&)>                                           ConnectionCallBack;
typedef std::function<void(const TcpConnPtr&, struct evbuffer*)>                         MessageCallBack;
typedef std::function<void(const TcpConnPtr&)>                                           WriteCompleteCallBack;
typedef std::function<void(const TcpConnPtr&)>                                           CloseCallBack;
typedef std::function<void(const TcpConnPtr&)>                                           HeartBeatCallBack;


class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
    friend class TcpServer;
    friend class TcpClient;
  public:
    TcpConnection(struct event_base *base, evutil_socket_t fd, const std::string& name);
    ~TcpConnection();

  public:
    enum State { kDisconnected, kConnecting, kConnected, kDisconnecting };
    void close();
    int send(const unsigned char *buffer, int size);

  public:
    std::string getRemoteAddress() const;

    int getfd() {
        return fd_;
    }

    std::string getName() const {
        return name_;
    }

    void setActiveTime(int64_t time) {
        activeTime_ = time;
    }
    int64_t getActiveTime() const {
        return activeTime_;
    }

    bool getSendHeartBeat() const {
        return sendHeartBeat_;
    }

    int getHeartBeatInterval() const {
        return heartBeatInterval_;
    }

    bufferevent *getBev() const {
        return bev_;
    }

    void setState(const State &state);

    void setMessageCallback(MessageCallBack cb) {
        message_cb_ = cb;
    }
    void setConnectionCallback(ConnectionCallBack cb) {
        connection_cb_ = cb;
    }
    void setCloseCallback(CloseCallBack cb) {
        close_cb_ = cb;
    }

    void setHBCallback(HeartBeatCallBack cb) {
        heartBeat_cb_ = cb;
    }

    void setReadWaterMark(int low, int high);
    void setWriteWaterMark(int low, int high);

    void setHeartBeatOpt(bool isSendHeartBeat, int interval);

  private:
    static void read_cb(struct bufferevent *bev, void *ctx);
    //static void write_cb( struct bufferevent * bev, void * ctx );
    static void event_cb(struct bufferevent *bev, short sEvent, void *ctx);

    void onConnectionEstablished();
    void onClose();
    void onMessage(evbuffer *input);
    void onHeartBeat();

    std::string name_;
    evutil_socket_t fd_;
    State state_;

    struct bufferevent *bev_;

    ConnectionCallBack connection_cb_;
    CloseCallBack close_cb_;
    MessageCallBack message_cb_;
    HeartBeatCallBack heartBeat_cb_;

    int64_t activeTime_;
    bool sendHeartBeat_;
    int heartBeatInterval_;
};

#endif // TCPCONNECTION_H
