#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "httpcontext.h"

class HttpServer {
  public:
    HttpServer(struct event_base *base_);
    ~HttpServer();

    bool listen(int port, const char* ip = "0.0.0.0");

    void registerHandler(const std::string& uri, HTTPRequestCallback callback);
    void registerDefaultHandler(HTTPRequestCallback callback);

    struct event_base *getBase() const {
        return base_;
    }

  private:
    static void genericCallback(struct evhttp_request* req, void* arg);
    void handleRequest(struct evhttp_request* req);
    void defaultHandleRequest(const ContextPtr& ctx);
    void sendReply(struct evhttp_request* req, const std::string& response, int http_code);

  private:
    struct event_base *base_;
    struct evhttp* evhttp_;
    int port_;
    struct evhttp_bound_socket* evhttp_bound_socket_;
    HTTPRequestCallbackMap callbacks_;
    HTTPRequestCallback default_callback_;
};

#endif // HTTPSERVER_H
