/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Implementacja serwera WWW (WiFi AP)
 * Dostęp: http://192.168.4.1
 * =============================================================
 */

#include "webserver.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "patterns.h"
#include "guns.h"
#include "painter.h"
#include "encoder.h"
#include "storage.h"

KM251WebServer webServer;

static WebServer server(WEB_SERVER_PORT);

KM251WebServer::KM251WebServer()
    : _clientConnected(false)
    , _wifiStarted(false)
{
}

void KM251WebServer::begin()
{
    _setupWiFiAP();
    _setupRoutes();
    server.begin();
    Serial.printf("[WEB] Serwer uruchomiony na http://%s\n", getIPAddress().c_str());
}

void KM251WebServer::update()
{
    server.handleClient();
}

String KM251WebServer::getIPAddress() const
{
    return WiFi.softAPIP().toString();
}

void KM251WebServer::_setupWiFiAP()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL);
    _wifiStarted = true;
    Serial.printf("[WEB] WiFi AP: %s haslo: %s\n", WIFI_AP_SSID, WIFI_AP_PASS);
}

void KM251WebServer::_setupRoutes()
{
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", _generateMainPage());
    });

    server.on("/api/status", HTTP_GET, []() {
        server.send(200, "application/json", _generateStatusJSON());
    });

    server.on("/api/pattern/axis", HTTP_POST, []() {
        if (server.hasArg("id")) {
            uint8_t id = server.arg("id").toInt();
            if (id < NUM_PATTERNS) {
                patternManager.setActiveAxisPattern(static_cast<PatternID>(id));
                storageManager.saveLastAxisPattern(id);
                if (paintProcess.getState() == PaintState::PAINTING) {
                    paintProcess.setAxisPattern(static_cast<PatternID>(id));
                }
                server.send(200, "application/json", "{\"ok\":true}");
                return;
            }
        }
        server.send(400, "application/json", "{\"ok\":false}");
    });

    server.on("/api/pattern/edge", HTTP_POST, []() {
        if (server.hasArg("id")) {
            uint8_t id = server.arg("id").toInt();
            if (id >= 10 && id <= 14) {
                patternManager.setActiveEdgePattern(static_cast<PatternID>(id));
                storageManager.saveLastEdgePattern(id);
                if (paintProcess.getState() == PaintState::PAINTING) {
                    paintProcess.setEdgePattern(static_cast<PatternID>(id));
                }
                server.send(200, "application/json", "{\"ok\":true}");
                return;
            }
        }
        server.send(400, "application/json", "{\"ok\":false}");
    });

    server.on("/api/reverse", HTTP_POST, []() {
        patternManager.toggleReversed();
        server.send(200, "application/json", "{\"ok\":true}");
    });

    server.on("/api/start", HTTP_POST, []() {
        paintProcess.start();
        server.send(200, "application/json", "{\"ok\":true}");
    });

    server.on("/api/pause", HTTP_POST, []() {
        paintProcess.pause();
        server.send(200, "application/json", "{\"ok\":true}");
    });

    server.on("/api/resume", HTTP_POST, []() {
        paintProcess.resume();
        server.send(200, "application/json", "{\"ok\":true}");
    });

    server.on("/api/stop", HTTP_POST, []() {
        paintProcess.stop();
        server.send(200, "application/json", "{\"ok\":true}");
    });

    server.on("/calibration", HTTP_GET, []() {
        server.send(200, "text/html", _generateCalibrationPage());
    });

    server.on("/api/cal/start", HTTP_POST, []() {
        wheelEncoder.startCalibration();
        server.send(200, "application/json", "{\"ok\":true}");
    });

    server.on("/api/cal/begin", HTTP_POST, []() {
        wheelEncoder.beginMeasurement();
        server.send(200, "application/json", "{\"ok\":true}");
    });

    server.on("/api/cal/end", HTTP_POST, []() {
        wheelEncoder.endMeasurement();
        server.send(200, "application/json", "{\"ok\":true}");
    });

    server.on("/api/cal/save", HTTP_POST, []() {
        storageManager.saveCalibration(wheelEncoder.getCalibrationFactor());
        server.send(200, "application/json", "{\"ok\":true}");
    });
}

