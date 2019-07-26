// Codigo fuente comprimido desde https://htmlcompressor.com/compressor/}
const char PAGE_funciones_js[] PROGMEM = R"=====(var Estados,Programas,Bombas,Sistema,Nombres,Wifi,timer;function geid(a){return document.getElementById(a)}function CargarJSON(b){var a=new XMLHttpRequest();a.onreadystatechange=function(){if(this.readyState==4&&this.status==200){if(this.responseText!=null){var c=JSON.parse(this.responseText);console.log(c);if(b=="estados"){Estados=c;VerEstados()}if(b=="salidas"){Programas=c;VerSalida()}if(b=="bombas"){Bombas=c;VerBomba()}if(b=="sistema"){Sistema=c;VerSistema()}if(b=="nombres"){Nombres=c;VerNombres()}if(b=="wifi"){Wifi=c;VerWifi()}}}};a.open("GET",b,true);a.send();if(b=="estados"){timer=setTimeout('CargarJSON("estados")',2000)}else{clearTimeout(timer)}}function Cargar(){CargarJSON("nombres");visibilidad("estados")}function visibilidad(b){var d=document.getElementsByName("main-content")[0],a=d.getElementsByTagName("article"),e=a.length;for(var c=0;c<e;c++){if(a[c].getAttribute("name")==b){a[c].style.display="block"}else{a[c].style.display="none"}}CargarJSON(b)}function ToResponsive(){var a=geid("myTopnav");if(a.className==="topnav"){a.className+=" responsive"}else{a.className="topnav"}}function VerModo(f,e,c){var a=geid(f);var b=geid(e);var d=geid(c);if(a.checked){b.style.display="block";d.style.display="none"}else{b.style.display="none";d.style.display="block"}}function VerEstados(){geid("status0").innerHTML=Estados[0]?"Activado":"Desactivado";geid("status1").innerHTML=Estados[1]?" *** BLOQUEADO ***":"";for(i=2;i<Estados.length;i++){var b=Estados[i];var a=geid("status"+i);if(b==1){a.style.background="lime"}else{a.style.background="red"}}}function VerSalida(){var d=geid("selecsalida");var a=d.options[d.selectedIndex].value;var c=a-1;var b=geid("salidax");var f=Programas[c];if(a==0){b.style.display="none"}else{if(f[0]){geid("ctrlauto").checked=true;VerModo("ctrlauto","auto","manual")}else{geid("ctrlmanual").checked=true;VerModo("ctrlmanual","manual","auto");var e=geid("ManS");if(Estados[a+2]){e.value="OFF"}else{e.value="ON"}}geid("hini").value=CorregirValor(f[1])+":"+CorregirValor(f[2]);geid("hfin").value=CorregirValor(f[3])+":"+CorregirValor(f[4]);geid("dur").value=f[5];geid("rep").value=f[6];geid("lu").checked=f[7];geid("ma").checked=f[8];geid("mi").checked=f[9];geid("ju").checked=f[10];geid("vi").checked=f[11];geid("sa").checked=f[12];geid("do").checked=f[13];SetVal("dur","durv");SetVal("rep","repv");b.style.display="block"}}function VerNombres(){for(var a=0;a<10;a++){geid("status"+(2+a)).insertAdjacentHTML("afterend","<a class='tag' id='nombre"+a+"' href='#' onclick='CambiarNombre(this.id)'> "+Nombres[a]+"</a>")}}function CambiarNombre(b){var c=geid(b).text;var a=parseInt(b.split("nombre")[1]);var d=prompt("Ingrese un nombre para esta salida:",c);if(d!=null){if(d!=c){geid(b).innerHTML=" "+d;Nombres[a]=d;EnviarJSON(Nombres,"setnombres")}}}function CorregirValor(a){str=""+a;var b="00";var c=b.substring(0,b.length-str.length)+str;return c}function VerBomba(){var e=geid("selecbomba");var b=e.options[e.selectedIndex].value;var a=b-1;var d=geid("bombax");var c=Bombas[a];if(b==0){d.style.display="none"}else{if(c[0]){geid("ctrlautob").checked=true;VerModo("ctrlautob","autob","manualb")}else{geid("ctrlmanualb").checked=true;VerModo("ctrlmanualb","manualb","autob");var f=geid("ManB");if(Estados[a+10]){f.value="OFF"}else{f.value="ON"}}geid("uno").checked=c[1];geid("dos").checked=c[2];geid("tre").checked=c[3];geid("cua").checked=c[4];geid("cin").checked=c[5];geid("sei").checked=c[6];geid("sie").checked=c[7];geid("och").checked=c[8];d.style.display="block"}}function VerSistema(){geid("sisauto").checked=Sistema[0];geid("inext").checked=Sistema[1];geid("inextmode").checked=Sistema[2];geid("hsis").value=CorregirValor(Sistema[3])+":"+CorregirValor(Sistema[4]);geid("fsis").value="20"+CorregirValor(Sistema[7])+"-"+CorregirValor(Sistema[6])+"-"+CorregirValor(Sistema[5])}function SendConfSalida(){var d=geid("selecsalida");var c=d.options[d.selectedIndex].value-1;var a=[];a[0]=c;a[1]=geid("ctrlauto").checked?1:0;var e=geid("hini").value;a[2]=parseInt(e.split(":")[0]);a[3]=parseInt(e.split(":")[1]);var b=geid("hfin").value;a[4]=parseInt(b.split(":")[0]);a[5]=parseInt(b.split(":")[1]);a[6]=parseInt(geid("dur").value);a[7]=parseInt(geid("rep").value);a[8]=geid("lu").checked?1:0;a[9]=geid("ma").checked?1:0;a[10]=geid("mi").checked?1:0;a[11]=geid("ju").checked?1:0;a[12]=geid("vi").checked?1:0;a[13]=geid("sa").checked?1:0;a[14]=geid("do").checked?1:0;a[15]=(geid("ManS").value=="ON")?1:0;EnviarJSON(a,"setsalidas")}function CambiarValor(a){var b=geid(a);if(b.value=="ON"){b.value="OFF"}else{b.value="ON"}}function SendConfBomba(){var b=geid("selecbomba");var a=b.options[b.selectedIndex].value-1;var c=[];c[0]=a;c[1]=geid("ctrlautob").checked?1:0;c[2]=geid("uno").checked?1:0;c[3]=geid("dos").checked?1:0;c[4]=geid("tre").checked?1:0;c[5]=geid("cua").checked?1:0;c[6]=geid("cin").checked?1:0;c[7]=geid("sei").checked?1:0;c[8]=geid("sie").checked?1:0;c[9]=geid("och").checked?1:0;c[10]=(geid("ManB").value=="ON")?1:0;EnviarJSON(c,"setbombas")}function SendConfSistema(){var c=[];c[0]=geid("sisauto").checked?1:0;c[1]=geid("inext").checked?1:0;c[2]=geid("inextmode").checked?1:0;var b=geid("hsis").value;c[3]=parseInt(b.split(":")[0]);c[4]=parseInt(b.split(":")[1]);var a=geid("fsis").value;c[5]=parseInt(a.split("-")[2]);c[6]=parseInt(a.split("-")[1]);c[7]=parseInt(a.split("-")[0])-2000;EnviarJSON(c,"setsistema")}function EnviarJSON(c,a){var b=new XMLHttpRequest();b.open("PUT",a,true);b.setRequestHeader("Content-Type","text/json");b.send(JSON.stringify(c))}function SetVal(b,a){geid(a).innerHTML=geid(b).value}function SendReset(){if(confirm("Esto reiniciar� los par�metros de f�brica, Esta Seguro???")==true){var a=new XMLHttpRequest();a.open("GET","reset",null);a.send()}alert("Reinicie el equipo para aplicar los cambios!")}function VerWifi(){geid("onlinesw").checked=Wifi.ONLINE;if(!Wifi.ONLINE){geid("online").style.display="none"}geid("ApSSID").value=Wifi.SSID;geid("ApPASS").value=Wifi.PASS;geid("dhcpsw").checked=Wifi.DHCP;if(!Wifi.DHCP){geid("dhcp").style.display="none"}geid("IP").value=Wifi.IP;geid("GW").value=Wifi.GW;geid("MASK").value=Wifi.MASK;geid("serversw").checked=Wifi.SERVER;if(!Wifi.SERVER){geid("server").style.display="none"}geid("serverdir").value=Wifi.SERVERDIR;geid("user").value=Wifi.SRV_USER;geid("pass").value=Wifi.SRV_PASS}function SendConfWifi(){var a=Wifi;a.ONLINE=geid("onlinesw").checked?1:0;a.SSID=geid("ApSSID").value;a.PASS=geid("ApPASS").value;a.DHCP=geid("dhcpsw").checked?1:0;a.IP=geid("IP").value;a.GW=geid("GW").value;a.MASK=geid("MASK").value;a.SERVER=geid("serversw").checked?1:0;a.SERVERDIR=geid("serverdir").value;EnviarJSON(a,"setwifi")}function ValidarIP(a){var b=geid(a);var c=/^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;if(b.value.match(c)){b.style.background="lightgreen";return true}else{alert("Error en la direcci�n !!!");b.style.background="red";return false}};)=====";