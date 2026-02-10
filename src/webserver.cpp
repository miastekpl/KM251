/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Implementacja serwera WWW (WiFi AP)
 * Serwer HTTP: ESP-IDF natywny (esp_http_server)
 * WiFi AP: Trassar / 12345678
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
#include "sdlogger.h"

KM251WebServer webServer;

static httpd_handle_t httpServer = NULL;

// =============================================================
// Deklaracje wyprzedzające - generatory HTML/JSON
// =============================================================
static String generateMainPage();
static String generateStatusJSON();

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

    Serial.printf("[WEB] Zarejestrowano 14 endpointow HTTP\n");
}

// =============================================================
// Strona glowna - panel sterowania smartfon
// =============================================================
static String generateMainPage()
{
    String html = R"rawhtml(<!DOCTYPE html><html lang="pl"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>KM251 Trassar</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,Arial,sans-serif;background:#111;color:#eee;padding:8px;max-width:480px;margin:0 auto}
h1{text-align:center;color:#0ff;margin:8px 0;font-size:1.3em}
h2{color:#0ff;font-size:1em;margin:8px 0 4px}
.card{background:#1a1a2e;border-radius:10px;padding:10px;margin:6px 0}
.status{display:grid;grid-template-columns:1fr 1fr;gap:6px}
.si{background:#0f3460;border-radius:8px;padding:8px;text-align:center}
.si.wide{grid-column:1/3}
.sl{font-size:0.65em;color:#999;text-transform:uppercase}
.sv{font-size:1.5em;font-weight:bold;color:#0ff}
.sv.big{font-size:2.2em}
.guns{display:flex;gap:4px;justify-content:center;margin:6px 0}
.gun{width:46px;height:36px;border-radius:6px;font-weight:bold;font-size:0.85em;
display:flex;align-items:center;justify-content:center;border:2px solid #333}
.gun-on{background:#0c0;color:#000;border-color:#0f0}
.gun-off{background:#222;color:#555;border-color:#333}
.gun-pat{background:#cc0;color:#000;border-color:#ff0}
.pats{display:grid;grid-template-columns:1fr 1fr;gap:4px}
.pb{padding:7px 4px;border:2px solid #333;border-radius:8px;background:#0f3460;color:#ddd;
font-size:0.8em;cursor:pointer;text-align:center;transition:0.15s;-webkit-tap-highlight-color:transparent}
.pb:active{transform:scale(0.95)}
.pb.act{border-color:#0f0;background:#0a3000;color:#0f0}
.cb{display:flex;gap:6px;margin:6px 0}
.btn{flex:1;padding:12px 4px;border:none;border-radius:8px;font-size:1em;font-weight:bold;
cursor:pointer;-webkit-tap-highlight-color:transparent;transition:0.15s}
.btn:active{transform:scale(0.95)}
.b-start{background:#090;color:#fff}
.b-pause{background:#e90;color:#000}
.b-stop{background:#b00;color:#fff}
.b-resume{background:#06a;color:#fff}
.b-rev{background:#609;color:#fff}
.warn{text-align:center;color:#f80;font-size:0.75em;margin:4px 0}
.info{text-align:center;color:#666;font-size:0.7em;margin:4px 0}
</style></head><body>
<h1>KM251 Trassar</h1>
<div class="card"><div class="status">
<div class="si wide"><div class="sl">Predkosc</div><div class="sv big" id="speed">---</div></div>
<div class="si"><div class="sl">Stan</div><div class="sv" id="state">---</div></div>
<div class="si"><div class="sl">Powierzchnia</div><div class="sv" id="area">---</div></div>
<div class="si"><div class="sl">Dystans</div><div class="sv" id="dist">---</div></div>
<div class="si"><div class="sl">Kalibracja</div><div class="sv" id="cal">---</div></div>
</div></div>
<div class="card"><h2>Pistolety</h2><div class="guns" id="guns"></div>
<div class="warn" id="speedWarn"></div></div>
<div class="card"><h2>Sterowanie</h2>
<div class="cb">
<button class="btn b-start" onclick="api('/api/start')">START</button>
<button class="btn b-pause" onclick="api('/api/pause')">PAUZA</button>
<button class="btn b-stop" onclick="api('/api/stop')">STOP</button>
</div>
<div class="cb">
<button class="btn b-resume" onclick="api('/api/resume')">WZNOW</button>
<button class="btn b-rev" onclick="api('/api/reverse')">ODWROC P-3</button>
</div></div>
<div class="card"><h2>Wzorce - Os Jezdni</h2><div class="pats" id="axP"></div></div>
<div class="card"><h2>Wzorce - Krawedz</h2><div class="pats" id="edP"></div></div>
<div class="card"><h2>Kalibracja</h2>
<div class="cb">
<button class="btn b-start" onclick="api('/api/cal/start')">1.Start</button>
<button class="btn b-resume" onclick="api('/api/cal/begin')">2.Jedz</button>
</div>
<div class="cb">
<button class="btn b-pause" onclick="api('/api/cal/end')">3.Stop</button>
<button class="btn b-rev" onclick="api('/api/cal/save');alert('Zapisano!')">4.Zapisz</button>
</div></div>
<div class="info">KM251 v)rawhtml";
    html += FW_VERSION_STRING;
    html += R"rawhtml( | WiFi: Trassar | http://192.168.4.1</div>
<script>
const AP=[
{i:0,c:'P-1a',n:'Przeryw. dluga'},{i:1,c:'P-1b',n:'Przeryw. krotka'},
{i:2,c:'P-1c',n:'Wydzielajaca'},{i:3,c:'P-1d',n:'Prowadz. waska'},
{i:4,c:'P-1e',n:'Prowadz. szer.'},{i:5,c:'P-2a',n:'Ciagla waska'},
{i:6,c:'P-2b',n:'Ciagla szer.'},{i:7,c:'P-3a',n:'Przekracz. dl.'},
{i:8,c:'P-3b',n:'Przekracz. kr.'},{i:9,c:'P-4',n:'Podwojna ciagla'}
];
const EP=[
{i:10,c:'P-6',n:'Ostrzegawcza'},{i:11,c:'P-7a',n:'Kraw.prz.szer.'},
{i:12,c:'P-7b',n:'Kraw.ciag.szer.'},{i:13,c:'P-7c',n:'Kraw.prz.wask.'},
{i:14,c:'P-7d',n:'Kraw.ciag.wask.'}
];
function api(u,b){fetch(u,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b||''}).catch(e=>{})}
function init(){
let h='';AP.forEach(p=>{h+='<button class="pb" id="a'+p.i+'" onclick="api(\'/api/pattern/axis\',\'id='+p.i+'\')">'+p.c+'<br><small>'+p.n+'</small></button>'});
document.getElementById('axP').innerHTML=h;
h='';EP.forEach(p=>{h+='<button class="pb" id="e'+p.i+'" onclick="api(\'/api/pattern/edge\',\'id='+p.i+'\')">'+p.c+'<br><small>'+p.n+'</small></button>'});
document.getElementById('edP').innerHTML=h;
}
function upd(){
fetch('/api/status').then(r=>r.json()).then(d=>{
document.getElementById('state').textContent=d.state;
document.getElementById('speed').textContent=d.speed_kmh.toFixed(1)+' km/h';
document.getElementById('dist').textContent=d.distance_m.toFixed(1)+' m';
document.getElementById('area').textContent=d.area_m2.toFixed(2)+' m\u00B2';
document.getElementById('cal').textContent=d.calibrated?'OK':'Wymagana';
document.getElementById('cal').style.color=d.calibrated?'#0f0':'#f00';
let g='';for(let i=0;i<6;i++){let on=d.guns[i];let ip=d.gun_pattern&&d.gun_pattern[i];
g+='<div class="gun '+(on?'gun-on':(ip?'gun-pat':'gun-off'))+'">P'+(i+1)+'</div>'}
document.getElementById('guns').innerHTML=g;
let w=document.getElementById('speedWarn');
if(d.state==='Malowanie'&&d.speed_kmh<3.0){w.textContent='! Za mala predkosc - pistolety zablokowane !';}else{w.textContent='';}
document.querySelectorAll('.pb').forEach(b=>b.classList.remove('act'));
let ae=document.getElementById('a'+d.axis_pattern);if(ae)ae.classList.add('act');
let ee=document.getElementById('e'+d.edge_pattern);if(ee)ee.classList.add('act');
}).catch(e=>{});}
init();upd();setInterval(upd,500);
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

    // Maska wzorca - które pistolety są w aktywnym wzorcu
    const PatternDef& axisDef = patternManager.getActiveAxisDef();
    const PatternDef& edgeDef = patternManager.getActiveEdgeDef();
    JsonArray patArr = doc["gun_pattern"].to<JsonArray>();
    for (uint8_t i = 0; i < NUM_GUNS; i++) {
        patArr.add((bool)(axisDef.guns[i] || edgeDef.guns[i]));
    }

    if (sdLogger.isReady()) {
        doc["sd_ok"] = true;
        doc["sd_size_mb"] = (unsigned long)sdLogger.getCardSizeMB();
    } else {
        doc["sd_ok"] = false;
    }

    String output;
    serializeJson(doc, output);
    return output;
}
