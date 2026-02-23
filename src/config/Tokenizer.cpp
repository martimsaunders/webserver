#include "../../inc/config/Config.hpp"
#include "../../inc/config/Token.hpp"
#include <cctype>

//========================================================================
// Text reading helpers (line/col tracking)
//========================================================================
// Advances one character in the text and updates:
// - i: index in the text
// - line/col: position for error reporting (parser/lexer)
static void skip(std::string const &text, size_t &i, int &line, int &col){
	if (i >= text.size()) return;
	if (text[i] == '\n'){ line++; col = 1; }
	else col++;
	i++;
}

// Skip whitespace (spaces, tabs, newlines, etc.)
void skipWs(std::string const &text, size_t &i, int &line, int &col){
	while (i  < text.size() && std::isspace(static_cast<unsigned char>(text[i])))
		skip(text, i, line, col);
}

// Skip a comment: everything from '#' until end of line
void skipComment(std::string const &text, size_t &i, int &line, int &col){
	while (i < text.size() && text[i] != '\n')
		skip(text, i, line, col);
	if (i < text.size() && text[i] == '\n')
		skip(text, i, line, col);
}

//========================================================================
// Character / word classification
//========================================================================
// Delimiters are characters that separate tokens:
// whitespace, '{', '}', ';' and '#' (start of comment)
static bool isDelimeter(char c){
	return (std::isspace(static_cast<unsigned char>(c)) || c == '{' || c == '}' || c == ';' || c == '#');
}

// Checks if a string contains only digits (e.g. "8080")
static bool isAllDigits(std::string const &str){
	if (str.empty()) return (false);
	for (size_t i = 0; i < str.size(); i++)
		if (!std::isdigit(static_cast<unsigned char>(str[i])))
			return (false);
	return (true);
}

// Checks if a string is a valid identifier:
// - starts with a letter or '_'
// - remaining chars can be letter, digit or '_'
static bool isIdentifier(std::string const &str){
	if (str.empty()) return (false);
	if (!std::isalpha(static_cast<unsigned char>(str[0])) && static_cast<unsigned char>(str[0]) != '_')
		return (false);
	for (size_t i = 1; i < str.size(); i++){
		unsigned char c = static_cast<unsigned char>(str[i]);
		if (!std::isdigit(c) && !std::isalpha(c) && c != '_')
			return (false);
	}
	return (true);
}

//========================================================================
// tokenize
//========================================================================
// Converts the config file text into a vector of tokens.
// Supported tokens:
// - Delimiters: '{', '}', ';'
// - Comments: '# ... \n' (ignored)
// - Words: classified as Number / Identifier / String
// Always appends an End token at the end so the parser can detect EOF.
std::vector<Token> tokenize(std::string const &text){
	std::vector<Token> tokens;
	size_t i = 0;
	int line = 1;
	int col = 1;

	while (i < text.size()){
		// Skip whitespace
		skipWs(text, i, line, col);
		if (i >= text.size()) break;

		// Skip comments
		if (text[i] == '#'){
			skipComment(text, i, line, col);
			continue;
		}

		// Single-character tokens: '{', '}', ';'
		if (text[i] == '{' || text[i] == '}' || text[i] == ';'){
			Token tokDel;
			tokDel.line = line;
			tokDel.col = col;
			tokDel.value = std::string(1, text[i]);

			if (text[i] == '{') tokDel.type = LBrace;
			else if (text[i] == '}') tokDel.type = RBrace;
			else tokDel.type = Semicolon;

			tokens.push_back(tokDel);
			skip(text, i, line, col);
			continue;
		}

		// "Word" token: read until a delimiter is found
		int const tokLine = line;
		int const tokCol = col;
		std::string word;

		while (i < text.size() && !isDelimeter(text[i])){
			word += text[i];
			skip(text, i, line, col);
		}

		// Token classification:
		// - only digits -> Number
		// - valid identifier -> Identifier
		// - otherwise -> String (paths, IPs, extensions, etc.)
		Token tokWord;
		tokWord.line = tokLine;
		tokWord.col = tokCol;
		tokWord.value = word;

		if (isAllDigits(word)) tokWord.type = Number;
		else if (isIdentifier(word)) tokWord.type = Identifier;
		else tokWord.type = String;

		tokens.push_back(tokWord);
	}

	// End token: ensures peek() can always return End at EOF
	Token tokEnd;
	tokEnd.line = line;
	tokEnd.col = col;
	tokEnd.value = "";
	tokEnd.type = End;
	tokens.push_back(tokEnd);

	return (tokens);
}
