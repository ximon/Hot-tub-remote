#include "RouteHandler.h"
#include <ESP8266WebServer.h>

RouteHandler::RouteHandler(char *routePath, HTTPMethod routeMethod, THandlerFunction routeHandler)
{
  route = routePath;
  httpMethod = routeMethod;
  handler = routeHandler;
}

bool RouteHandler::canHandle(HTTPMethod requestMethod, char *requestUri)
{
  return requestMethod == httpMethod && requestUri == route;
}

bool RouteHandler::handle(ESP8266WebServer &server, HTTPMethod requestMethod, char *requestUri)
{
  if (!canHandle(requestMethod, requestUri))
    return false;

  handler();
  return true;
}
