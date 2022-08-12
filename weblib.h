#ifndef __WEBLIB_H_
#define __WEBLIB_H_

#include<sys/socket.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<openssl/ssl.h>
#include<openssl/err.h>
using namespace std;

ssize_t send(int __fd, string __buf) {
    return send(__fd, const_cast<char*>(__buf.c_str()), __buf.size(), 0);
}

ssize_t ssl_send(SSL* __fd, string __buf) {
    return SSL_write(__fd, const_cast<char*>(__buf.c_str()), __buf.size()); 
}

string recv(int __fd) {
    const int length = 1024 * 1024;
    char __buf[length] = "";
    memset(__buf, '\0', sizeof __buf);
    int s = recv(__fd, __buf, sizeof __buf, 0);
    if (s == -1) {
        cout << "Failed to recieve data!" << endl;
        exit(3);
    } return __buf;
}

string ssl_recv(SSL* __fd) {
    const int length = 1024 * 1024;
    char __buf[length] = "";
    memset(__buf, '\0', sizeof __buf);
    int s = SSL_read(__fd, __buf, sizeof __buf);
    if (s < 0) {
        cout << "Failed to recieve data!" << endl;
        exit(3);
    } return __buf;
}

string readFile(string path) {
    ifstream t;
    t.open(path.c_str());
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

#endif