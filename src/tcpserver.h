#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <set>
#include <functional>
#include <memory>
#include <unordered_map>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>

#include "libevent_headers.h"
#include "tcpconnection.h"

typedef std::function<void()> Functor;

class PipeEventWatcher;

class TcpServer {
  public:
    TcpServer(struct event_base *base);
    ~TcpServer();
    int listen(const char* ip, int port);
    void stop();
    void reStart();
    bool isStoped();

    void runInLoop(const Functor &functor);

    void setConnectionCallback(const ConnectionCallBack& cb) {
        connection_cb_ = cb;
    }

    void setMessageCallback(MessageCallBack cb) {
        message_cb_ = cb;
    }

    void setHeartBeat(bool isSendHeartBeat, int interval);

    void setHBCallback(HeartBeatCallBack cb) {
        HBCallback_ = cb;
    }

  private:
    static void error_cb(struct evconnlistener *listener, void *ctx);
    static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
                            struct sockaddr *sa, int socklen, void *ud);

    void initWatcher();
    void doPendingFunctors();

    bool IsInLoopThread() const {
        return tid_ == std::this_thread::get_id();
    }
    void queueInLoop(const Functor &cb);

    void addConnection(std::string& name, const TcpConnPtr& conn);
    void RemoveConnection(const TcpConnPtr& conn);

    void newConnection(evutil_socket_t fd);

    struct event_base *base_;
    struct evconnlistener *listener_;

    uint64_t next_conn_id_;
    std::unordered_map<std::string, TcpConnPtr>      sessionMap_;

    ConnectionCallBack connection_cb_;
    MessageCallBack message_cb_;
    HeartBeatCallBack HBCallback_;

    std::thread::id tid_;
    std::mutex mutex_;
    std::shared_ptr<PipeEventWatcher> watcher_;
    std::atomic<bool> notified_;
    std::vector<Functor> pending_functors_;
    std::atomic<int> pending_functor_count_;

    bool isStoped_;

    bool sendHeartBeat_;
    int heartBeatInterval_;
};

#endif  // TCPSERVER_H
