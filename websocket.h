#ifndef _WEBSOCKET_H_
#define _WEBSOCKET_H_

#include<sys/socket.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<openssl/sha.h>
#include<openssl/ssl.h>
#include<openssl/err.h>
#include<pthread.h>
using namespace std;

const string httpd_version = "1.0.0";
const string magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
typedef map<string, string> argvar;
argvar _e, __default_response;
string _endl = "<br/>";
map<string, argvar> http_mime;

/** 全局参数列表 */
#define HTTP_ENABLE_SSL 1
bool https = false; // 是否开启https
#define HTTP_LISTEN_HOST 2
string http_host = "ALL"; // 运行主机名
#define HTTP_LISTEN_PORT 3
int http_port = 8080; // 监听端口
#define HTTP_SSL_CACERT 4
string http_cacert = "cert.pem"; // 证书路径
#define HTTP_SSL_PRIVKEY 5
string http_privkey = "privkey.pem"; // 私钥路径
/** 全局参数结束 */

const string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

/**
 * @brief 判断是否为base64字符
 * 
 * @param c 查询字符 
 * @return true 
 * @return false 
 */
static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

/**
 * @brief 编码base64
 * 
 * @param bytes_to_encode 编码数据
 * @param in_len 编码长度
 * @return string 
 */
string base64_encode(const char *bytes_to_encode, unsigned int in_len) {
    string ret;
    int i = 0, j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; (i < 4); i++) ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if (i) {
        for (j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];
        while ((i++ < 3)) ret += '=';
    }
    return ret;
}

/**
 * @brief base64解密
 * 
 * @param encoded_string 
 * @return string 
 */
string base64_decode(const string& encoded_string) {
    size_t in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    string ret;
    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++) char_array_4[i] = base64_chars.find(char_array_4[i]) & 0xff;
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            for (i = 0; (i < 3); i++) ret += char_array_3[i];
            i = 0;
        }
    }
    if (i) {
        for (j = 0; j < i; j++) char_array_4[j] = base64_chars.find(char_array_4[j]) & 0xff;
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }
    return ret;
}

struct http_request {
    string method = "";
    string path = "";
    string protocol = "";
    argvar argv;
    string postdata;
};

struct client_conn {
    int conn;
    int thread_id;
    SSL* ssl;
};

/** WebSocket数据收发相关函数 */
const int http_len = 1 << 12;
/** 二进制转换 */
vector<int> to2(unsigned char x) {
    vector<int> st;
    for (int j = 0; j < 8; j++) st.push_back(x % 2), x /= 2;
    reverse(st.begin(), st.end());
    return st;
}
/** 获取十进制值 */
int getval(vector<int> x, int st, int len) {
    int e = st + len - 1;
    e = min(e, int(x.size() - 1));
    int res = 0;
    for (int i = st; i <= e; i++) res *= 2, res += x[i];
    return res;
}
/** 接收数据接口 */
int ws_recv_data(client_conn __fd, char __buf[http_len], int len) {
    memset(__buf, '\0', http_len);
    int s = -1;
    if (!https) s = recv(__fd.conn, __buf, len, 0);
    else s = SSL_read(__fd.ssl, __buf, len);
    if (s == -1) {
        cout << "Failed to recieve data!" << endl;
        pthread_exit(NULL);
    }
    return s;
}

/**
 * @brief 发送信息
 * 
 * @param __fd 客户端连接符
 * @param __buf 信息主体
 * @return ssize_t 
 */
ssize_t send(int __fd, string __buf) {
    return send(__fd, const_cast<char*>(__buf.c_str()), __buf.size(), 0);
}

/**
 * @brief 通过SSL发送信息
 * 
 * @param __fd 客户端ssl连接符
 * @param __buf 信息主体
 * @return ssize_t 
 */
ssize_t ssl_send(SSL* __fd, string __buf) {
    return SSL_write(__fd, const_cast<char*>(__buf.c_str()), __buf.size()); 
}

