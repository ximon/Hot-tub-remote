#ifndef ROUTEHANDLER_H
#define ROUTEHANDLER_H

#include <ESP8266WebServer.h>

class RouteHandler : public RequestHandler {
  private:
    String route;
    HTTPMethod httpMethod;
    
    typedef std::function<void(void)> THandlerFunction;
    THandlerFunction handler;
    
  public:
    RouteHandler(String routePath, HTTPMethod routeMethod, THandlerFunction routeHandler);
    bool canHandle(HTTPMethod requestMethod, String requestUri);
    bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri);
};

#endif
