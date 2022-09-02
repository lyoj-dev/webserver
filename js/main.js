var ws = new WebSocket("wss://localhost.littleyang.ml:8080/"); 
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