/**
 * @brief WebSocket信息加密与发送
 * 
 * @param __fd 客户端连接符
 * @param __buf 信息主体
 * @return ssize_t 
 */
ssize_t ws_send(client_conn __fd, string __buf, bool extra = false) {
    const int MaxL = 131000;
    char dat[MaxL]; int pt = 0;
    memset(dat, '\0', MaxL);

    /** 构造第一帧 */
    int FIN = __buf.size() <= MaxL;
    int RSV1 = 0;
    int RSV2 = 0;
    int RSV3 = 0;
    int opcode = !extra;
    int frame0 = FIN << 7 | RSV1 << 6 | RSV2 << 5 | RSV3 << 4 | opcode;
    dat[pt++] = frame0;

    /** 构造数据长度帧 */
    int MASK = 0;
    int len = min(int(__buf.size()), MaxL);
    if (len <= 125) dat[pt++] = MASK << 7 | len;
    else if (len < (1 << 16)) {
        dat[pt++] = MASK << 7 | 126;
        dat[pt++] = len >> 8;
        dat[pt++] = len % (1 << 8);
    } else {
        dat[pt++] = MASK << 7 | 127;
        int st[8] = {0};
        for (int i = 0; i < 8; i++) 
            st[i] = len % (1 << 8), len >>= 8;
        for (int i = 7; i >= 0; i--) dat[pt++] = st[i];
    }

    /** 构造数据帧 */
    for (int i = 0; i < len; i++) dat[pt++] = __buf[i];
    
    /** 发送数据帧 */
    int s = -1;
    if (!https) s = send(__fd.conn, dat, pt, 0);
    else s = SSL_write(__fd.ssl, dat, pt);
    if (s == -1) {
        cout << "Failed to send data!" << endl;
        pthread_exit(NULL);
    }

    /** 分段发送 */
    if (__buf.size() > MaxL) s += ws_send(__fd, __buf.substr(MaxL), true);

    return s;
}

/**
 * @brief 接收信息
 * 
 * @param __fd 客户端连接符
 * @return string 
 */
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

/**
 * @brief 通过SSL接收信息
 * 
 * @param __fd 客户端连接符
 * @return string 
 */
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

/**
 * @brief WebSocket信息接收与解密
 * 
 * @param conn 客户端连接符
 * @return string 
 */
string ws_recv(client_conn conn) {

    /** 接受数据 */
    char __buf[http_len] = "";
    int s = ws_recv_data(conn, __buf, 2);

    /** 解析头数据 */
    if (s < 2) {
        cout << "Invalid WebSocket Data Frame!" << endl;
        pthread_exit(NULL);
    }
    vector<int> frame0 = to2(__buf[0]);
    int FIN = frame0[0];
    int RSV1 = frame0[1];
    int RSV2 = frame0[2];
    int RSV3 = frame0[3];
    int opcode = getval(frame0, 4, 4);

    /** 解析数据长度 */
    vector<int> frame1 = to2(__buf[1]);
    int MASK = frame1[0];
    int type = 0;
    long long len = getval(frame1, 1, 7);

    /** 数据长度需要用2byte存储的情况 */
    if (len == 126) {
        type = 1;
        s = ws_recv_data(conn, __buf, 2);
        if (s < 2) {
            cout << "Invalid WebSocket Data Frame!" << endl;
            pthread_exit(NULL);
        }
        len = (getval(to2(__buf[0]), 0, 8) << 8) + getval(to2(__buf[1]), 0, 8);
    }

    /** 数据长度需要用8byte存储的情况 */
    if (len == 127) {
        type = 2;
        s = ws_recv_data(conn, __buf, 8);
        if (s < 8) {
            cout << "Invalid WebSocket Data Frame!" << endl;
            pthread_exit(NULL);
        }
        len = 0;
        for (int i = 0; i < 8; i++) {
            len <<= 8;
            len += getval(to2(__buf[i]), 0, 8);
        }
    }

    /** 获取maskkey */
    int maskkey[4] = {0};
    s = ws_recv_data(conn, __buf, 4);
    if (s < 4) {
        cout << "Invalid WebSocket Data Frame!" << endl;
        pthread_exit(NULL);
    }
    for (int i = 0; i < 4; i++) maskkey[i] = getval(to2(__buf[i]), 0, 8);

    /** 解析文本 */
    int x = 0;
    string res = "";
    const long long MaxL = http_len;
    s = ws_recv_data(conn, __buf, min(len, MaxL));
    int pt = 0;
    for (long long i = 0; i < len; i++) {
        /** 由于TCP缓冲区的原因，导致数据传输不完整，需要多次调用recv读取 */
        if (pt >= s) {
            s = ws_recv_data(conn, __buf, min(len - i, MaxL));
            pt = 0;
        }

        /** 解析i位置上的字符 */
        int data = getval(to2(__buf[pt]), 0, 8);
        data = ((~maskkey[i % 4]) & data) | (maskkey[i % 4] & (~data));
        res += char(data);
        pt++;
    }

    /** FIN值为0时，后续还有数据帧，需要继续获取 */
    if (FIN == 0) res += ws_recv(conn);

    return res;
}

