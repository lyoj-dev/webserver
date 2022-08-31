#ifndef _HTTPD_H_
#define _HTTPD_H_
#define __windows__ (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))

// 指定linux环境下头文件
#ifdef __linux__ 
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<unistd.h>
// 指定Windows环境下头文件 
#elif __windows__
#include<Windows.h>
// 指定其他环境头文件 
#else 
#error("We only support Windows & Linux system! Sorry for that your system wasn't supported!")
#endif
#include<openssl/ssl.h>
#include<openssl/err.h>
#include<pthread.h>
using namespace std;

const string httpd_version = "1.0.1";
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
int http_port = 8888; // 监听端口
#define HTTP_SSL_CACERT 4
string http_cacert = "cert.pem"; // 证书路径
#define HTTP_SSL_PRIVKEY 5
string http_privkey = "privkey.pem"; // 私钥路径
#define HTTP_MULTI_THREAD 6
int http_thread_num = 8; // 运行线程数
/** 全局参数结束 */

struct client_conn {
    #ifdef __linux__
    int conn;
    #elif __windows__
    SOCKET conn;
    #endif
    int thread_id;
    SSL* ssl;
};

void exitRequest(client_conn&);
void putRequest(client_conn&, int, argvar);

/**
 * @brief 发送信息
 * 
 * @param __fd 客户端连接符
 * @param __buf 信息主体
 * @return ssize_t 
 */
ssize_t send(client_conn __fd, string __buf) {
    if (!https) return send(__fd.conn, const_cast<char*>(__buf.c_str()), __buf.size(), 0);
    else return SSL_write(__fd.ssl, const_cast<char*>(__buf.c_str()), __buf.size()); 
}

/**
 * @brief 接收信息
 * 
 * @param __fd 客户端连接符
 * @return string 
 */
string recv(client_conn __fd) {
    const int length = 1024 * 1024;
    char __buf[length] = "";
    memset(__buf, '\0', sizeof __buf);
    int s = -1;
    if (!https) s = recv(__fd.conn, __buf, sizeof __buf, 0);
    else s = SSL_read(__fd.ssl, __buf, sizeof __buf);
    if (s == -1) {
        cout << "Failed to recieve data!" << endl;
        exitRequest(__fd);
    } return __buf;
}

struct http_request {
    string method = "";
    string path = "";
    string protocol = "";
    argvar argv;
    string postdata;
};

int sock;
struct sockaddr_in server_address;
string http_code[1010];
jmp_buf buf[1024 * 1024];

SSL_CTX *ctx;

/**
 * @brief 初始化HTTP服务器
 * 
 */
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
    __default_response["Server"] = "Web Server Version " + httpd_version;
    __default_response["Access-Control-Allow-Origin"] = "*";
    __default_response["Connection"] = "Keep-Alive";
    __default_response["Content-Type"] = "text/html; charset=utf-8";
    
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
    #ifdef __linux__
    bzero(&server_address, sizeof(server_address));
    #elif __windows__
	WORD w_req = MAKEWORD(2, 2);
	WSADATA wsadata; int err;
	err = WSAStartup(w_req, &wsadata);
	if (err != 0) {
        cout << "Failed to initialize SOCKET!" << endl;
        exit(3);
    }
	if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wHighVersion) != 2) {
        cout << "SOCKET version is not correct!" << endl;
        exit(3);
    }
    #endif
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = http_host == "ALL" ? htons(INADDR_ANY) : inet_addr(http_host.c_str());
    server_address.sin_port = htons(http_port);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cout << "Failed to initialize socket!" << endl;
        exit(3);
    }

    /** 绑定服务端socket */
    #ifdef __linux__
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #endif
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
#ifdef __linux__
socklen_t client_addrlength = sizeof(client);
#elif __windows__
int client_addrlength = sizeof(client);
#endif

/**
 * @brief 接收客户端socket
 * 
 * @return int 
 */
