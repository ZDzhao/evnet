#include "tcpserver.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <future>

#include "logging.h"
#include "util.h"
#include "eventwatcher.h"

TcpServer::TcpServer(struct event_base *base)
    :base_(base),
     tid_(std::this_thread::get_id()),
     notified_(false),
     pending_functor_count_(0),
     isStoped_(false),
     sendHeartBeat_(false),
     heartBeatInterval_(0) {
    initWatcher();
}

TcpServer::~TcpServer() {
    stop();
    sessionMap_.clear();
}

int TcpServer::listen(const char *ip, int port) {
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(ip);
    sin.sin_port = htons(port);
    listener_ = evconnlistener_new_bind(base_, listener_cb, static_cast<void *>(this),
                                        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
                                        (struct sockaddr *)&sin, sizeof(struct sockaddr_in));

    if (listener_) {
        evconnlistener_set_error_cb(listener_, error_cb);
        log_info("tcp server listen %s:%d", ip, port);
        return 0;
    } else {
        log_err("tcp server listen %s:%d error", ip, port);
        return -1;
    }
}

void TcpServer::stop() {
    for(auto& connPtr : sessionMap_) {
        connPtr.second->close();
    }
    evconnlistener_disable(listener_);
    isStoped_ = true;
}

void TcpServer::reStart() {
    isStoped_ = false;
    evconnlistener_enable(listener_);
}

bool TcpServer::isStoped() {
    return isStoped_;
}

void TcpServer::listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
                            struct sockaddr *sa, int socklen, void *ud) {
    TcpServer *self = static_cast<TcpServer *>(ud);

    self->newConnection(fd);
}

void TcpServer::initWatcher() {
    watcher_.reset(new PipeEventWatcher(base_, std::bind(&TcpServer::doPendingFunctors, this)));
    int rc = watcher_->Init();
    assert(rc);
    rc = rc && watcher_->AsyncWait();
    assert(rc);
    if (!rc) {
        log_err("PipeEventWatcher init failed");
    }
}

void TcpServer::doPendingFunctors() {
    std::vector<Functor> functors;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        notified_.store(false);
        pending_functors_.swap(functors);
    }

    for (size_t i = 0; i < functors.size(); ++i) {
        functors[i]();
        --pending_functor_count_;
    }
}

void TcpServer::runInLoop(const Functor& functor) {
    if (IsInLoopThread()) {
        functor();
    } else {
        queueInLoop(functor);
    }
}

void TcpServer::setHeartBeat(bool isSendHeartBeat, int interval) {
    sendHeartBeat_ = isSendHeartBeat;
    heartBeatInterval_ = interval;
}

void TcpServer::queueInLoop(const Functor& cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_functors_.emplace_back(cb);
    }
    ++pending_functor_count_;
    if (!notified_.load()) {
        watcher_->Notify();
        notified_.store(true);
    }
}

void TcpServer::addConnection(std::string &name, const TcpConnPtr &conn) {
    sessionMap_.emplace(name, conn);
}

void TcpServer::RemoveConnection(const TcpConnPtr &conn) {
    auto f = [ = ]() {
        this->sessionMap_.erase(conn->getName());
    };
    runInLoop(f);
}

void TcpServer::newConnection(evutil_socket_t fd) {
    std::string conn_name = "server" + util::get_peer_ip(fd) + "#" + std::to_string(next_conn_id_++);
    TcpConnPtr conn(new TcpConnection(base_, fd, conn_name));

    conn->setMessageCallback(message_cb_);
    conn->setConnectionCallback(connection_cb_);
    conn->setCloseCallback(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));
    conn->setHeartBeatOpt(sendHeartBeat_, heartBeatInterval_);
    conn->setHBCallback([this](const TcpConnPtr& conn) {
        if(HBCallback_) {
            HBCallback_(conn);
        }
    });

    conn->onConnectionEstablished();
    addConnection(conn_name, conn);
}

void TcpServer::error_cb(struct evconnlistener *listener, void *ctx) {
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    log_err("Got an error %d (%s) on the listener, Shutting down.", err, evutil_socket_error_to_string(err));
    event_base_loopexit(base, NULL);
}