void putRequest(client_conn&, int, argvar);

int sock;
struct sockaddr_in server_address;
string http_code[1010];
SSL_CTX *ctx;

void http_init() {

    /** 设置HTTP代码解释 */
    http_code[100] = "Continue";
    http_code[101] = "Switching Protocols";

    http_code[200] = "OK";
    http_code[201] = "Created";
    http_code[202] = "Accepted";
    http_code[203] = "Non-Authoritative Information";
    http_code[204] = "No Content";
    http_code[205] = "Reset Content";
    http_code[206] = "Partial Content";

    http_code[300] = "Multiple Choices";
    http_code[301] = "Moved Permanently";
    http_code[302] = "Found";
    http_code[303] = "See Other";
    http_code[304] = "Not Modified";
    http_code[305] = "Use Proxy";
    http_code[306] = "Unused";
    http_code[307] = "Temporary Redirect";

    http_code[400] = "Bad Request";
    http_code[401] = "Unauthorized";
    http_code[402] = "Payment Required";
    http_code[403] = "Forbidden";
    http_code[404] = "Not Found";
    http_code[405] = "Method Not Allowed";
    http_code[406] = "Not Acceptable";
    http_code[407] = "Proxy Authentication Required";
    http_code[408] = "Request Time-out";
    http_code[409] = "Conflict";
    http_code[410] = "Gone";
    http_code[411] = "Length Required";
    http_code[412] = "Precondition Failed";
    http_code[413] = "Request Entity Too Large";
    http_code[414] = "Request-URI Too Large";
    http_code[415] = "Unsupported Media Type";
    http_code[416] = "Requested range not satisfiable";
    http_code[417] = "Expectation Failed";

    http_code[500] = "Internal Server Error";
    http_code[501] = "Not Implemented";
    http_code[502] = "Bad Gateway";
    http_code[503] = "Service Unavailable";
    http_code[504] = "Gateway Time-out";
    http_code[505] = "HTTP Version not supported";
    
    /** 设置默认响应头 */
    __default_response["Server"] = "Websocket Server Version " + httpd_version;
    __default_response["Access-Control-Allow-Origin"] = "*";
    __default_response["Connection"] = "upgrade";
    __default_response["Upgrade"] = "websocket";

    if (https) {
        /** SSL 库初始化 */
        SSL_library_init();
        /** 载入所有 SSL 算法 */
        OpenSSL_add_all_algorithms();
        /** 载入所有 SSL 错误消息 */
        SSL_load_error_strings();
        /** 初始化SSL_CTX */
        ctx = SSL_CTX_new(SSLv23_server_method());
        if (ctx == NULL) {
            cout << "Failed to create new ctx!" << endl;
            exit(3);
        }
        /** 载入用户的数字证书 */
        if (SSL_CTX_use_certificate_file(ctx, http_cacert.c_str(), SSL_FILETYPE_PEM) <= 0) {
            cout << "Failed to load cacert!" << endl;
            exit(3);
        }
        /** 载入用户私钥 */
        if (SSL_CTX_use_PrivateKey_file(ctx, http_privkey.c_str(), SSL_FILETYPE_PEM) <= 0) {
            cout << "Failed to load privkey!" << endl;
            exit(3);
        }
        /** 检查用户私钥是否正确 */
        if (!SSL_CTX_check_private_key(ctx)) {
            cout << "Incorrect privkey!" << endl;
            exit(3);
        }    
    }

    /** 初始化服务端socket */
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = http_host == "ALL" ? htons(INADDR_ANY) : inet_addr(http_host.c_str());
    server_address.sin_port = htons(http_port);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cout << "Failed to initialize socket!" << endl;
        exit(3);
    }

    /** 绑定服务端socket */
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    int send_size = 1024 * 1024;
    int optlen = sizeof(send_size); 
    assert(setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &send_size, optlen) != -1); 
    int recv_size = 1024 * 1024;
    optlen = sizeof(recv_size); 
    assert(setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &recv_size, optlen) != -1); 
    int ret = bind(sock, (struct sockaddr*)&server_address, sizeof(server_address));
    if (ret == -1) {
        cout << "Failed to bind socket!" << endl;
        exit(3);
    }

    /** 设置服务端监听态 */
    ret = listen(sock,1);
    if (ret == -1) {
        cout << "Failed to listen to client!" << endl;
        exit(3);
    }
}

