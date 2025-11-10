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
    bool isStarted() { return m_started; }

private:
    bool m_started;
    void setupRoutes();
};

#endif // WEB_SERVER_H
