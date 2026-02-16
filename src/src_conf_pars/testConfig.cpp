#include "../../inc/inc_conf_pars/Config.hpp"
#include "../../inc/inc_conf_pars/Token.hpp"
#include "../../inc/inc_conf_pars/Parser.hpp"
#include <iostream>
#include <map>
#include <vector>
#include <string>

// helpers
static void indent(int n) {
	for (int i = 0; i < n; ++i) std::cout << ' ';
}

static const char* tf(bool b) { return b ? "true" : "false"; }

static void printErrorPages(const std::map<int, std::string>& m, int ind) {
	indent(ind);
	std::cout << "error_pages (" << m.size() << ")\n";
	for (std::map<int, std::string>::const_iterator it = m.begin(); it != m.end(); ++it) {
		indent(ind + 2);
		std::cout << it->first << " -> " << it->second << "\n";
	}
}

static void printCgi(const std::map<std::string, std::string>& m, int ind) {
	indent(ind);
	std::cout << "cgi (" << m.size() << ")\n";
	for (std::map<std::string, std::string>::const_iterator it = m.begin(); it != m.end(); ++it) {
		indent(ind + 2);
		std::cout << it->first << " -> " << it->second << "\n";
	}
}

static void printLocation(const LocationConfig& loc, int ind) {
	indent(ind);
	std::cout << "location " << loc.path << " {\n";

	indent(ind + 2);
	std::cout << "root: " << loc.root << "\n";
	indent(ind + 2);
	std::cout << "index: " << loc.index << "\n";
	indent(ind + 2);
	std::cout << "client_max_body_size: " << loc.client_max_body_size << "\n";

	indent(ind + 2);
	std::cout << "methods: GET=" << tf(loc.allow_get)
			  << " POST=" << tf(loc.allow_post)
			  << " DELETE=" << tf(loc.allow_delete) << "\n";

	indent(ind + 2);
	std::cout << "autoindex: " << tf(loc.autoindex) << "\n";

	indent(ind + 2);
	std::cout << "upload_enabled: " << tf(loc.upload_enabled) << "\n";
	indent(ind + 2);
	std::cout << "upload_store: " << loc.upload_store << "\n";

	indent(ind + 2);
	std::cout << "redirect: has=" << tf(loc.has_redirect)
			  << " code=" << loc.redirect_code
			  << " target=" << loc.redirect_target << "\n";

	printErrorPages(loc.error_pages, ind + 2);
	printCgi(loc.cgi, ind + 2);

	indent(ind);
	std::cout << "}\n";
}

static void printServer(const ServerConfig& srv, int ind) {
	indent(ind);
	std::cout << "server {\n";

	indent(ind + 2);
	std::cout << "listen: " << srv.port << "\n";
	indent(ind + 2);
	std::cout << "host: " << srv.host << "\n";
	indent(ind + 2);
	std::cout << "root: " << srv.root << "\n";
	indent(ind + 2);
	std::cout << "index: " << srv.index << "\n";
	indent(ind + 2);
	std::cout << "client_max_body_size: " << srv.client_max_body_size << "\n";

	printErrorPages(srv.error_pages, ind + 2);

	indent(ind + 2);
	std::cout << "locations (" << srv.locations.size() << ")\n";
	for (size_t i = 0; i < srv.locations.size(); ++i) {
		printLocation(srv.locations[i], ind + 4);
	}

	indent(ind);
	std::cout << "}\n";
}

void debugPrintConfig(const Config& cfg) {
	std::cout << "===== CONFIG DUMP =====\n";
	std::cout << "servers: " << cfg.servers.size() << "\n";
	for (size_t i = 0; i < cfg.servers.size(); ++i) {
		printServer(cfg.servers[i], 0);
	}
	std::cout << "===== END DUMP =====\n";
}

void printTok(std::vector<Token> tokens){
	for (size_t i = 0; i < tokens.size(); i++){
			std::string str[] = {"Identifier","String","Number","LBrace","RBrace","Semicolon","End"};
			std::cout << "Word: " << tokens[i].value << std::endl;
			std::cout << "Col:  " << tokens[i].col << std::endl;
			std::cout << "Lin:  " << tokens[i].line << std::endl;
			std::cout << "Type: " << str[tokens[i].type] << std::endl << std::endl;
	}
}