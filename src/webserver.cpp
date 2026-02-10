/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Implementacja serwera WWW (WiFi AP)
 * Serwer HTTP: ESP-IDF natywny (esp_http_server)
 * Dostęp: http://192.168.4.1
 * =============================================================
 */

#include "webserver.h"
#include <WiFi.h>
#include <esp_http_server.h>
#include <ArduinoJson.h>
#include "patterns.h"
#include "guns.h"
#include "painter.h"
#include "encoder.h"
#include "storage.h"

KM251WebServer webServer;

static httpd_handle_t httpServer = NULL;

// =============================================================
// Deklaracje wyprzedzające - generatory HTML/JSON
// =============================================================
static String generateMainPage();
static String generateStatusJSON();
static String generateCalibrationPage();

// =============================================================
// Helpers
// =============================================================
static esp_err_t sendHTML(httpd_req_t *req, const String& html)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.length());
    return ESP_OK;
}

static esp_err_t sendJSON(httpd_req_t *req, const char* json)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    return ESP_OK;
}

static esp_err_t sendJSONOk(httpd_req_t *req)
{
    return sendJSON(req, "{\"ok\":true}");
}

static esp_err_t sendJSONError(httpd_req_t *req)
{
    httpd_resp_set_status(req, "400 Bad Request");
    return sendJSON(req, "{\"ok\":false}");
}

static String readPostBody(httpd_req_t *req)
{
    int len = req->content_len;
    if (len <= 0 || len > 256) return "";
    char buf[257];
    int ret = httpd_req_recv(req, buf, len);
    if (ret <= 0) return "";
    buf[ret] = '\0';
    return String(buf);
}

static String getArg(const String& body, const char* key)
{
    String search = String(key) + "=";
    int idx = body.indexOf(search);
    if (idx < 0) return "";
    idx += search.length();
    int end = body.indexOf('&', idx);
    if (end < 0) end = body.length();
    return body.substring(idx, end);
}

// =============================================================
// Handlery HTTP
// =============================================================
static esp_err_t handleRoot(httpd_req_t *req)
{
    return sendHTML(req, generateMainPage());
}

static esp_err_t handleStatus(httpd_req_t *req)
{
    String json = generateStatusJSON();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json.c_str(), json.length());
    return ESP_OK;
}

static esp_err_t handlePatternAxis(httpd_req_t *req)
{
    String body = readPostBody(req);
    String idStr = getArg(body, "id");
    if (idStr.length() > 0) {
        uint8_t id = idStr.toInt();
        if (id < NUM_PATTERNS) {
            patternManager.setActiveAxisPattern(static_cast<PatternID>(id));
            storageManager.saveLastAxisPattern(id);
            if (paintProcess.getState() == PaintState::PAINTING) {
                paintProcess.setAxisPattern(static_cast<PatternID>(id));
            }
            return sendJSONOk(req);
        }
    }
    return sendJSONError(req);
}

static esp_err_t handlePatternEdge(httpd_req_t *req)
{
    String body = readPostBody(req);
    String idStr = getArg(body, "id");
    if (idStr.length() > 0) {
        uint8_t id = idStr.toInt();
        if (id >= 10 && id <= 14) {
            patternManager.setActiveEdgePattern(static_cast<PatternID>(id));
            storageManager.saveLastEdgePattern(id);
            if (paintProcess.getState() == PaintState::PAINTING) {
                paintProcess.setEdgePattern(static_cast<PatternID>(id));
            }
            return sendJSONOk(req);
        }
    }
    return sendJSONError(req);
}

static esp_err_t handleReverse(httpd_req_t *req)
{
    patternManager.toggleReversed();
    return sendJSONOk(req);
}

static esp_err_t handleStart(httpd_req_t *req)
{
    paintProcess.start();
    return sendJSONOk(req);
}

static esp_err_t handlePause(httpd_req_t *req)
{
    paintProcess.pause();
    return sendJSONOk(req);
}

