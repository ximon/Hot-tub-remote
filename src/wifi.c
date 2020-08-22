#include <IotWebConf.h>

const char deviceName[] = "HotTubRemote";
const char initialApPassword[] = "HotTubRemote";
/*
DNSServer dnsServer;
WebServer webServer(80);

IotWebConf iotWebConf(deviceName, &dnsServer, &webServer, initialApPassword);
*/
void setupWiFi()
{
    /*
    iotWebConf.init();

    server.on("/", handleRoot);
    server.on("/config", [] { iotWebConf.handleConfig(); });
    server.onNotFound([]() { iotWebConf.handleNotFound(); })
    */
}

void processWiFi()
{
    /*
    iotWebConf.doLoop();
*/
}
/*
void handleRoot()
{
    // -- Let IotWebConf test and handle captive portal requests.
    if (iotWebConf.handleCaptivePortal())
    {
        // -- Captive portal request were already served.
        return;
    }
    String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
    s += "<title>IotWebConf 01 Minimal</title></head><body>Hello world!";
    s += "Go to <a href='config'>configure page</a> to change settings.";
    s += "</body></html>\n";

    server.send(200, "text/html", s);
}
*/