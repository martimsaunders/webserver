#include "../../inc/config/Config.hpp"
#include "../../inc/config/Token.hpp"
#include "../../inc/config/Parser.hpp"
#include <cctype>
#include <stdexcept>

static std::string readFile(std::string const &path){
	std::ifstream infile(path.c_str());
	if (!infile.is_open())
		throw std::runtime_error("Could not open config file");
	if (path.size() < 5 || path.substr(path.size() - 5) != ".conf")
		throw std::runtime_error("Config file must end with .conf");
	std::string content;
	std::string line;
	while (std::getline(infile, line))
		content += line + "\n";
	return (content);
}

Config createConfig(std::string const &path){
		std::string text = readFile(path);
		std::vector<Token> tokens = tokenize(text);
		//printTok(tokens);
		Parser p(tokens);
		Config cfg;
		cfg = p.parseconfig();
		validateConfig(cfg);
		debugPrintConfig(cfg);
		return (cfg);
}