struct sockaddr_in client;
socklen_t client_addrlength = sizeof(client);

/**
 * @brief 接收客户端socket
 * 
 * @return int 
 */
int accept() {
    int ret = accept(sock, (struct sockaddr*)&client, &client_addrlength);
    if (ret < 0) {
        cout << "Failed to accept request!" << endl;
        // pthread_exit(NULL);
        exit(0);
    }
    return ret;
}

/**
 * @brief 拆散字符串
 * 
 * @param seperator 分隔符
 * @param source 源字符串
 * @return vector<string> 
 */
vector<string> explode(const char* seperator, const char* source) {
	string src = source; vector<string> res;
	while (src.find(seperator) != string::npos) {
		int wh = src.find(seperator);
		res.push_back(src.substr(0, src.find(seperator)));
		src = src.substr(wh + string(seperator).size());
	} res.push_back(src);
	return res;
}

/**
 * @brief 结束请求
 * 
 * @param conn 客户端连接符
 */
void exitRequest(client_conn& conn) {
    close(conn.conn);
    // longjmp(buf[conn.thread_id], 0);
    pthread_exit(NULL);
}

/**
 * @brief 获取HTTP请求头
 * 
 * @param conn 客户端连接符
 * @return http_request 
 */
http_request getRequest(client_conn& conn) {

    /** 获取请求头 */
    string s;
    if (https) s = ssl_recv(conn.ssl);
    else s = recv(conn.conn);
    if (s == "") exitRequest(conn);

    /** 判断请求方式 */
    vector<string> __arg = explode("\r\n", s.c_str());
    if (__arg.size() < 1) {
        cout << "Invalid HTTP request!" << endl;
        cout << "Error Code: 0x01" << endl;
        putRequest(conn, 500, __default_response);
        exitRequest(conn);
    } 

    /** 读取请求头的第一行 */
    vector<string> header = explode(" ", __arg[0].c_str());
    if (header.size() < 3 || (
        header[0] != "GET" && header[0] != "HEAD" && header[0] != "POST" &&
        header[0] != "PUT" && header[0] != "DELETE" && header[0] != "CONNECT" && 
        header[0] != "OPTIONS" && header[0] != "TRACE" && header[0] != "PATCH"
    )) {
        cout << "Invalid HTTP request!" << endl;
        putRequest(conn, 500, __default_response);
        exitRequest(conn);
    }
    http_request request;
    request.method = header[0];
    request.path = header[1];
    request.protocol = header[2];

    /** 读取请求头参数 */
    int pt = 1;
    for (; pt < __arg.size(); pt++) {
        if (__arg[pt].find(": ") == string::npos) break;
        int __wh = __arg[pt].find(": ");
        string key = __arg[pt].substr(0, __wh);
        string value = __arg[pt].substr(__wh + 2);
        request.argv.insert(make_pair(key, value));
    }
    pt++;
    if (pt < __arg.size()) 
        request.postdata = __arg[pt];

    /** 返回请求头信息 */
    return request;
}

