#include "httpserver.h"

#include <signal.h>

#include "logging.h"


class IgnoreSigPipe {
  public:
    IgnoreSigPipe() {
        ::signal(SIGPIPE, SIG_IGN);
    }
};

IgnoreSigPipe initObj;

HttpServer::HttpServer(event_base *base)
    :base_(base) {
    if (!base_) {
        log_err("event_base null");
        return;
    }

    evhttp_ = evhttp_new(base_);
    evhttp_set_allowed_methods(evhttp_, EVHTTP_REQ_GET|EVHTTP_REQ_POST);
    if (!evhttp_) {
        log_err("create evhttp fail");
        return;
    }
}

HttpServer::~HttpServer() {
    event_base_free(base_);
    if (evhttp_) {
        evhttp_free(evhttp_);
        evhttp_ = nullptr;
    }
    if (base_) {
        event_base_free(base_);
        base_ = nullptr;
    }
}


bool HttpServer::listen(int port, const char* ip) {
    assert(evhttp_);

    port_ = port;
    evhttp_bound_socket_ = evhttp_bind_socket_with_handle(evhttp_, ip, port);
    if (!evhttp_bound_socket_) {
        log_err("evhttp bind %s:%d fail", ip, port_);
        return false;
    }
    log_info("evhttp bind %s:%d", ip, port_);
    evhttp_set_gencb(evhttp_, &HttpServer::genericCallback, this);
    return true;
}

void HttpServer::registerHandler(const std::string &uri, HTTPRequestCallback callback) {
    callbacks_[uri] = callback;
}

void HttpServer::registerDefaultHandler(HTTPRequestCallback callback) {
    default_callback_ = callback;
}

void HttpServer::genericCallback(evhttp_request *req, void *arg) {
    HttpServer* hsrv = static_cast<HttpServer*>(arg);
    hsrv->handleRequest(req);
}

void HttpServer::handleRequest(evhttp_request *req) {
    ContextPtr ctx(new Context(req));

    log_info("body:%s", ctx->body.c_str());
    for(auto& kv : ctx->params) {
        log_info("key:%s, value:%s", kv.first.c_str(), kv.second.c_str());
    }
    log_info("handle request path = %s url = %s", ctx->path.c_str(), req->uri);

    if (callbacks_.empty()) {
        defaultHandleRequest(ctx);
        return;
    }

    auto it = callbacks_.find(ctx->path);
    if (it != callbacks_.end()) {
        ctx->addResponseHeader("Content-Type", "text/plain");
        auto f = std::bind(&HttpServer::sendReply, this, req, std::placeholders::_1, std::placeholders::_2);
        it->second(ctx, f);
        return;
    } else {
        log_info("not find the path, %s", ctx->path.c_str());
        defaultHandleRequest(ctx);
    }
}

void HttpServer::defaultHandleRequest(const ContextPtr &ctx) {
    if (default_callback_) {
        auto f = std::bind(&HttpServer::sendReply, this, ctx->req(), std::placeholders::_1, std::placeholders::_2);
        default_callback_(ctx, f);
    } else {
        evhttp_send_error(ctx->req(), HTTP_BADREQUEST, "Bad Request");
    }
}


struct Response {
    Response(struct evhttp_request* r, const std::string& m)
        : req(r), buffer(nullptr) {
        if (m.size() > 0) {
            buffer = evbuffer_new();
            evbuffer_add(buffer, m.c_str(), m.size());
        }
    }

    ~Response() {
        if (buffer) {
            evbuffer_free(buffer);
            buffer = nullptr;
        }
    }

    struct evhttp_request* req;
    struct evbuffer* buffer;
};

void HttpServer::sendReply(evhttp_request *req, const std::string &response_data, int http_code) {
    log_info("send reply...");

    std::shared_ptr<Response> response(new Response(req, response_data));

    if (!response->buffer) {
        evhttp_send_error(response->req, HTTP_INTERNAL, "internal error");
        return;
    }

    evhttp_send_reply(response->req, http_code, "OK", response->buffer);
}
