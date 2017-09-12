#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include <string>
#include <vector>
#include <map>


namespace util {

//ip处理函数
std::string get_local_ip(int fd);
std::string get_local_ip();
uint16_t get_local_port(int fd);
std::string get_peer_ip(int fd);
uint16_t get_peer_port(int fd);
unsigned int ipToInt(const char *ipstr);
void int2ip( int ip_num, char *ip );

//时间处理函数
long getCurrentTime();
std::string timetodate(const time_t time);

//http处理函数
int split(const std::string& str, std::vector<std::string>& ret_, std::string sep = ",");
std::map<std::string, std::string> getParamsMap(std::string queryString);

std::vector<std::string> split(const std::string& str, std::string sep = ",");

std::string hexdump(const void *buf, size_t len);
}
#endif // UTIL_H
