#include "RouteHandler.h"
#include <ESP8266WebServer.h>

RouteHandler::RouteHandler(String routePath, HTTPMethod routeMethod, THandlerFunction routeHandler) {
  route = routePath;
  httpMethod = routeMethod;
  handler = routeHandler;
}
 
bool RouteHandler::canHandle(HTTPMethod requestMethod, String requestUri) {
    return requestMethod == httpMethod && requestUri == route;
}

bool RouteHandler::handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri){
  if (!canHandle(requestMethod, requestUri))
    return false;
    
  handler();
  return true;
}
