/*
TODO:
inherit root from global - DONE
do autoindex and redirect? (change codes) - DONE
need headers trimmed and in lowercase from parsing - DONE
is serverconfig index file a name or path? - DONE
introduce allowed methods in post/delete - DONE
post only with uploadpath or uri sensitive (at least !eisdir) - DONE
path parsing coming from http (igonre query strings, ...) 
introduce response constraints (maxlength, ...)
*/

#include <iostream>
#include "../inc/HttpRequest.hpp"
#include "../inc/ServerConfig.hpp"
#include "../inc/Location.hpp"
#include "../inc/RequestHandler.hpp"
#include "../inc/ResponseBuilder.hpp"
#include "../inc/FileService.hpp"

int main()
{
    // --- Set up server config (struct-based) ---
    ServerConfig server;
    server.host = "127.0.0.1";
    server.port = 8080;
    server.root = "./www";
    server.index = "index.html";
    server.client_max_body_size = 1024 * 1024; // 1 MB

    // Error pages
    server.error_pages[404] = "./www/404.html";
    server.error_pages[403] = "./www/403.html";

    // Location: "/" -> content + upload + delete
    Location rootLoc;
    rootLoc.redirect_code = 302;
    rootLoc.allow_get = true;
    rootLoc.allow_post = true;
    rootLoc.allow_delete = true;
    rootLoc.autoindex = false;
    rootLoc.has_redirect = false;
    rootLoc.upload_enabled = true;
    rootLoc.client_max_body_size = 1024 * 1024;
    rootLoc.path = "/";
    rootLoc.root = "./www";
    rootLoc.index = "index.html";
    rootLoc.upload_store = "./www";
    rootLoc.redirect_target = "";

    // Location: "/auto" -> force autoindex by using missing index
    Location autoLoc;
    autoLoc.redirect_code = 302;
    autoLoc.allow_get = true;
    autoLoc.allow_post = false;
    autoLoc.allow_delete = false;
    autoLoc.autoindex = true;
    autoLoc.has_redirect = false;
    autoLoc.upload_enabled = false;
    autoLoc.client_max_body_size = 1024 * 1024;
    autoLoc.path = "/auto";
    autoLoc.root = "./www";
    autoLoc.index = "does_not_exist.html";
    autoLoc.upload_store = "";
    autoLoc.redirect_target = "";

    // Location: "/redir" -> redirect
    Location redirLoc;
    redirLoc.redirect_code = 302;
    redirLoc.allow_get = true;
    redirLoc.allow_post = false;
    redirLoc.allow_delete = false;
    redirLoc.autoindex = false;
    redirLoc.has_redirect = true;
    redirLoc.upload_enabled = false;
    redirLoc.client_max_body_size = 1024 * 1024;
    redirLoc.path = "/redir";
    redirLoc.root = "./www";
    redirLoc.index = "";
    redirLoc.upload_store = "";
    redirLoc.redirect_target = "/index.html";

    server.locations.push_back(rootLoc);
    server.locations.push_back(autoLoc);
    server.locations.push_back(redirLoc);

    // // Hardcoded POST test with Content-Disposition filename
    // HttpRequest postNamedReq;
    // postNamedReq.setMethod("POST");
    // postNamedReq.setUri("/");
    // postNamedReq.setVersion("HTTP/1.1");
    // postNamedReq.addHeader("content-type", "application/x-python-code; charset=utf-8");
    // postNamedReq.addHeader("content-disposition", "form-data; name=\"file\"; filename=file.py");
    // postNamedReq.setBody("print(\"Hello World\")");
    // HttpResponse postNamedRes = RequestHandler::handleRequest(postNamedReq, server);

    // std::cout << "[POST FILENAME TEST] Status: " << postNamedRes.getStatusCode() << std::endl;
    // std::cout << "[POST FILENAME TEST] Body:\n" << postNamedRes.getBody() << std::endl;

    // Hardcoded POST test with no filename
    HttpRequest postAnonReq;
    postAnonReq.setMethod("POST");
    postAnonReq.setUri("/index.html");
    postAnonReq.setVersion("HTTP/1.1");
    postAnonReq.setBody("{\"message\":\"anonymous upload\"}");
    HttpResponse postAnonRes = RequestHandler::handleRequest(postAnonReq, server);

    std::cout << "[POST DEFAULT NAME TEST] Status: " << postAnonRes.getStatusCode() << std::endl;
    std::cout << "[POST DEFAULT NAME TEST] Body:\n" << postAnonRes.getBody() << std::endl;

    // Hardcoded GET autoindex test
    HttpRequest autoReq;
    autoReq.setMethod("GET");
    autoReq.setUri("/auto");
    autoReq.setVersion("HTTP/1.1");
    HttpResponse autoRes = RequestHandler::handleRequest(autoReq, server);

    std::cout << "[AUTOINDEX TEST] Status: " << autoRes.getStatusCode() << std::endl;
    std::cout << "[AUTOINDEX TEST] Body:\n" << autoRes.getBody() << std::endl;

    // // Hardcoded redirect test
    // HttpRequest redirReq;
    // redirReq.setMethod("GET");
    // redirReq.setUri("/redir");
    // redirReq.setVersion("HTTP/1.1");
    // HttpResponse redirRes = RequestHandler::handleRequest(redirReq, server);

    // std::cout << "[REDIRECT TEST] Status: " << redirRes.getStatusCode() << std::endl;
    // std::cout << "[REDIRECT TEST] Body:\n" << redirRes.getBody() << std::endl;

	// Hardcoded get test
    HttpRequest getReq;
    getReq.setMethod("GET");
    getReq.setUri("/");
    getReq.setVersion("HTTP/1.1");
    HttpResponse getRes = RequestHandler::handleRequest(getReq, server);

    std::cout << "[GET TEST] Status: " << getRes.getStatusCode() << std::endl;
    std::cout << "[GET TEST] Body:\n" << getRes.getBody() << std::endl;

    return 0;
}
