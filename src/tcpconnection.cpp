#include "tcpconnection.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include "logging.h"

TcpConnection::TcpConnection(struct event_base *base, evutil_socket_t fd, const std::string &name)
    :fd_(fd),
     name_(name),
     state_(kConnecting),
     sendHeartBeat_(false),
     activeTime_(time(NULL)) {
    log_info("get connection name:%s, fd:%d", name_.c_str(), fd);

    if(fd > 0) {
        evutil_make_socket_nonblocking(fd);
    }


    bev_ = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE);
    if(bev_ == NULL) {
        log_err("bufferevent_socket_new failed, err: %s", strerror(errno));
    }
    bufferevent_setcb(bev_, read_cb, NULL, event_cb, static_cast<void *>(this));
    bufferevent_enable(bev_, EV_TIMEOUT | EV_READ | EV_WRITE | EV_PERSIST);

    struct evbuffer *output = bufferevent_get_output(bev_);
    evbuffer_enable_locking(output, NULL);
}

TcpConnection::~TcpConnection() {
    log_info("connection %s delete", name_.c_str());
}

std::string TcpConnection::getRemoteAddress() const {
    return util::get_peer_ip(fd_) + ":" + std::to_string(util::get_peer_port(fd_));
}

void TcpConnection::setReadWaterMark(int low, int high) {
    bufferevent_setwatermark(bev_, EV_READ, low, high);
}

void TcpConnection::setWriteWaterMark(int low, int high) {
    bufferevent_setwatermark(bev_, EV_WRITE, low, high);
}

void TcpConnection::setHeartBeatOpt(bool isSendHeartBeat, int interval) {
    sendHeartBeat_ = isSendHeartBeat;
    heartBeatInterval_ = interval;
    struct timeval tTimeout = {interval, 0};
    bufferevent_set_timeouts( bev_, &tTimeout, NULL);
}

void TcpConnection::close() {
    onClose();
}

int TcpConnection::send(const unsigned char *buffer, int size) {
    if(bev_) {
        bufferevent_lock(bev_);
        struct evbuffer *output = bufferevent_get_output(bev_);
        evbuffer_lock(output);
        evbuffer_add(output, buffer, size);
        evbuffer_unlock(output);
        return size;
    }
    return 0;
}

void TcpConnection::read_cb(struct bufferevent *bev, void *ctx) {
    log_trace("bufferevent read cb");
    TcpConnection *self = static_cast<TcpConnection *>(ctx);
    struct evbuffer *input = bufferevent_get_input(self->bev_);
    size_t n = evbuffer_get_length(input);
    if(n > 0) {
        self->onMessage(input);
    } else {
        self->close();
    }
    self->setActiveTime(time(NULL));
}

void TcpConnection::event_cb(struct bufferevent *bev, short sEvent, void *ctx) {
    TcpConnection *self = static_cast<TcpConnection *>(ctx);
    if( sEvent == BEV_EVENT_CONNECTED ) {
        log_info("%p connection established", bev);
        self->setState(kConnected);
        self->onConnectionEstablished();
        return;
    }

    if (sEvent & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        if (sEvent & BEV_EVENT_ERROR) {
            int err = EVUTIL_SOCKET_ERROR();
            log_warn("connection:%s recv errno: %d, err: %s", self->getName().c_str(), err, evutil_socket_error_to_string(err));
        } else {
            log_warn("connection:%s recv EOF", self->getName().c_str());
        }

        self->setState(kDisconnecting);
        self->onClose();
        return;
    }
    if(sEvent & (BEV_EVENT_TIMEOUT|BEV_EVENT_READING)) {
        if(self->getSendHeartBeat()) {
            self->onHeartBeat();
        }
        bufferevent_enable(bev, EV_TIMEOUT | EV_READ | EV_WRITE | EV_PERSIST);

        struct timeval tTimeout = {self->getHeartBeatInterval(), 0};
        bufferevent_set_timeouts( bev, &tTimeout, NULL);
    }
}

void TcpConnection::onClose() {
    log_warn("%s close connection", name_.c_str());
    bufferevent_free(bev_);
    bev_ = NULL;
    if(close_cb_) {
        close_cb_(shared_from_this());
    }
}

void TcpConnection::onMessage(evbuffer* input) {
    if(message_cb_) {
        message_cb_(shared_from_this(), input);
    }
}

void TcpConnection::onHeartBeat() {
    if(bev_) {
        if(heartBeat_cb_) {
            heartBeat_cb_(shared_from_this());
        }
    }
}

void TcpConnection::setState(const State &state) {
    state_ = state;
}

void TcpConnection::onConnectionEstablished() {
    state_ = kConnected;
    if(connection_cb_) {
        connection_cb_(shared_from_this());
    }
}

