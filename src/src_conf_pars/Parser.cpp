#include "../../inc/inc_conf_pars/Parser.hpp"
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
	server.client_max_body_size = 1 * 1024 * 1024;
	server.host = "0.0.0.0";
	server.root = ".";
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
			if (!(match(Identifier) || match(String)))
				throw ParseError("expected value after host", peek());
			srv.host = consume().value;
			if (srv.host.empty())
				throw ParseError("host value cannot be empty", peek());
			expectSemicolon();
			seen_host = true;
		}
		else if (name == "root"){
			if (seen_root)
				throw ParseError("duplicate 'root' in server block", peek());
			if (!(match(Identifier) || match(String)))
				throw ParseError("expected path after root", peek());
			std::string path = consume().value;
			if (path.empty())
				throw ParseError("root path cannot be empty", peek());
			if (path.size() > 1 && path[path.size() - 1] == '/')
				throw ParseError("path root ends with /", peek());
			srv.root = path;
			expectSemicolon();
			seen_root = true;
		}
		else if (name == "index"){
			if (seen_index)
				throw ParseError("duplicate 'index' in server block", peek());
			if (!(match(Identifier) || match(String)))
				throw ParseError("expected path after index", peek());
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
			if (!(match(Identifier) || match(String)))
				throw ParseError("expected path after error_page <code>", peek());
			std::string path = consume().value;
			if (path.empty())
				throw ParseError("error_page path cannot be empty", peek());
			if (srv.error_pages.find(code) != srv.error_pages.end())
				throw ParseError("duplicate error_page code in server block", peek());
			srv.error_pages[code] = path;
			expectSemicolon();
		}
		else if (name == "location"){
			if (!(match(Identifier) || match(String)))
				throw ParseError("expected path after location", peek());
			std::string path = consume().value;
			if (path.empty())
				throw ParseError("location path cannot be empty", peek());
			if (path[0] != '/')
				throw ParseError("location path must start with '/'", peek());
			if (path.size() > 1 && path[path.size() - 1] == '/')
				throw ParseError("path location ends with /", peek());
			LocationConfig loc = parselocation(path, srv);
			for (size_t i = 0; i < srv.locations.size(); i++)
				if (srv.locations[i].path == path)
					throw ParseError("duplicate location path in server block", peek());
			srv.locations.push_back(loc);
		}
		else{
			throw ParseError(std::string("unknown directive in server: ") + name, peek());
		}
	}
	expect(RBrace, "expected '}' to close server block");
	for (size_t i = 0; i < srv.locations.size(); i++)
		for (std::map<int, std::string>::const_iterator it = srv.error_pages.begin(); it != srv.error_pages.end(); ++it)
			if (srv.locations[i].error_pages.count(it->first) == 0)
				srv.locations[i].error_pages[it->first] = it->second;
	return (srv);
}

static LocationConfig defaultLocationConfig(std::string const &path, ServerConfig const &srv){
	LocationConfig loc;
	loc.path = path;
	loc.redirect_code = 0;
	loc.allow_get = true;
	loc.allow_post = false;
	loc.allow_delete = false;
	loc.autoindex = false;
	loc.has_redirect = false;
	loc.upload_enabled = false;
	loc.client_max_body_size = srv.client_max_body_size;
	loc.root = srv.root;
	loc.index = srv.index;
	loc.upload_store = "";
	loc.redirect_target = "";
	return (loc);
}