static esp_err_t handleResume(httpd_req_t *req)
{
    paintProcess.resume();
    return sendJSONOk(req);
}

static esp_err_t handleStop(httpd_req_t *req)
{
    paintProcess.stop();
    return sendJSONOk(req);
}

static esp_err_t handleCalibrationPage(httpd_req_t *req)
{
    return sendHTML(req, generateCalibrationPage());
}

static esp_err_t handleCalStart(httpd_req_t *req)
{
    wheelEncoder.startCalibration();
    return sendJSONOk(req);
}

static esp_err_t handleCalBegin(httpd_req_t *req)
{
    wheelEncoder.beginMeasurement();
    return sendJSONOk(req);
}

static esp_err_t handleCalEnd(httpd_req_t *req)
{
    wheelEncoder.endMeasurement();
    return sendJSONOk(req);
}

static esp_err_t handleCalSave(httpd_req_t *req)
{
    storageManager.saveCalibration(wheelEncoder.getCalibrationFactor());
    return sendJSONOk(req);
}

// =============================================================
// Rejestracja endpointow
// =============================================================
static void registerURI(httpd_handle_t srv, const char* uri,
                         httpd_method_t method, esp_err_t (*handler)(httpd_req_t*))
{
    httpd_uri_t def = {};
    def.uri       = uri;
    def.method    = method;
    def.handler   = handler;
    def.user_ctx  = NULL;
    httpd_register_uri_handler(srv, &def);
}

// =============================================================
// Klasa KM251WebServer
// =============================================================

KM251WebServer::KM251WebServer()
    : _clientConnected(false)
    , _wifiStarted(false)
{
}

void KM251WebServer::begin()
{
    _setupWiFiAP();
    _setupRoutes();
    Serial.printf("[WEB] Serwer uruchomiony na http://%s\n", getIPAddress().c_str());
}

void KM251WebServer::update()
{
    // ESP-IDF httpd dziala na osobnym tasku FreeRTOS
    // Nie wymaga obslugi w loop()
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
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = WEB_SERVER_PORT;
    config.max_uri_handlers = 16;
    config.stack_size = 8192;

    if (httpd_start(&httpServer, &config) != ESP_OK) {
        Serial.println("[WEB] BLAD uruchamiania serwera HTTP!");
        return;
    }

    // Strony HTML
    registerURI(httpServer, "/",             HTTP_GET,  handleRoot);
    registerURI(httpServer, "/calibration",  HTTP_GET,  handleCalibrationPage);

    // API - status
    registerURI(httpServer, "/api/status",   HTTP_GET,  handleStatus);

    // API - wzorce
    registerURI(httpServer, "/api/pattern/axis", HTTP_POST, handlePatternAxis);
    registerURI(httpServer, "/api/pattern/edge", HTTP_POST, handlePatternEdge);
    registerURI(httpServer, "/api/reverse",      HTTP_POST, handleReverse);

    // API - sterowanie
    registerURI(httpServer, "/api/start",    HTTP_POST, handleStart);
    registerURI(httpServer, "/api/pause",    HTTP_POST, handlePause);
    registerURI(httpServer, "/api/resume",   HTTP_POST, handleResume);
    registerURI(httpServer, "/api/stop",     HTTP_POST, handleStop);

    // API - kalibracja
    registerURI(httpServer, "/api/cal/start", HTTP_POST, handleCalStart);
    registerURI(httpServer, "/api/cal/begin", HTTP_POST, handleCalBegin);
    registerURI(httpServer, "/api/cal/end",   HTTP_POST, handleCalEnd);
    registerURI(httpServer, "/api/cal/save",  HTTP_POST, handleCalSave);

    Serial.printf("[WEB] Zarejestrowano 15 endpointow HTTP\n");
}

// =============================================================
// Generowanie strony HTML
// =============================================================
static String generateMainPage()
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
static String generateStatusJSON()
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
static String generateCalibrationPage()
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
