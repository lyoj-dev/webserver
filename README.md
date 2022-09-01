# Web Server Core & WebSocket Server Core

示例程序编译指令: 

网站服务端: `g++ main.cpp -o main -lpthread -lcrypto -lssl`

WebSocket 服务端: `g++ websocket.cpp -owebsocket -lpthread -lcrypto -lssl`

## Web Server Core

开发历程博客: 

1. [lyoj-dev's blog](https://blog.lyoj.ml/archives/5/)
2. [LittleYang0531's blog](https://blog.littleyang.ml/#/post/35)

### 使用方法:

1. 引入 `httpd.h` 头文件
2. 使用 app.addRoute(path, func) 添加路由
3. 使用 app.setopt(key, value) 设置运行参数
4. 使用 app.run() 运行 webserver

具体参考 `main.cpp` 里的代码

### 样例文件

实现断点续传功能: [file.cpp](https://github.com/lyoj-dev/webserver/blob/main/example/webserver/file.cpp)

## WebSocket Server Core

开发历程博客: 

~~暂无博客~~

### 使用方法:

1. 引入 `websocket.h` 头文件
2. 使用 app.addRoute(path, func) 添加路由
3. 使用 app.setopt(key, value) 设置运行参数
4. 使用 app.run() 运行 webserver

### 样例文件

~~暂无样例~~

## 更新日志: 

### 1.0.2

1. WebServer Core 提供断点续传的样例代码
2. 规范化日志输出格式
3. 新增头文件信息

### 1.0.1

1. 提供对 Windows 系统的支持
2. WebServer Core 允许多次向客户端发送数据

### 1.0.0

1. 第一个版本发布