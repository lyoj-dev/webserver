# Web Server Core

Compile Command: `g++ main.cpp -o main -lpthread -lcrypto -lssl`

开发历程博客: 

1. [lyoj-dev's blog](https://blog.lyoj.ml/archives/5/)
2. [LittleYang0531's blog](https://blog.littleyang.ml/#/post/35)

## 使用方法:

1. 引入 `httpd.h` 头文件
2. 使用 app.addRoute(path, func) 添加路由
3. 使用 app.setopt(key, value) 设置运行参数
4. 使用 app.run() 运行 webserver

具体参考 `main.cpp` 里的代码

## API 列表

### 另定义类型

**映射参数列表 argvar**

```cpp
typedef map<string, string> argvar;
```

**HTTP请求 http_request**

```cpp
struct http_request {
    string method = ""; // 请求方法
    string path = ""; // 请求路径
    string protocol = ""; // 请求协议
    argvar argv; // 请求头参数
    string postdata; // POST数据
};
```

**客户端连接符 client_conn**

```cpp
struct client_conn {
    int conn; // 客户端连接符
    int thread_id; // 线程id
    SSL* ssl; // SSL客户端连接符
};
```

**一维参数列表 param**

```cpp
typedef vector<string> param;
```

### 一些函数

**路由添加 void app.addRoute()**

参数列表: 

 - 路由路径 `string path`
 - 路由函数 `function<void(client_conn, http_request, param)> func`

备注: 

路由路径可使用 `%d(匹配数字)`, `%f(匹配浮点数)`, `%s(匹配字符串)` 进行匹配

**参数设置 bool app.setopt()**

参数列表: 

 - 设置参数 `int _t`
 - 参数值 `?`

返回值: `bool` 为是否修改成功

可供选择的参数有: 

 - `HTTP_ENABLE_SSL` 是否开启 SSL
 - `HTTP_LISTEN_HOST` 监听主机，`ALL` 为监听所有主机
 - `HTTP_LISTEN_PORT` 监听端口
 - `HTTP_SSL_CACERT` SSL 的 CERT 证书
 - `HTTP_SSL_PRIVKEY` SSL 的 PRIVATE 证书
 - `HTTP_MULTI_THREAD` 线程池大小

...
