#ifndef ROUTEHANDLER_H
#define ROUTEHANDLER_H

#include <ESP8266WebServer.h>

class RouteHandler : public RequestHandler
{
private:
  char *route;
  HTTPMethod httpMethod;

  typedef std::function<void(void)> THandlerFunction;
  THandlerFunction handler;

public:
  RouteHandler(char *routePath, HTTPMethod routeMethod, THandlerFunction routeHandler);
  bool canHandle(HTTPMethod requestMethod, char *requestUri);
  bool handle(ESP8266WebServer &server, HTTPMethod requestMethod, char *requestUri);
};

#endif
