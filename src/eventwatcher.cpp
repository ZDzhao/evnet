#include "eventwatcher.h"

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "libevent_headers.h"
#include "logging.h"

EventWatcher::EventWatcher(struct event_base* evbase, const Handler& handler)
    : evbase_(evbase), attached_(false), handler_(handler) {
    event_ = new event;
    memset(event_, 0, sizeof(struct event));
}

EventWatcher::~EventWatcher() {
    FreeEvent();
    Close();
}

bool EventWatcher::Init() {
    if (!DoInit()) {
        goto failed;
    }

    ::event_base_set(evbase_, event_);
    return true;

failed:
    Close();
    return false;
}


void EventWatcher::Close() {
    DoClose();
}

bool EventWatcher::Watch() {
    if (attached_) {
        if (event_del(event_) != 0) {
            log_err("event_del failed. fd=%d, event_=%p", event_->ev_fd, event_);
        }
        attached_ = false;
    }

    assert(!attached_);
    if (event_add(event_, NULL) != 0) {
        log_err("event_add failed. fd=%d, event_=%p", event_->ev_fd, event_);
        return false;
    }
    attached_ = true;
    return true;
}

void EventWatcher::FreeEvent() {
    if (event_) {
        if (attached_) {
            event_del(event_);
        }

        delete (event_);
        event_ = nullptr;
    }
}

void EventWatcher::Cancel() {
    assert(event_);
    FreeEvent();

    if (cancel_callback_) {
        cancel_callback_();
        cancel_callback_ = Handler();
    }
}

void EventWatcher::SetCancelCallback(const Handler& cb) {
    cancel_callback_ = cb;
}

//////////////////////////////////////////////////////////////////////////

PipeEventWatcher::PipeEventWatcher(struct event_base* event_base,
                                   const Handler& handler)
    : EventWatcher(event_base, handler) {
    memset(pipe_, 0, sizeof(pipe_[0] * 2));
}

bool PipeEventWatcher::DoInit() {
    assert(pipe_[0] == 0);

    if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, pipe_) < 0) {
        log_err("create socketpair ERROR errno:%d, errstr:%s", errno, strerror(errno));
        goto failed;
    }

    if (evutil_make_socket_nonblocking(pipe_[0]) < 0 ||
            evutil_make_socket_nonblocking(pipe_[1]) < 0) {
        goto failed;
    }

    ::event_set(event_, pipe_[1], EV_READ | EV_PERSIST,
                &PipeEventWatcher::HandlerFn, this);
    return true;
failed:
    Close();
    return false;
}

void PipeEventWatcher::DoClose() {
    if (pipe_[0] > 0) {
        EVUTIL_CLOSESOCKET(pipe_[0]);
        EVUTIL_CLOSESOCKET(pipe_[1]);
        memset(pipe_, 0, sizeof(pipe_[0]) * 2);
    }
}

void PipeEventWatcher::HandlerFn(int /*fd*/, short /*which*/, void* v) {
    PipeEventWatcher* e = (PipeEventWatcher*)v;
    char buf[128];
    int n = 0;

    if ((n = ::recv(e->pipe_[1], buf, sizeof(buf), 0)) > 0) {
        e->handler_();
    }
}

bool PipeEventWatcher::AsyncWait() {
    return Watch();
}

void PipeEventWatcher::Notify() {
    char buf[1] = {};

    if (::send(pipe_[0], buf, sizeof(buf), 0) < 0) {
        return;
    }
}