/**
 * @brief 放置响应头
 * 
 * @param conn 客户端连接符
 * @param code 响应代码
 * @param argv 响应参数
 * @param content 输出内容
 */
void putRequest(client_conn& conn, int code, argvar argv) {

    /** 判断响应代码 */
    if (code <= 0 || code >= 1000 || http_code[code] == "") {
        cout << "Invalid Response Code!" << endl;
        putRequest(conn, 500, __default_response);
        exitRequest(conn);
    }

    /** 构造响应头 */
    stringstream __buf;
    __buf << "HTTP/1.1 " << code << " " << http_code[code] << "\r\n";
    for (auto it = argv.begin(); it != argv.end(); it++)
        __buf << (*it).first << ": " << (*it).second << "\r\n";
    __buf << "\r\n";

    /** 发送响应头 */
    if (https) ssl_send(conn.ssl, __buf.str());
    else send(conn.conn, __buf.str());
}

/**
 * @brief 获取GET参数
 * 
 * @param request 请求头信息
 * @return argvar
 */
argvar getParam(http_request request) {

    /** 读取路径信息 */
    string path = request.path;
    if (path.find("?") == string::npos) return _e;

    /** 提取参数信息 */
    string param = path.substr(path.find("?") + 1);
    vector<string> __arg = explode("&", param.c_str());

    /** 逐个处理 */
    argvar $_GET;
    for (int i = 0; i < __arg.size(); i++) {
        if (__arg[i].find("=") == string::npos) 
            $_GET.insert(make_pair(__arg[i], ""));
        else {
            string key = __arg[i].substr(0, __arg[i].find("="));
            string val = __arg[i].substr(__arg[i].find("=") + 1);
            $_GET.insert(make_pair(key, val));
        }
    } 
    
    /** 返回 */
    return $_GET;
}

/**
 * @brief 获取POST参数
 * 
 * @param request 请求头信息
 * @return argvar 
 */
argvar postParam(http_request request) {

    /** 提取参数信息 */
    vector<string> __arg = explode("&", request.postdata.c_str());

    /** 逐个处理 */
    argvar $_POST;
    for (int i = 0; i < __arg.size(); i++) {
        if (__arg[i] == "") continue;
        if (__arg[i].find("=") == string::npos) 
            $_POST.insert(make_pair(__arg[i], ""));
        else {
            string key = __arg[i].substr(0, __arg[i].find("="));
            string val = __arg[i].substr(__arg[i].find("=") + 1);
            $_POST.insert(make_pair(key, val));
        }
    } 
    
    /** 返回 */
    return $_POST;
}

/**
 * @brief 获取COOKIE参数
 * 
 * @param request 请求头信息
 * @return argvar 
 */