LocationConfig Parser::parselocation(std::string const &path, ServerConfig const &srv){
	expect(LBrace, "expected '{' after location");
	LocationConfig loc = defaultLocationConfig(path, srv);
	bool seen_allow_methods = false;
	bool seen_autoindex = false;
	bool seen_root = false;
	bool seen_index = false;
	bool seen_client_max_body_size = false;
	bool seen_upload_store = false;
	bool seen_upload = false;
	bool seen_return = false;
	while(!match(RBrace)){
		if (match(End))
			throw ParseError("unexpected end of file inside location block", peek());
		if (!match(Identifier))
			throw ParseError("expected directive name inside location block", peek());
		std::string name = consume().value;
		if (name == "allow_methods"){
			if (seen_allow_methods)
				throw ParseError("duplicate 'allow_methods' in location block", peek());
			if (match(Semicolon))
				throw ParseError("allow_methods requires at least one method", peek());
			loc.allow_get = false;
			loc.allow_post = false;
			loc.allow_delete = false;
			while (!match(Semicolon)){
				if (match(End))
					throw ParseError("unexpected end of file in allow_methods", peek());
				if (!match(Identifier))
					throw ParseError("expected method name in allow_methods", peek());
				std::string m = consume().value;
				if (m == "GET") loc.allow_get = true;
				else if (m == "POST") loc.allow_post = true;
				else if (m == "DELETE") loc.allow_delete = true;
				else
					throw ParseError(std::string("unsupported method in allow_methods: ") + m, peek());
			}
			expectSemicolon();
			seen_allow_methods = true;
		}
		else if (name == "autoindex"){
			if (seen_autoindex)
				throw ParseError("duplicate 'autoindex' in location block", peek());
			if (!match(Identifier))
				throw ParseError("expected 'on' or 'off' after autoindex", peek());
			std::string v = consume().value;
			if (v == "on") loc.autoindex = true;
			else if ( v == "off") loc.autoindex = false;
			else throw ParseError("autoindex must be 'on' or 'off'", peek());
			expectSemicolon();
			seen_autoindex = true;
		}
		else if (name == "root"){
			if (seen_root)
				throw ParseError("duplicate 'root' in location block", peek());
			if (!(match(Identifier) || match(String)))
				throw ParseError("expected path after root", peek());
			std::string path = consume().value;
			if (path.empty())
				throw ParseError("root path cannot be empty", peek());
			if (path.size() > 1 && path[path.size() - 1] == '/')
				throw ParseError("path root ends with /", peek());
			loc.root = path;
			expectSemicolon();
			seen_root = true;
		}
		else if (name == "index"){
			if (seen_index)
				throw ParseError("duplicate 'index' in location block", peek());
			if (!(match(Identifier) || match(String)))
				throw ParseError("expected path after index", peek());
			loc.index = consume().value;
			expectSemicolon();
			seen_index = true;
		}
		else if (name == "client_max_body_size"){
			if (seen_client_max_body_size)
				throw ParseError("duplicate 'client_max_body_size' in location block", peek());
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
			loc.client_max_body_size = n;
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
			if (!(match(Identifier) || match(String)))
				throw ParseError("expected path after error_page <code>", peek());
			std::string path = consume().value;
			if (path.empty())
				throw ParseError("error_page path cannot be empty", peek());
			if (loc.error_pages.find(code) != loc.error_pages.end())
				throw ParseError("duplicate error_page code in location block", peek());
			loc.error_pages[code] = path;
			expectSemicolon();
		}
		else if (name == "upload"){
			if (seen_upload)
				throw ParseError("duplicate 'upload' in location block", peek());
			if (!match(Identifier))
				throw ParseError("expected 'on' or 'off' after upload", peek());
			std::string v = consume().value;
			if (v == "on") loc.upload_enabled = true;
			else if ( v == "off") loc.upload_enabled = false;
			else throw ParseError("upload must be 'on' or 'off'", peek());
			expectSemicolon();
			seen_upload = true;
		}
		else if (name == "upload_store"){
			if (seen_upload_store)
				throw ParseError("duplicate 'upload_store' in location block", peek());
			if (!(match(Identifier) || match(String)))
				throw ParseError("expected path after upload_store", peek());
			loc.upload_store = consume().value;
			if (loc.upload_store.empty())
				throw ParseError("upload_store path cannot be empty", peek());
			expectSemicolon();
			seen_upload_store = true;
		}
		else if (name == "return"){
			if (seen_return)
				throw ParseError("duplicate 'return' in location block", peek());
			if (!match(Number))
				throw ParseError("expected redirect code after return", peek());
			bool ok = false;
			long codeL = parseLongStrict(peek().value, ok);
			if (!ok)
				throw ParseError("invalid redirect code in return", peek());
			if (codeL < 300 || codeL > 599)
				throw ParseError("status code out of range in return", peek());
			consume();
			if (match(Semicolon)){
				loc.redirect_code = static_cast<int>(codeL);
				loc.has_redirect = true;
				expectSemicolon();
				seen_return = true;
				continue;
			}
			if (!(match(Identifier) || match(String)))
				throw ParseError("expected path after redirect code", peek());
			loc.redirect_target = consume().value;
			if (loc.redirect_target.empty())
				throw ParseError("return redirect target cannot be empty", peek());
			loc.redirect_code = static_cast<int>(codeL);
			loc.has_redirect = true;
			expectSemicolon();
			seen_return = true;
		}
		else if (name == "cgi"){
			if (!(match(Identifier) || match(String)))
				throw ParseError("expected cgi extension after cgi", peek());
			std::string ext = consume().value;
			if (ext.empty())
				throw ParseError("cgi extension cannot be empty", peek());
			if (ext[0] != '.')
				throw ParseError("cgi extension must start with '.'", peek());
			if (!(match(Identifier) || match(String)))
				throw ParseError("expected path after cgi extension", peek());
			if (loc.cgi.count(ext))
				throw ParseError("duplicate cgi extension in location block", peek());
			std::string bin = consume().value;
			if (bin.empty())
				throw ParseError("cgi path cannot be empty", peek());
			loc.cgi[ext] = bin;
			expectSemicolon();
		}
		else{
			throw ParseError(std::string("unknown directive in location: ") + name, peek());
		}
	}
	expect(RBrace, "expected '}' to close location block");
	return (loc);
}
