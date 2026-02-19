#ifndef TOKEN_HPP
# define TOKEN_HPP

# include <string>
# include <vector>

enum			TokenType
{
	Identifier,
	String,
	Number,
	LBrace,
	RBrace,
	Semicolon,
	End,
};

struct			Token
{
	int			col;
	int			line;
	TokenType	type;
	std::string value;
};

std::vector<Token> tokenize(std::string const &text);

#endif