argvar cookieParam(http_request request) {
    
    /**  获取Cookie字符串 */
    if (request.argv.find("Cookie") == request.argv.end()) return _e;
    string s = request.argv["Cookie"];

    /** 拆散字符串 */
    vector<string> arr = explode("; ", s.c_str());
    argvar $_COOKIE;
    for (int i = 0; i < arr.size(); i++) {
        if (arr[i].find("=") != string::npos) {
            string key = arr[i].substr(0, arr[i].find("="));
            string val = arr[i].substr(arr[i].find("=") + 1);
            $_COOKIE.insert(make_pair(key, val));
        }
    }

    /** 返回 */
    return $_COOKIE;
}

/**
 * @brief 合并两个数组
 * 
 * @param a 
 * @param b 
 * @return argvar 
 */
argvar merge(argvar a, argvar b) {
    for (auto it : b) a[it.first] = it.second;
    return a;
}

typedef vector<string> param;

void* work_thread(void*);

class application {
    public: 
        struct r {
            string path;
            function<void(client_conn, http_request, param)> main;
            r(){}
            r(string path, function<void(client_conn, http_request, param)> main):path(path),main(main){}
        }; 
        vector<r> route;

        /**
         * @brief 判断是否为整数
         * 
         * @param x 传入参数
         * @return true 
         * @return false 
         */
        bool isInt(string x) {
            if (x.size() == 0) return false;
            int st = 0; 
            if (x[0] == '-') st++;
            for (int i = st; i < x.size(); i++) 
                if (x[i] < '0' || x[i] > '9') return false;
            return true;
        }

        /**
         * @brief 判断是否为小数
         * 
         * @param x 传入参数
         * @return true 
         * @return false 
         */
        bool isDouble(string x) {
            if (x.size() == 0) return false;
            bool pointed = false; int st = 0;
            if (x[0] == '-') st++;
            for (int i = st; i < x.size(); i++) {
                if (x[i] == '.') {
                    if (!pointed) pointed = true;
                    else return false;
                } else if (x[i] < '0' || x[i] > '9') return false;
            } return true;
        }

        /**
         * @brief 匹配路径
         * 
         * @param __route 路由结构体
         * @param path 匹配串
         * @return true 匹配成功
         * @return false 匹配失败
         */
        bool matchPath(r __route, string path) {

            /** 拆散字符串 */
            vector<string> __goal = explode("/", __route.path.c_str());
            vector<string> __path = explode("/", path.c_str());
            if (__goal.size() != __path.size()) return false;

            /** 逐个判断 */
            for (int i = 0; i < __goal.size(); i++) {
                if (__goal[i] == "%d" || __goal[i] == "%D") {
                    if (!isInt(__path[i])) return false;
                } else if (__goal[i] == "%f" || __goal[i] == "%F") {
                    if (!isDouble(__path[i])) return false;
                } else if (__goal[i] == "%s" || __goal[i] == "%S") {
                    
                } else {
                    if (__goal[i] != __path[i]) return false;
                }
            } return true;
        }

        /**
         * @brief 添加路由
         * 
         * @param path 路由路径
         * @param func 执行函数
         */
        void addRoute(string path, function<void(client_conn, http_request, param)> func) {
            route.push_back(r(path, func));
        }

        /**
         * @brief 程序运行主函数
         * 
         * @param host 主机名
         * @param port 运行端口
         */
        void run() {
            http_init();
            while(1) {
                int conn = accept();
                pthread_t pt;
                pthread_create(&pt, NULL, work_thread, (void*)&conn);
            }
        }

        /**
         * @brief 设置服务端选项
         * 
         * @param _t 选项类型 
         * @param ... 选项值
         * @return true 
         * @return false 
         */
        bool setopt(int _t, ...) {
            va_list arg;
            va_start(arg, _t);
            switch(_t) {
                case HTTP_ENABLE_SSL: https = va_arg(arg, int); break;
                case HTTP_LISTEN_HOST: http_host = va_arg(arg, const char*); break;
                case HTTP_LISTEN_PORT: http_port = va_arg(arg, int); break;
                case HTTP_SSL_CACERT: http_cacert = va_arg(arg, const char*); break;
                case HTTP_SSL_PRIVKEY: http_privkey = va_arg(arg, const char*); break;
                default: return false;
            }
            return true;
        }
}app;

