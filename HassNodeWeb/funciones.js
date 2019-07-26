var JsonConfig, JsonData, NumSalidas, OutStatus;

function geid(s){ return document.getElementById(s);}

function Cargar() {
    CargarJsonConfig();
    visibilidad('home');
}

function CargarJsonConfig(){
    var i, iden;
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function(){
        if(this.readyState == 4 && this.status == 200){
            JsonConfig = JSON.parse(this.responseText);
            console.log(JsonConfig);
            
            geid('title_name').innerText = JsonConfig.NODE_NAME;
            geid('nodename').innerText = JsonConfig.NODE_NAME; // Cambia el nombre del nodo con la info de JSON
            if(JsonConfig.SENSORS.TEMP.active){
                geid("main-footer").insertAdjacentHTML('afterbegin',"<p id='temp' style='position:absolute; font-size: 2em;'></p>");
            }
            if(JsonConfig.MQTT.ACTIVE){
                // geid("main-footer").insertAdjacentHTML('afterbegin',"<p id='mqttstatus' style='position:absolute; right:10%; border:1px solid white; font-size:2em;'>MQTT</p>");
                geid("main-footer").insertAdjacentHTML('afterbegin',"<div style='position:absolute; right:10%; font-size:1.5em;'>MQTT <div id='mqttstatus'; class='Status'></div></div>");
            }
            NumSalidas = JsonConfig.OUTPUTS.NUM;
            // Creo los elementos de las salidas segun la cantidad, con id=salida(i)
            for(i=0;i<NumSalidas;i++)
            {
                iden = geid('out'+i);
                iden.innerHTML = "<br><label class='switch'><input type='checkbox' id='salida" + i + "'onclick='SendOut()'><div class='slider round'></div></label><br>";
                iden.insertAdjacentHTML('afterend',"<a class='tag' id='output_name" + i + "'href='#' onclick='CambiarNombre(this.id)'>" + JsonConfig.OUTPUTS.OUT[i].name +  "</a>");
                // iden.insertAdjacentText('afterend',JsonConfig.OUTPUTS.OUT[i].name);
                Ver(iden, true);
            }
            
            // MQTT
            geid('mqttsw').checked = JsonConfig.MQTT.ACTIVE? true:false;
            Ver(geid('mqttserver'),JsonConfig.MQTT.ACTIVE? true:false);
            geid('mqttserverdir').value = JsonConfig.MQTT.SERVER;
            geid('mqttuser').value = JsonConfig.MQTT.USER;
            geid('mqttpass').value = JsonConfig.MQTT.PASS;
            geid('mqttport').value = JsonConfig.MQTT.PORT;
            
            var table = geid('topics');
            var row = table.insertRow(1);           // Nueva Fila
            var nombre = row.insertCell(0);         // 
            var state_topic = row.insertCell(1);
            var set_topic = row.insertCell(2);

            if(JsonConfig.SENSORS.TEMP.active){
                nombre.innerHTML = "Temperatura";
                state_topic.innerHTML = "<input type='text' id='tempstatetopic'></input>";
                geid('tempstatetopic').value = JsonConfig.SENSORS.TEMP.mqtt.stateTopic;
            }
            
            for(i=0;i<NumSalidas;i++)
            {
                row = table.insertRow(i+2);
                
                nombre = row.insertCell(0);
                nombre.innerHTML = geid('output_name'+i).innerText;
                
                state_topic = row.insertCell(1);
                state_topic.innerHTML = "<input type='text' id='statetopic" + i + "'></input>";
                geid('statetopic'+i).value = JsonConfig.OUTPUTS.OUT[i].mqtt.stateTopic;

                set_topic = row.insertCell(2);
                set_topic.innerHTML = "<input type='text' id='settopic" + i + "'></input>";
                geid('settopic'+i).value = JsonConfig.OUTPUTS.OUT[i].mqtt.setTopic;
            }

            // Network - AP
            geid('apssid').value = JsonConfig.AP.SSID;
            geid('appass').value = JsonConfig.AP.PASS;
            geid('apip').value = JsonConfig.AP.IP;
            // Wifi
            geid('wifissid').value = JsonConfig.WIFI.SSID;
            geid('wifipass').value = JsonConfig.WIFI.PASS;
            geid('dhcpsw').checked = JsonConfig.WIFI.DHCP? true:false;
            Ver(geid('dhcp'),JsonConfig.WIFI.DHCP? false:true);
            geid('wifiip').value = JsonConfig.WIFI.IP;
            geid('wifimask').value = JsonConfig.WIFI.MASK;
            geid('wifigw').value = JsonConfig.WIFI.GW;

            // geid("tempsw").checked = JsonConfig.SENSORS.TEMP.active? true:false;
            // geid("numsalidas").value = NumSalidas;
        }
    };
    xhttp.open("GET", "config.json", true);
    xhttp.send();
}

function ValidarIP(iden){
    var obj = geid(iden);
    var ipformat = /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;
    if(obj.value.match(ipformat))
    {
        obj.style.background = "lightgreen";
        return true;
    }
    else
    {
        alert("Error en la dirección !!!");
        obj.style.background = "red";
        return false;
    }
}

