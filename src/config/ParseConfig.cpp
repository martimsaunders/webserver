#include "../../inc/config/Config.hpp"
#include "../../inc/config/Token.hpp"
#include "../../inc/config/Parser.hpp"
#include <cctype>
#include <stdexcept>

//========================================================================
// readFile
//========================================================================
// Reads a configuration file into a string.
// Validates that the path ends with ".conf" and throws on failure.
static std::string readFile(std::string const &path){
	std::ifstream infile(path.c_str());

	// I/O failure: file does not exist / no permissions / etc.
	if (!infile.is_open())
		throw std::runtime_error("Could not open config file");

	// Project rule: only accept files with .conf extension
	if (path.size() < 5 || path.substr(path.size() - 5) != ".conf")
		throw std::runtime_error("Config file must end with .conf");

	// Read line by line and rebuild the full text with '\n'
	// (we keep line breaks to help line/col reporting in the parser)
	std::string content;
	std::string line;
	while (std::getline(infile, line))
		content += line + "\n";

	return (content);
}

//========================================================================
// createConfig
//========================================================================
// Full pipeline to build a Config:
// - reads the file (.conf) into text
// - tokenizes (lexer) into a token vector
// - parses (parser) into a Config structure
// - validates semantic rules (required fields, ranges, conflicts, etc.)
// - returns a ready-to-use Config
// Note: any error throws an exception.
Config createConfig(std::string const &path){
		std::string text = readFile(path);

		// Lexer: transforms the text into tokens (keywords, numbers, '{', '}', ';', etc.)
		std::vector<Token> tokens = tokenize(text);

		// Optional debug: print generated tokens
		//printTok(tokens);

		// Parser: consumes tokens and builds the Config structure
		Parser p(tokens);
		Config cfg;
		cfg = p.parseconfig();

		// Semantic validation: ensures global consistency of the config
		validateConfig(cfg);

		// Optional debug: print final config structure
		//debugPrintConfig(cfg);

		return (cfg);
}