int cnt = 0;
pthread_mutex_t g_mutex_lock;
/**
 * @brief 获取当前线程id
 * 
 * @return int 
 */
int get_thread_id() {
    pthread_mutex_lock(&g_mutex_lock);
    int res = ++cnt;
    pthread_mutex_unlock(&g_mutex_lock);
    return res;
}

/**
 * @brief 线程格式化输出
 * 
 * @param thread_id 线程id
 * @param info 输出信息
 */
void output(int thread_id, string info) {
    pthread_mutex_lock(&g_mutex_lock);
    cout << "[thread #" << thread_id << "] " << info << endl;
    pthread_mutex_unlock(&g_mutex_lock);
}

/**
 * @brief 线程主函数
 * 
 * @param arg 客户端连接符
 * @return void* 
 */
void* work_thread(void* arg) {
    int conn = *(int*)arg;
    int thread_id = get_thread_id();

    SSL* ssl;
    if (https) {
        /** 基于 ctx 产生一个新的 SSL */
        ssl = SSL_new(ctx);
        /** 将连接用户的 socket 加入到 SSL */
        SSL_set_fd(ssl, conn);
        if (SSL_accept(ssl) == -1) pthread_exit(NULL);
    }
    
    /** 获取新连接 */
    client_conn conn2;
    conn2.conn = conn;
    conn2.thread_id = 0;
    conn2.ssl = ssl;
    http_request request = getRequest(conn2);
    stringstream buffer;

    buffer << "New Connection: " << request.method << " " << request.path;
    output(thread_id, buffer.str());

    /** 提取路径 */
    string rlpath = request.path;
    if (rlpath.find("?") != string::npos) 
        rlpath = rlpath.substr(0, rlpath.find("?"));

    /** 分发路由 */
    for (int i = 0; i < app.route.size(); i++) {
        if (app.matchPath(app.route[i], rlpath)) {

            /** 计算Sec_WebSocket_Accept的值 */
            if (request.argv.find("Sec-WebSocket-Key") == request.argv.end()) {
                buffer.str("");
                buffer << "Invalid WebSocket Request!";
                output(thread_id, buffer.str());
                pthread_exit(NULL);
            }
            string req_key = request.argv["Sec-WebSocket-Key"];
            string key = req_key + magic_string;
            SHA_CTX sha_ctx;
            unsigned char result[20] = ""; char enc[20] = "";
            SHA1_Init(&sha_ctx);
            SHA1_Update(&sha_ctx, key.c_str(), key.size());
            SHA1_Final(&(result[0]), &sha_ctx);
            for (int i = 0; i < 20; i++) enc[i] = result[i];
            string sec_key = "";
            sec_key = base64_encode(enc, 20);
            argvar ret = __default_response;
            ret["Sec-WebSocket-Accept"] = sec_key;

            buffer.str("");
            buffer << "Secure WebSocket Accept: " << sec_key;
            output(thread_id, buffer.str());
            putRequest(conn2, 101, ret);
            
            /** 参数提取 */
            param argv;
            string __goal = app.route[i].path;
            string __path = rlpath;
            vector<string> __a1 = explode("/", __goal.c_str());
            vector<string> __a2 = explode("/", __path.c_str());
            for (int j = 0; j < __a1.size(); j++) 
                if (__a1[j] == "%d" || __a1[j] == "%D" ||
                    __a1[j] == "%f" || __a1[j] == "%F" || 
                    __a1[j] == "%s" || __a1[j] == "%S")
                    argv.push_back(__a2[j]);

            /** 主函数执行 */
            app.route[i].main(conn2, request, argv);
            pthread_exit(NULL);
        }
    }

    /** 无效路由 */
    putRequest(conn2, 404, __default_response);
    exitRequest(conn2);

    return (void*)NULL;
}

#endif