function EnviarJSON(objeto, tag){
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.open("PUT",tag,true);
    xmlhttp.setRequestHeader("Content-Type", "text/json");
    xmlhttp.send(JSON.stringify(objeto));
    
}

// Envia un JSON de la forma: [0,1,....,0] segun el n° de salidas
// Acá no solicito respuesta (Deberia hacerlo con GET?), la visualizacion del estado de las salidas en la web depende de 
// la funcion setinterval. Ver https://circuits4you.com/2018/02/04/esp8266-ajax-update-part-of-web-page-without-refreshing/
function SendOut(){
    var JsonOut = [];
    for(var i=0;i<NumSalidas;i++){
        JsonOut[i] = geid("salida" + i).checked? 1:0;
    }
    EnviarJSON(JsonOut,"setout");
}

// Envia el JSON con la configuracion igual que config.json
function SendConfig(){
    JsonConfig.NODE_NAME = geid("nodename").innerText;
    JsonConfig.AP.SSID   = geid('apssid').value;
    JsonConfig.AP.PASS   = geid('appass').value;
    JsonConfig.AP.IP     = geid('apip').value;
    JsonConfig.WIFI.SSID = geid('wifissid').value;
    JsonConfig.WIFI.PASS = geid('wifipass').value;
    JsonConfig.WIFI.DHCP = geid('dhcpsw').checked? 1:0;
    JsonConfig.WIFI.IP   = geid('wifiip').value;
    JsonConfig.WIFI.MASK = geid('wifimask').value;
    JsonConfig.WIFI.GW   = geid('wifigw').value;
    JsonConfig.MQTT.ACTIVE = geid('mqttsw').checked? 1:0;
    JsonConfig.MQTT.SERVER = geid('mqttserverdir').value;
    JsonConfig.MQTT.USER = geid('mqttuser').value;
    JsonConfig.MQTT.PASS = geid('mqttpass').value;
    JsonConfig.MQTT.PORT = geid('mqttport').value;
    for(var i=0; i<NumSalidas; i++){
        JsonConfig.OUTPUTS.OUT[i].name = geid("output_name"+i).innerText;
        JsonConfig.OUTPUTS.OUT[i].mqtt.stateTopic = geid("statetopic"+i).value;
        JsonConfig.OUTPUTS.OUT[i].mqtt.setTopic = geid("settopic"+i).value;
    }
    JsonConfig.SENSORS.TEMP.mqtt.stateTopic = geid("tempstatetopic").value;
    
    EnviarJSON(JsonConfig,"config");
    console.log(JSON.stringify(JsonConfig));
}

function CambiarNombre(iden) {
    var antiguo = geid(iden).text;
    var nuevo = prompt("Ingrese el nuevo nombre:", antiguo);
    if(nuevo != null)
    {
        if(nuevo != antiguo)
        {
            geid(iden).innerHTML = " " + nuevo; // Si el nuevo nombre es diferente lo cambio
            SendConfig();
        }
    }
}

// Solicita data.json cada 2 seg. con info de temperatura y estado de salidas 
setInterval(function(){
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function(){
        if(this.readyState == 4 && this.status == 200){
            JsonData = JSON.parse(this.responseText);
            console.log(JsonData);
            geid('temp').innerText = JsonData.tempvalor;
            geid('mqttstatus').style.background = JsonData.mqtt?"lime":"red";
            
            for(var i=0; i<NumSalidas; i++)
            {
                geid('salida'+i).checked = JsonData.salidas[i]? true:false;
            }
        }
    };
    xhttp.open("GET", "data.json", true);
    xhttp.send();
}, 2000 );

function visibilidad(name) {
    var seccion = document.getElementsByName("main-content")[0],
        articulos = seccion.getElementsByTagName("article"),
        total = articulos.length;

    for (var i = 0; i < total; i++) {
        if (articulos[i].getAttribute("name") == name)  articulos[i].style.display = 'block';
        else                                            articulos[i].style.display = 'none';
    }
}

// Cambia la clase CSS de la barra de Navegación al detectar pantallas pequeñas para adaptar la web
function ToResponsive() {   
    var x = geid("myTopnav");
    if (x.className === "topnav") {
        x.className += " responsive";
    } else {
        x.className = "topnav";
    }
}

// Permite visualizar/ocultar un elemento por su id, ej: Ver(id_del_elemento, true/false)
function Ver(idctrl, view){
    if(view === true){
        idctrl.style.display = 'block';
    }
    else{
        idctrl.style.display = 'none';
    }
}

function SendSetup(tipo){
    var text, msg;
    switch(tipo){
        case 0: text = "El nodo se reiniciará! ";
                msg = "reboot";
                break;
        case 1: text = "Esto reiniciará los parámetros de fábrica! ";
                msg = "reset";
                break;
        default:break;
    }

    if(confirm(text + "Está Seguro?") == true){
        var xmlhttp = new XMLHttpRequest();
        xmlhttp.open("GET",msg,null);
        xmlhttp.send();
    }
    
    // alert("Reinicie el equipo para aplicar los cambios!");
}
