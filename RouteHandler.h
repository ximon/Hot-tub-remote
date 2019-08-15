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
    RouteHandler(String routePath, HTTPMethod routeMethod, THandlerFunction routeHandler) {
      route = routePath;
      httpMethod = routeMethod;
      handler = routeHandler;
    }
 
    bool canHandle(HTTPMethod requestMethod, String requestUri) {
/*
        Serial.println("CanHandle()");
        Serial.print("  Request: ");
        Serial.print(requestMethod);
        Serial.print(" @ <");
        Serial.print(requestUri);
        Serial.println(">");

        Serial.print("  Props: <");
        Serial.print(httpMethod);
        Serial.print(" @ <");
        Serial.print(route);
        Serial.println(">");

        Serial.print("Result: ");
        Serial.println(requestMethod == httpMethod && requestUri == route);
        */
      
        return requestMethod == httpMethod && requestUri == route;
    }

    bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri){
      if (!canHandle(requestMethod, requestUri))
        return false;
        
      handler();
      return true;
    }
};

#endif
