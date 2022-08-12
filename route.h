#ifndef _ROUTE_H_
#define _ROUTE_H_

#include<pthread.h>
#include"weblib.h"
#include"httpd.h"
using namespace std;
typedef vector<string> param;

class thread_pool {
    private: 
        pthread_t pt[1024 * 1024];
        vector<int> connlist;
        pthread_mutex_t g_mutex_lock;

        int cnt = 0;
        int get_thread_id() {
            pthread_mutex_lock(&g_mutex_lock);
            int res = ++cnt;
            pthread_mutex_unlock(&g_mutex_lock);
            return res;
        }

        void output(int thread_id, string info) {
            pthread_mutex_lock(&g_mutex_lock);
            cout << "[thread #" << thread_id << "] " << info << endl;
            pthread_mutex_unlock(&g_mutex_lock);
        }

        int getConn() {
            pthread_mutex_lock(&g_mutex_lock);
            int conn = connlist.size() ? *connlist.begin() : -1;
            if (conn != -1) connlist.erase(connlist.begin());
            pthread_mutex_unlock(&g_mutex_lock);
            return conn;
        }

        void work_thread();

        static void* pre_thread(void* arg) {
            thread_pool* is = (thread_pool*)arg;
            is->work_thread();
            return (void*)NULL;
        }

    public:
        void init(int thread_num) {
            for (int i = 1; i <= thread_num; i++) {
                pthread_create(&pt[i], NULL, pre_thread, (void*)this);
            }
        }

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

void thread_pool::work_thread() {
    int id = this->get_thread_id();
    this->output(id, "Listening...");
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
                putRequest(conn2, 200, __default_response, "");
                break;
            }
        }

        /** 无效路由 */
        stringstream buffer;
        buffer << "<html>" << endl;
        buffer << "<head><title>404 Not Found</title></head>" << endl;
        buffer << "<body>" << endl;
        buffer << "<center><h1>404 Not Found</h1></center>" << endl;
        buffer << "<hr><center>Made by <a href='https://github.com/LittleYang0531'>@LittleYang0531</a></center>" << endl;
        buffer << "</body>" << endl;
        buffer << "</html>" << endl;
        putRequest(conn2, 404, __default_response, buffer.str());
    }
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         

#endif