#ifndef CONTEXT_H
#define CONTEXT_H

#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <cassert>
#include <string>
#include <map>
#include <functional>
#include <memory>

#include "libevent_headers.h"

#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/http_compat.h>

#include "util.h"

static std::string char2string(const char* src) {
    if(src) {
        return std::string(src);
    } else {
        return std::string("");
    }
}

struct Context {
    Context(struct evhttp_request* r)
        :req_(r) {
        init();
    }

    ~Context() {

    }

    void addResponseHeader(const std::string& key, const std::string& value) {
        evhttp_add_header(req_->output_headers, key.data(), value.data());
    }

    const char* findRequestHeader(const char* key) {
        return evhttp_find_header(req_->input_headers, key);
    }

    const char* original_uri() const {
        return req_->uri;
    }

    struct evhttp_request* req() const {
        return req_;
    }

    evhttp_cmd_type method;

    std::string scheme;
    std::string host;
    int port;
    std::string path;
    std::string query;
    std::string fragment;

    std::string body;
    std::map<std::string, std::string> params;

  private:
    struct evhttp_request* req_;

    void init() {
        method = req_->type;
        const evhttp_uri* uri = evhttp_request_get_evhttp_uri(req_);
        if(uri) {
            scheme   = char2string(evhttp_uri_get_scheme(uri));
            host     = char2string(evhttp_request_get_host(req_));
            port     = evhttp_uri_get_port(uri);
            path     = char2string(evhttp_uri_get_path(uri));
            query    = char2string(evhttp_uri_get_query(uri));
            fragment = char2string(evhttp_uri_get_fragment(uri));
        }
        if(method == EVHTTP_REQ_POST) {
            int content_length = atoi(evhttp_find_header(req_->input_headers, "Content-Length"));
            char buf[content_length+1];
            memset(buf, 0, sizeof buf);
            evbuffer_remove(req_->input_buffer, (void*)buf, content_length);

            //replace "+" to " " for www-form-urlencoded
//              int i = 0;

//              for (i = 0; i < content_length; i++) {
//                if (*(buf + i) == '+') {
//                  *(buf + i) = ' ';
//                }
//              }

            char* decode_body = evhttp_decode_uri(buf);
            if(decode_body) {
                body = std::string(decode_body);
                params = util::getParamsMap(query);
                free(decode_body);
            }
        }
        if(method == EVHTTP_REQ_GET) {
            params = util::getParamsMap(query);
        }
    }

};

typedef std::shared_ptr<Context> ContextPtr;
typedef std::function<void(const std::string& response_data, int response_code)> HTTPSendResponseCallback;
typedef std::function <
void(const ContextPtr& ctx,
     const HTTPSendResponseCallback& respcb) > HTTPRequestCallback;

typedef std::map<std::string, HTTPRequestCallback> HTTPRequestCallbackMap;



#endif // CONTEXT_H
