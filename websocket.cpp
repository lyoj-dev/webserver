#include<bits/stdc++.h>
#include"websocket.h"
using namespace std;
int main() {
    app.addRoute("/", [](client_conn conn, http_request request, param argv){
        string info = ws_recv(conn);
        ws_send(conn, "Hello Client!");
        exitRequest(conn);
    });
    app.setopt(HTTP_ENABLE_SSL, false);
    app.setopt(HTTP_LISTEN_HOST, "ALL");
    app.setopt(HTTP_LISTEN_PORT, 8080);
    app.run();
}