int accept() {
    int ret = accept(sock, (struct sockaddr*)&client, &client_addrlength);
    if (ret < 0) {
        cout << "Failed to accept request!" << endl;
        pthread_exit(NULL);
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
    #ifdef __linux__
    close(conn.conn);
    #elif __windows__
    closesocket(conn.conn);
    #endif
    longjmp(buf[conn.thread_id], 0);
}

/**
 * @brief 获取HTTP请求头
 * 
 * @param conn 客户端连接符
 * @return http_request 
 */
http_request getRequest(client_conn& conn) {

    /** 获取请求头 */
    string s = recv(conn);
    if (s == "") exitRequest(conn);

    /** 判断请求方式 */
    vector<string> __arg = explode("\r\n", s.c_str());
    if (__arg.size() < 1) {
        cout << "Invalid HTTP request!" << endl;
        stringstream buffer;
        buffer << "<html>" << endl;
        buffer << "<head><title>500 Internal Server Error</title></head>" << endl;
        buffer << "<body>" << endl;
        buffer << "<center><h1>500 Internal Server Error</h1></center>" << endl;
        buffer << "<hr><center>Made by <a href='https://github.com/lyoj-dev/webserver'>webserver</a> v" << httpd_version << "</center>" << endl;
        buffer << "</body>" << endl;
        buffer << "</html>" << endl;
        putRequest(conn, 500, __default_response);
        send(conn, buffer.str());
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
        stringstream buffer;
        buffer << "<html>" << endl;
        buffer << "<head><title>500 Internal Server Error</title></head>" << endl;
        buffer << "<body>" << endl;
        buffer << "<center><h1>500 Internal Server Error</h1></center>" << endl;
        buffer << "<hr><center>Made by <a href='https://github.com/lyoj-dev/webserver'>webserver</a> v" << httpd_version << "</center>" << endl;
        buffer << "</body>" << endl;
        buffer << "</html>" << endl;
        putRequest(conn, 500, __default_response);
        send(conn, buffer.str());
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
        stringstream buffer;
        buffer << "<html>" << endl;
        buffer << "<head><title>500 Internal Server Error</title></head>" << endl;
        buffer << "<body>" << endl;
        buffer << "<center><h1>500 Internal Server Error</h1></center>" << endl;
        buffer << "<hr><center>Made by <a href='https://github.com/lyoj-dev/webserver'>webserver</a> v" << httpd_version << "</center>" << endl;
        buffer << "</body>" << endl;
        buffer << "</html>" << endl;
        putRequest(conn, 500, __default_response);
        send(conn, buffer.str());
        exitRequest(conn);
    } 

    /** 构造响应头 */
    stringstream __buf;
    __buf << "HTTP/1.1 " << code << " " << http_code[code] << "\r\n";
    for (auto it = argv.begin(); it != argv.end(); it++)
        __buf << (*it).first << ": " << (*it).second << "\r\n";
    __buf << "\r\n";

    /** 发送响应头 */
    send(conn, __buf.str());
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

/**
 * @brief 获取mime类型
 * 
 * @param ext 文件后缀名
 * @return argvar 
 */
argvar mime(string ext) {
    argvar _r = _e;
    if (ext == ".aac") _r["Content-Type"] = "audio/aac";
    else if (ext == ".abw") _r["Content-Type"] = "application/x-abiword";
    else if (ext == ".arc") _r["Content-Type"] = "application/x-freearc";
    else if (ext == ".avi") _r["Content-Type"] = "video/x-msvideo";
    else if (ext == ".azw") _r["Content-Type"] = "application/vnd.amazon.ebook";
    else if (ext == ".bin") _r["Content-Type"] = "application/octet-stream";
    else if (ext == ".bmp") _r["Content-Type"] = "image/bmp";
    else if (ext == ".bz") _r["Content-Type"] = "application/x-bzip";
    else if (ext == ".bz2") _r["Content-Type"] = "application/x-bzip2";
    else if (ext == ".csh") _r["Content-Type"] = "application/x-csh";
    else if (ext == ".css") _r["Content-Type"] = "text/css";
    else if (ext == ".csv") _r["Content-Type"] = "text/csv";
    else if (ext == ".doc") _r["Content-Type"] = "application/msword";
    else if (ext == ".docx") _r["Content-Type"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    else if (ext == ".eot") _r["Content-Type"] = "application/vnd.ms-fontobject";
    else if (ext == ".epub") _r["Content-Type"] = "application/epub+zip";
    else if (ext == ".gif") _r["Content-Type"] = "image/gif";
    else if (ext == ".htm") _r["Content-Type"] = "text/html";
    else if (ext == ".html") _r["Content-Type"] = "text/html";
    else if (ext == ".ico") _r["Content-Type"] = "image/vnd.microsoft.icon";
    else if (ext == ".ics") _r["Content-Type"] = "text/calendar";
    else if (ext == ".jar") _r["Content-Type"] = "application/java-archive";
    else if (ext == ".jpeg") _r["Content-Type"] = "image/jpeg";
    else if (ext == ".jpg") _r["Content-Type"] = "image/jpeg";
    else if (ext == ".js") _r["Content-Type"] = "text/javascript";
    else if (ext == ".json") _r["Content-Type"] = "application/json";
    else if (ext == ".jsonld") _r["Content-Type"] = "application/ld+json";
    else if (ext == ".mid") _r["Content-Type"] = "audio/midi";
    else if (ext == ".midi") _r["Content-Type"] = "audio/midi";
    else if (ext == ".mjs") _r["Content-Type"] = "text/javascript";
    else if (ext == ".mp3") _r["Content-Type"] = "audio/mpeg";
    else if (ext == ".mp4") _r["Content-Type"] = "video/mp4";
    else if (ext == ".mpeg") _r["Content-Type"] = "video/mpeg";
    else if (ext == ".mpkg") _r["Content-Type"] = "application/vnd.apple.installer+xml";
    else if (ext == ".odp") _r["Content-Type"] = "application/vnd.oasis.opendocument.presentation";
    else if (ext == ".ods") _r["Content-Type"] = "application/vnd.oasis.opendocument.spreadsheet";
    else if (ext == ".odt") _r["Content-Type"] = "application/vnd.oasis.opendocument.text";
    else if (ext == ".oga") _r["Content-Type"] = "audio/ogg";
    else if (ext == ".ogv") _r["Content-Type"] = "video/ogg";
    else if (ext == ".ogx") _r["Content-Type"] = "application/ogg";
    else if (ext == ".otf") _r["Content-Type"] = "font/otf";
    else if (ext == ".png") _r["Content-Type"] = "image/png";
    else if (ext == ".pdf") _r["Content-Type"] = "application/pdf";
    else if (ext == ".ppt") _r["Content-Type"] = "application/vnd.ms-powerpoint";
    else if (ext == ".pptx") _r["Content-Type"] = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    else if (ext == ".rar") _r["Content-Type"] = "application/x-rar-compressed";
    else if (ext == ".rtf") _r["Content-Type"] = "application/rtf";
    else if (ext == ".sh") _r["Content-Type"] = "application/x-sh";
    else if (ext == ".svg") _r["Content-Type"] = "image/svg+xml";
    else if (ext == ".swf") _r["Content-Type"] = "application/x-shockwave-flash";
    else if (ext == ".tar") _r["Content-Type"] = "application/x-tar";
    else if (ext == ".tif") _r["Content-Type"] = "image/tiff";
    else if (ext == ".tiff") _r["Content-Type"] = "image/tiff";
    else if (ext == ".ttf") _r["Content-Type"] = "font/ttf";
    else if (ext == ".txt") _r["Content-Type"] = "text/plain";
    else if (ext == ".vsd") _r["Content-Type"] = "application/vnd.visio";
    else if (ext == ".wav") _r["Content-Type"] = "audio/wav";
    else if (ext == ".weba") _r["Content-Type"] = "audio/webm";
    else if (ext == ".webm") _r["Content-Type"] = "video/webm";
    else if (ext == ".webp") _r["Content-Type"] = "image/webp";
    else if (ext == ".woff") _r["Content-Type"] = "font/woff";
    else if (ext == ".woff2") _r["Content-Type"] = "font/woff2";
    else if (ext == ".xhtml") _r["Content-Type"] = "application/xhtml+xml";
    else if (ext == ".xls") _r["Content-Type"] = "application/vnd.ms-excel";
    else if (ext == ".xlsx") _r["Content-Type"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    else if (ext == ".xml") _r["Content-Type"] = "text/xml";
    else if (ext == ".xul") _r["Content-Type"] = "application/vnd.mozilla.xul+xml";
    else if (ext == ".zip") _r["Content-Type"] = "application/zip";
    else if (ext == ".3gp") _r["Content-Type"] = "video/3gpp";
    else if (ext == ".3g2") _r["Content-Type"] = "video/3gpp2";
    else if (ext == ".7z") _r["Content-Type"] = "application/x-7z-compressed";
    else _r["Content-Type"] = "text/plain";
    return _r;
}

typedef vector<string> param;

class thread_pool {
    private: 
        pthread_t pt[1024 * 1024];
        vector<int> connlist;
        pthread_mutex_t g_mutex_lock;

        int cnt = 0;
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
         * @brief 获取客户端连接信息
         * 
         * @return int 
         */
        int getConn() {
            pthread_mutex_lock(&g_mutex_lock);
            int conn = connlist.size() ? *connlist.begin() : -1;
            if (conn != -1) connlist.erase(connlist.begin());
            pthread_mutex_unlock(&g_mutex_lock);
            return conn;
        }

        void work_thread();

        /**
         * @brief 多线程预处理函数
         * 
         * @param arg 线程参数
         * @return void* 
         */
        static void* pre_thread(void* arg) {
            thread_pool* is = (thread_pool*)arg;
            is->work_thread();
            return (void*)NULL;
        }

    public:
        /**
         * @brief 初始化多线程
         * 
         * @param thread_num 线程数
         */
        void init(int thread_num) {
            for (int i = 1; i <= thread_num; i++) {
                pthread_create(&pt[i], NULL, pre_thread, (void*)this);
            }
        }

        /**
         * @brief 插入一个客户端连接
         * 
         * @param conn 客户端连接符
         */
        void addConn(int conn) {
            connlist.push_back(conn);
        }
}pool;

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
            http_init(); pool.init(http_thread_num);
            cout << "Listening..." << endl;
            while(1) {
                int conn = accept();
                pool.addConn(conn);
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
                case HTTP_MULTI_THREAD: http_thread_num = va_arg(arg, int); break; 
                default: return false;
            }
            return true;
        }
}app;

/**
 * @brief 工作线程主函数
 * 
 */
void thread_pool::work_thread() {
    int id = this->get_thread_id();
    // this->output(id, "Listening...");
    while (1) {
        setjmp(buf[id]);
        int conn = this->getConn();
        if (conn == -1) continue;

        SSL* ssl;
        if (https) {
            /** 基于 ctx 产生一个新的 SSL */
            ssl = SSL_new(ctx);
            /** 将连接用户的 socket 加入到 SSL */
            SSL_set_fd(ssl, conn);
            if (SSL_accept(ssl) == -1) continue;
        }
        
        /** 获取新连接 */
        client_conn conn2;
        conn2.conn = conn;
        conn2.thread_id = id;
        conn2.ssl = ssl;
        http_request request = getRequest(conn2);
        this->output(id, "New Connection: " + request.method + " " + request.path);

        /** 提取路径 */
        string rlpath = request.path;
        if (rlpath.find("?") != string::npos) 
            rlpath = rlpath.substr(0, rlpath.find("?"));

        /** 分发路由 */
        for (int i = 0; i < app.route.size(); i++) {
            if (app.matchPath(app.route[i], rlpath)) {
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
                putRequest(conn2, 200, __default_response);
                send(conn2, "");
                exitRequest(conn2);
                break;
            }
        }

        /** 无效路由 */
        stringstream buffer;
        buffer << "<html>" << endl;
        buffer << "<head><title>404 Not Found</title></head>" << endl;
        buffer << "<body>" << endl;
        buffer << "<center><h1>404 Not Found</h1></center>" << endl;
        buffer << "<hr><center>Made by <a href='https://github.com/lyoj-dev/webserver'>webserver</a> v" << httpd_version << "</center>" << endl;
        buffer << "</body>" << endl;
        buffer << "</html>" << endl;
        putRequest(conn2, 404, __default_response);
        send(conn2, buffer.str());
        exitRequest(conn2);
    }
}

#endif
