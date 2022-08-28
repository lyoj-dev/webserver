var ws = new WebSocket("ws://127.0.0.1:8080/"); 
ws.onopen = function(){
    console.log("open");
    var msg = "Hello Server!";
    ws.send(msg);
}
ws.onmessage = function(e){
    console.log(e.data);
}
ws.onclose = function(e){
    console.log("close");
}
ws.onerror = function(e){
    console.log(error);
}