// =============================================================
// Generowanie strony HTML
// =============================================================
String KM251WebServer::_generateMainPage()
{
    String html = R"rawhtml(<!DOCTYPE html><html lang="pl"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>KM251 Malowarka</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:Arial,sans-serif;background:#1a1a2e;color:#eee;padding:10px}
h1{text-align:center;color:#0ff;margin:10px 0;font-size:1.4em}
h2{color:#0ff;font-size:1.1em;margin:10px 0 5px}
.card{background:#16213e;border-radius:8px;padding:12px;margin:8px 0}
.status{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.stat-item{background:#0f3460;border-radius:6px;padding:8px;text-align:center}
.stat-label{font-size:0.7em;color:#aaa}
.stat-value{font-size:1.3em;font-weight:bold;color:#0ff}
.guns{display:flex;gap:6px;flex-wrap:wrap;justify-content:center;margin:8px 0}
.gun{padding:6px 12px;border-radius:6px;font-weight:bold;font-size:0.9em;min-width:44px;text-align:center}
.gun-on{background:#0f0;color:#000}.gun-off{background:#333;color:#666}
.patterns{display:grid;grid-template-columns:1fr 1fr;gap:6px}
.pat-btn{padding:8px;border:2px solid #333;border-radius:6px;background:#0f3460;color:#eee;
font-size:0.85em;cursor:pointer;text-align:center;transition:0.2s}
.pat-btn:hover{border-color:#0ff;background:#1a4080}
.pat-btn.active{border-color:#0f0;background:#0a3000;color:#0f0}
.ctrl-btns{display:flex;gap:8px;margin:8px 0}
.btn{flex:1;padding:10px;border:none;border-radius:6px;font-size:1em;font-weight:bold;cursor:pointer}
.btn-start{background:#0a0;color:#fff}.btn-pause{background:#fa0;color:#000}
.btn-stop{background:#a00;color:#fff}.btn-reverse{background:#06a;color:#fff}
.btn:hover{opacity:0.85}
</style></head><body>
<h1>KM251 Malowarka Pasow Drogowych</h1>
<div class="card"><div class="status">
<div class="stat-item"><div class="stat-label">Stan</div><div class="stat-value" id="state">---</div></div>
<div class="stat-item"><div class="stat-label">Predkosc</div><div class="stat-value" id="speed">---</div></div>
<div class="stat-item"><div class="stat-label">Dystans</div><div class="stat-value" id="dist">---</div></div>
<div class="stat-item"><div class="stat-label">Powierzchnia</div><div class="stat-value" id="area">---</div></div>
</div></div>
<div class="card"><h2>Pistolety</h2><div class="guns" id="guns"></div></div>
<div class="card"><h2>Sterowanie</h2><div class="ctrl-btns">
<button class="btn btn-start" onclick="apiPost('/api/start')">START</button>
<button class="btn btn-pause" onclick="apiPost('/api/pause')">PAUZA</button>
<button class="btn btn-stop" onclick="apiPost('/api/stop')">STOP</button>
</div><div class="ctrl-btns">
<button class="btn btn-pause" onclick="apiPost('/api/resume')">WZNOW</button>
<button class="btn btn-reverse" onclick="apiPost('/api/reverse')">ODWROC P-3</button>
</div></div>
<div class="card"><h2>Wzorce - Os Jezdni</h2><div class="patterns" id="axis-pats"></div></div>
<div class="card"><h2>Wzorce - Krawedz</h2><div class="patterns" id="edge-pats"></div></div>
<div class="card" style="text-align:center"><a href="/calibration" style="color:#0ff">Kalibracja enkodera</a></div>
<script>
const axisPats=[
{id:0,code:'P-1a',name:'Przerywana dluga'},
{id:1,code:'P-1b',name:'Przerywana krotka'},
{id:2,code:'P-1c',name:'Wydzielajaca'},
{id:3,code:'P-1d',name:'Prowadzaca waska'},
{id:4,code:'P-1e',name:'Prowadzaca szeroka'},
{id:5,code:'P-2a',name:'Ciagla waska'},
{id:6,code:'P-2b',name:'Ciagla szeroka'},
{id:7,code:'P-3a',name:'Przekraczalna dluga'},
{id:8,code:'P-3b',name:'Przekraczalna krotka'},
{id:9,code:'P-4',name:'Podwojna ciagla'}
];
const edgePats=[
{id:10,code:'P-6',name:'Ostrzegawcza'},
{id:11,code:'P-7a',name:'Kraw.przeryw.szer.'},
{id:12,code:'P-7b',name:'Kraw.ciagla szer.'},
{id:13,code:'P-7c',name:'Kraw.przeryw.wask.'},
{id:14,code:'P-7d',name:'Kraw.ciagla wask.'}
];
function apiPost(url,body){fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body||''}).then(r=>r.json()).catch(e=>console.error(e))}
function setAxis(id){apiPost('/api/pattern/axis','id='+id)}
function setEdge(id){apiPost('/api/pattern/edge','id='+id)}
function buildPats(){
let ah='';axisPats.forEach(p=>{ah+='<button class="pat-btn" id="ap'+p.id+'" onclick="setAxis('+p.id+')">'+p.code+'<br><small>'+p.name+'</small></button>'});
document.getElementById('axis-pats').innerHTML=ah;
let eh='';edgePats.forEach(p=>{eh+='<button class="pat-btn" id="ep'+p.id+'" onclick="setEdge('+p.id+')">'+p.code+'<br><small>'+p.name+'</small></button>'});
document.getElementById('edge-pats').innerHTML=eh;
}
function updateStatus(){
fetch('/api/status').then(r=>r.json()).then(d=>{
document.getElementById('state').textContent=d.state;
document.getElementById('speed').textContent=d.speed_kmh.toFixed(1)+' km/h';
document.getElementById('dist').textContent=d.distance_m.toFixed(1)+' m';
document.getElementById('area').textContent=d.area_m2.toFixed(2)+' m2';
let gh='';for(let i=0;i<6;i++){let on=d.guns[i];gh+='<div class="gun '+(on?'gun-on':'gun-off')+'">P'+(i+1)+'</div>'}
document.getElementById('guns').innerHTML=gh;
document.querySelectorAll('.pat-btn').forEach(b=>{b.classList.remove('active')});
let ae=document.getElementById('ap'+d.axis_pattern);if(ae)ae.classList.add('active');
let ee=document.getElementById('ep'+d.edge_pattern);if(ee)ee.classList.add('active');
}).catch(e=>{});}
buildPats();updateStatus();setInterval(updateStatus,500);
</script></body></html>)rawhtml";
    return html;
}

// =============================================================
// Status JSON
// =============================================================
String KM251WebServer::_generateStatusJSON()
{
    JsonDocument doc;

    doc["state"] = paintProcess.getStateString();
    doc["speed_kmh"] = wheelEncoder.getSpeedKMH();
    doc["distance_m"] = paintProcess.getDistance_m();
    doc["area_m2"] = paintProcess.getStats().totalArea_m2;
    doc["axis_pattern"] = static_cast<uint8_t>(patternManager.getActiveAxisPattern());
    doc["edge_pattern"] = static_cast<uint8_t>(patternManager.getActiveEdgePattern());
    doc["reversed"] = patternManager.isReversed();
    doc["calibrated"] = wheelEncoder.isCalibrated();

    JsonArray gunsArr = doc["guns"].to<JsonArray>();
    for (uint8_t i = 0; i < NUM_GUNS; i++) {
        gunsArr.add(gunController.isGunActive(static_cast<GunID>(i)));
    }

    String output;
    serializeJson(doc, output);
    return output;
}

// =============================================================
// Strona kalibracji
// =============================================================
String KM251WebServer::_generateCalibrationPage()
{
    String html = R"rawhtml(<!DOCTYPE html><html lang="pl"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>KM251 Kalibracja</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:Arial,sans-serif;background:#1a1a2e;color:#eee;padding:10px}
h1{text-align:center;color:#0ff;margin:10px 0}
.card{background:#16213e;border-radius:8px;padding:16px;margin:10px 0;text-align:center}
.step{font-size:1.2em;margin:10px 0;color:#0ff}
.info{color:#aaa;margin:5px 0}
.btn{padding:14px 30px;border:none;border-radius:8px;font-size:1.1em;font-weight:bold;cursor:pointer;margin:5px}
.btn-go{background:#0a0;color:#fff}.btn-cancel{background:#a00;color:#fff}
#status{font-size:1.3em;margin:10px 0}
#pulses{font-size:2em;color:#0ff;margin:10px 0}
</style></head><body>
<h1>Kalibracja Enkodera</h1>
<div class="card">
<div id="status">Nacisnij START</div>
<div id="pulses">---</div>
<div class="info">Procedura: START > jedz 10m > START > Zapisz</div>
<div style="margin-top:10px">
<button class="btn btn-go" onclick="calStart()">1. Rozpocznij</button>
<button class="btn btn-go" onclick="calBegin()">2. Jedz!</button>
<button class="btn btn-go" onclick="calEnd()">3. Stop (10m)</button>
<button class="btn btn-go" onclick="calSave()">4. Zapisz</button>
<button class="btn btn-cancel" onclick="location.href='/'">Powrot</button>
</div></div>
<script>
function apiPost(u){fetch(u,{method:'POST'})}
function calStart(){apiPost('/api/cal/start')}
function calBegin(){apiPost('/api/cal/begin')}
function calEnd(){apiPost('/api/cal/end')}
function calSave(){apiPost('/api/cal/save');alert('Zapisano!')}
function poll(){fetch('/api/status').then(r=>r.json()).then(d=>{
document.getElementById('status').textContent=d.calibrated?'Skalibrowany':'Wymaga kalibracji';
}).catch(e=>{});}
setInterval(poll,1000);
</script></body></html>)rawhtml";
    return html;
}
