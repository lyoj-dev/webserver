#include<bits/stdc++.h>
#include"httpd.h"
using namespace std;

string readFile(string path) {
    ifstream t;
    t.open(path.c_str());
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}

int main(int argc, char *argv[]) {
    app.addRoute("/websocket", [](client_conn conn, http_request request, param argv){
        string js = "<script src='./script/main.js'></script>";
        putRequest(conn, 200, __default_response, js);
    });
    app.addRoute("/style/%s", [](client_conn conn, http_request request, param argv){
        string buffer = readFile("./css/" + argv[0]);
        putRequest(conn, 200, merge(__default_response, mime(".css")), buffer);
    });
    app.addRoute("/script/%s", [](client_conn conn, http_request request, param argv){
        string buffer = readFile("./js/" + argv[0]);
        putRequest(conn, 200, merge(__default_response, mime(".js")), buffer);
    });
    app.addRoute("/index", [](client_conn conn, http_request request, param argv){
        stringstream buffer;
        argvar $_GET = getParam(request);
        argvar $_POST = postParam(request);
        argvar $_COOKIE = cookieParam(request);
        buffer << "$_GET: " << _endl;
        for (auto it : $_GET) buffer << it.first << " " << it.second << _endl;
        buffer << "$_POST: " << _endl;
        for (auto it : $_POST) buffer << it.first << " " << it.second << _endl;
        buffer << "$_COOKIE: " << _endl;
        for (auto it : $_COOKIE) buffer << it.first << " " << it.second << _endl;
        putRequest(conn, 200, __default_response, buffer.str());
    });
    app.addRoute("/%d/%f/%s", [](client_conn conn, http_request request, param argv){
        stringstream buffer;
        buffer << argv[0] << " " << argv[1] << " " << argv[2] << endl;
        putRequest(conn, 200, __default_response, buffer.str());
    });
    app.setopt(HTTP_ENABLE_SSL, false);
    app.setopt(HTTP_LISTEN_PORT, 8888);
    app.setopt(HTTP_MULTI_THREAD, 3);
    app.setopt(HTTP_LISTEN_HOST, "ALL");
    app.run();
    return 0;
}