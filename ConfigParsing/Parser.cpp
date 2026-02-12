/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: praders <praders@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/12 12:58:51 by praders           #+#    #+#             */
/*   Updated: 2026/02/12 17:36:10 by praders          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../InConFile/Parser.hpp"
#include <cstdlib>
#include <sstream>
#include <cerrno>
#include <climits>

//ParseError
ParseError::ParseError(void): std::runtime_error("parse error"), line(0), col(0){}

ParseError::ParseError(ParseError const &other): std::runtime_error(other), line(other.line), col(other.col){}

ParseError &ParseError::operator=(ParseError const &other){
	if (this != &other){
		line = other.line;
		col = other.col;
	}
	return (*this);
}

ParseError::~ParseError(void) throw(){}

ParseError::ParseError(std::string const &msg, Token const &at): std::runtime_error(format(msg, at)), line(at.line), col(at.col){}

		
std::string ParseError::format(std::string const &msg, Token const &at){
	std::ostringstream oss;
	oss << msg << " (line " << at.line << ", col " << at.col << ")";
	return (oss.str());
}

//Parser
Parser::Parser(void): _token(NULL), _pos(0){}

Parser::Parser(Parser const &other): _token(other._token), _pos(other._pos){}

Parser &Parser::operator=(Parser const &other){
	if (this != &other){
		_token = other._token;
		_pos = other._pos;
	}
	return (*this);
}

Parser::~Parser(void){}

Parser::Parser(std::vector<Token> const &Tokens): _token(&Tokens), _pos(0){}

//Functions
Token const &Parser::peek(size_t offset) const{
	if (_token == NULL || _token->empty())
		throw std::runtime_error("Parser::peek: no tokens");
	size_t i = _pos + offset;
	if (i >= _token->size())
		return (_token->back());
	return ((*_token)[i]);
}

bool Parser::match(TokenType type) const{
	return (peek().type == type);
}

bool Parser::matchIdent(std::string const &kw) const{
	return (peek().type == Identifier && peek().value == kw);
}

Token const &Parser::expectIdent(char const *msg){
	if (!match(Identifier))
		throw ParseError(msg, peek());
	return (consume());
}

Token const &Parser::consume(void){
	Token const &token = peek();
	if (_token && _pos < _token->size())
		++_pos;
	return (token);
}

Token const &Parser::expect(TokenType type, char const *msg){
	if (!match(type))
		throw ParseError(msg, peek());
	return (consume());
}

void Parser::expectSemicolon(void){
	expect(Semicolon, "expected ';'");
}

Config Parser::parseconfig(void){
	Config server;
	
	while (peek().type != End){
		if (!matchIdent("server"))
			throw ParseError("expected 'server' block", peek());
		server.servers.push_back(parseserver());
	}
	return (server);
}

static ServerConfig defaultServerConfig(void){
	ServerConfig server;
	server.port = 80;
	server.client_max_body_size = 0;
	server.host = "0.0.0.0";
	server.root = "";
	server.index = "";
	return (server);
}

static long parseLongStrict(std::string const &s, bool&ok){
	ok = false;
	errno = 0;
	char *end = NULL;
	const char *start = s.c_str();
	long v = ::strtol(start, &end, 10);
	if (start == end)
		return (0);
	if (end == NULL || *end)
		return (0);
	if (errno == ERANGE)
		return (0);
	ok = true;
	return (v);
}

static size_t parseSizeTStrict(std::string const &s, bool&ok){
	ok = false;
	errno = 0;
	char *end = NULL;
	const char *start = s.c_str();
	unsigned long v = ::strtoul(start, &end, 10);
	if (start == end)
		return (0);
	if (end == NULL || *end)
		return (0);
	if (errno == ERANGE)
		return (0);
	if (v > static_cast<unsigned long>(static_cast<size_t>(-1)))
		return (0);
	ok = true;
	return (static_cast<size_t>(v));
}

ServerConfig Parser::parseserver(){
	expectIdent("expected 'server'");
	expect(LBrace, "expected '{' after server");
	ServerConfig srv = defaultServerConfig();
	bool seen_listen = false;
	bool seen_host = false;
	bool seen_root = false;
	bool seen_index = false;
	bool seen_client_max_body_size = false;
	while (!match(RBrace)){
		if (match(End))
			throw ParseError("unexpected end of file inside server block", peek());
		if (!match(Identifier))
			throw ParseError("expected directive name inside server block", peek());
		std::string name = consume().value;
		if (name == "listen"){
			if (seen_listen)
				throw ParseError("duplicate 'listen' in server block", peek());
			if (!match(Number))
				throw ParseError("expected port number after listen", peek());
			bool ok = false;
			long n = parseLongStrict(peek().value, ok);
			if (!ok)
				throw ParseError("invalid port number", peek());
			if (n <= 0 || n > 65535)
				throw ParseError("port number out of range", peek());
			consume();
			srv.port = static_cast<int>(n);
			expectSemicolon();
			seen_listen = true;
		}
		else if (name == "host"){
			if (seen_host)
				throw ParseError("duplicate 'host' in server block", peek());
			if (!(match(Identifier) || match(String) || match(Number)))
				throw ParseError("expected value after host", peek());
			srv.host = consume().value;
			expectSemicolon();
			seen_host = true;
		}
		else if (name == "root"){
			if (seen_root)
				throw ParseError("duplicate 'root' in server block", peek());
			if (!(match(Identifier) || match(String) || match(Number)))
				throw ParseError("expected value after root", peek());
			srv.root = consume().value;
			expectSemicolon();
			seen_root = true;
		}
		else if (name == "index"){
			if (seen_index)
				throw ParseError("duplicate 'index' in server block", peek());
			if (!(match(Identifier) || match(String) || match(Number)))
				throw ParseError("expected value after index", peek());
			srv.index = consume().value;
			expectSemicolon();
			seen_index = true;
		}
		else if (name == "client_max_body_size"){
			if (seen_client_max_body_size)
				throw ParseError("duplicate 'client_max_body_size' in server block", peek());
			if (!match(Number))
				throw ParseError("expected number after client_max_body_size", peek());
			bool ok = false;
			const size_t MAX_BODY_SIZE = 50 * 1024 * 1024;
			size_t n = parseSizeTStrict(peek().value, ok);
			if (!ok)
				throw ParseError("invalid client_max_body_size", peek());
			if (n > MAX_BODY_SIZE)
				throw ParseError("client_max_body_size too large (Max 50MB)", peek());
			consume();
			srv.client_max_body_size = n;
			expectSemicolon();
			seen_client_max_body_size = true;
		}
		else if (name == "error_page"){
			if (!match(Number))
				throw ParseError("expected status code after error_page", peek());
			bool ok = false;
			long codeL = parseLongStrict(peek().value, ok);
			if (!ok)
				throw ParseError("invalid status code in error_page", peek());
			if (codeL < 300 || codeL > 599)
				throw ParseError("status code out of range in error_page", peek());
			int code = static_cast<int>(codeL);
			consume();
			if (!(match(Identifier) || match(String) || match(Number)))
				throw ParseError("expected path after error_page <code>", peek());
			std::string path = consume().value;
			if (srv.error_pages.find(code) != srv.error_pages.end())
				throw ParseError("duplicate error_page code in server block", peek());
			srv.error_pages[code] = path;
			expectSemicolon();
		}
		else if (name == "location"){
			if (!(match(Identifier) || match(String) || match(Number)))
				throw ParseError("expected path after location", peek());
			std::string path = consume().value;
			LocationConfig loc = parselocation(path);
			srv.locations.push_back(loc);
		}
		else{
			throw ParseError(("unknown directive in server: " + name).c_str(), peek());
		}
	}
	expect(RBrace, "expected '}' to close server block");
	return (srv);
}

static LocationConfig defaultLocationConfig(std::string const &path){
	LocationConfig loc;
	loc.path = path;
	loc.redirect_code = 0;
	loc.allow_get = true;
	loc.allow_post = false;
	loc.allow_delete = false;
	loc.autoindex = false;
	loc.has_redirect = false;
	loc.upload_enabled = false;
	loc.root = 
}

LocationConfig Parser::parselocation(std::string const &path){
	expect(LBrace, "expected '{' after server");
	LocationConfig loc = defaultLocationConfig(path);
}
