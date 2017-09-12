#include "util.h"

#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <math.h>

#include "logging.h"

namespace util {

std::string get_local_ip(int fd) {
    struct sockaddr addr;
    struct sockaddr_in* addr_v4;
    socklen_t addr_len = sizeof(addr);

    memset(&addr, 0, sizeof(addr));
    if (0 == getsockname(fd, &addr, &addr_len)) {
        if (addr.sa_family == AF_INET) {
            addr_v4 = (sockaddr_in*) &addr;
            return std::string(inet_ntoa(addr_v4->sin_addr));
        }
    }
    return "0.0.0.0";
}

uint16_t get_local_port(int fd) {
    struct sockaddr addr;
    struct sockaddr_in* addr_v4;
    socklen_t addr_len = sizeof(addr);
    if (0 == getsockname(fd, &addr, &addr_len)) {
        if (addr.sa_family == AF_INET) {
            addr_v4 = (sockaddr_in*) &addr;
            return ntohs(addr_v4->sin_port);
        }
    }
    return 0;
}

std::string get_peer_ip(int fd) {
    struct sockaddr addr;
    struct sockaddr_in* addr_v4;
    socklen_t addr_len = sizeof(addr);
    if (0 == getpeername(fd, &addr, &addr_len)) {
        if (addr.sa_family == AF_INET) {
            addr_v4 = (sockaddr_in*) &addr;
            return std::string(inet_ntoa(addr_v4->sin_addr));
        }
    }
    return "0.0.0.0";
}

uint16_t get_peer_port(int fd) {
    struct sockaddr addr;
    struct sockaddr_in* addr_v4;
    socklen_t addr_len = sizeof(addr);
    if (0 == getpeername(fd, &addr, &addr_len)) {
        if (addr.sa_family == AF_INET) {
            addr_v4 = (sockaddr_in*) &addr;
            return ntohs(addr_v4->sin_port);
        }
    }
    return 0;
}

long getCurrentTime() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

unsigned int ipToInt(const char *ipstr) {
    return ntohl(inet_addr(ipstr));
}

void int2ip( int ip_num, char *ip ) {
    struct in_addr in = {htonl(ip_num)};
    strcpy( ip, (char*)inet_ntoa(in));
}

std::string timetodate(const time_t time) {
    struct tm *l=localtime(&time);
    char buf[128];
    snprintf(buf,sizeof(buf),"%04d-%02d-%02d %02d:%02d:%02d",l->tm_year+1900,l->tm_mon+1,l->tm_mday,l->tm_hour,l->tm_min,l->tm_sec);
    std::string s(buf);
    return s;
}

int split(const std::string& str, std::vector<std::string>& ret_, std::string sep) {
    if (str.empty()) {
        return 0;
    }

    std::string tmp;
    std::string::size_type pos_begin = str.find_first_not_of(sep);
    std::string::size_type comma_pos = 0;

    while (pos_begin != std::string::npos) {
        comma_pos = str.find(sep, pos_begin);
        if (comma_pos != std::string::npos) {
            tmp = str.substr(pos_begin, comma_pos - pos_begin);
            pos_begin = comma_pos + sep.length();
        } else {
            tmp = str.substr(pos_begin);
            pos_begin = comma_pos;
        }

        if (!tmp.empty()) {
            ret_.push_back(tmp);
            tmp.clear();
        }
    }
    return 0;
}

std::map<std::string, std::string> getParamsMap(std::string queryString) {
    std::map<std::string, std::string> kvs;
    if(!queryString.empty()) {
        std::vector<std::string> params;
        split(queryString, params, "&");
        for(auto param : params) {
            std::vector<std::string> kv;
            split(param, kv, "=");
            if(kv.size() == 2) {
                kvs[kv[0]] = kv[1];
            }
        }
    }
    return kvs;
}

bool is_safe(uint8_t b) {
    return b >= ' ' && b < 128;
}

std::string hexdump(const void *buf, size_t len) {
    std::string ret("\r\n");
    char tmp[8];
    const uint8_t *data = (const uint8_t *) buf;
    for (size_t i = 0; i < len; i += 16) {
        for (int j = 0; j < 16; ++j) {
            if (i + j < len) {
                int sz = sprintf(tmp, "%.2x ", data[i + j]);
                ret.append(tmp, sz);
            } else {
                int sz = sprintf(tmp, "   ");
                ret.append(tmp, sz);
            }
        }
        for (int j = 0; j < 16; ++j) {
            if (i + j < len) {
                ret += (is_safe(data[i + j]) ? data[i + j] : '.');
            } else {
                ret += (' ');
            }
        }
        ret += ('\n');
    }
    return ret;
}

std::vector<std::string> split(const std::string &str, std::string sep)
{
    std::vector<std::string> ret;
    if (str.empty()) {
        return ret;
    }

    std::string tmp;
    std::string::size_type pos_begin = str.find_first_not_of(sep);
    std::string::size_type comma_pos = 0;

    while (pos_begin != std::string::npos) {
        comma_pos = str.find(sep, pos_begin);
        if (comma_pos != std::string::npos) {
            tmp = str.substr(pos_begin, comma_pos - pos_begin);
            pos_begin = comma_pos + sep.length();
        } else {
            tmp = str.substr(pos_begin);
            pos_begin = comma_pos;
        }

        if (!tmp.empty()) {
            ret.push_back(tmp);
            tmp.clear();
        }
    }
    return ret;
}

}



