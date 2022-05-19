$(document).ready(function(){

        var WEBSOCKET_ROUTE = "/ws";

        if(window.location.protocol == "http:"){
            //localhost
            var ws = new WebSocket("ws://" + window.location.host + WEBSOCKET_ROUTE);
            }
        else if(window.location.protocol == "https:"){
            //Dataplicity
            var ws = new WebSocket("wss://" + window.location.host + WEBSOCKET_ROUTE);
            }

        ws.onopen = function(evt) {
            $("#ws-status").html("Connected");
            };

        ws.onmessage = function(evt) {
            $("#horse-status").html(evt.data);
            };

        ws.onclose = function(evt) {
            $("#ws-status").html("Disconnected");
            };

        $("#horse_on").click(function(){
            ws.send("on_h");
            });

        $("#horse_off").click(function(){
            ws.send("off_h");
            });
        
        $("#horse_em_stop").click(function(){
            ws.send("em_stop");
            });

        $("#horse_update_stat").click(function(){
            ws.send("stat_h");
            });


      });
