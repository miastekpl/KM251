/**
 * =============================================================
 * KM251 - Sterownik Malowarki Pasów Drogowych
 * Moduł serwera WWW (WiFi AP)
 * =============================================================
 */

#ifndef KM251_WEBSERVER_H
#define KM251_WEBSERVER_H

#include <Arduino.h>
#include "config.h"

// =============================================================
// Klasa serwera WWW
// =============================================================
class KM251WebServer {
public:
    KM251WebServer();
    void begin();
    void update();  // Obsługa klientów - wywoływać w loop()

    bool isClientConnected() const { return _clientConnected; }
    String getIPAddress() const;

private:
    bool _clientConnected;
    bool _wifiStarted;

    void _setupWiFiAP();
    void _setupRoutes();
};

extern KM251WebServer webServer;

#endif // KM251_WEBSERVER_H
