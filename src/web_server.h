/**
 * Web Server - Captive portal and local web UI
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>

class WebServer {
public:
    WebServer();
    void begin();
    void loop();
    void stop();

private:
    void setupRoutes();
    void handleRoot();
    void handleSetup();
    void handleConfig();
    void handleStatus();
    void handleNotFound();
};

#endif // WEB_SERVER_H
