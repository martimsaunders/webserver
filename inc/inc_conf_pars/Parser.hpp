/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: praders <praders@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/12 12:58:55 by praders           #+#    #+#             */
/*   Updated: 2026/02/17 12:48:34 by praders          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
# define PARSER_HPP

# include "Config.hpp"
# include "LocationConfig.hpp"
# include "ServerConfig.hpp"
# include "Token.hpp"
# include <stdexcept>
# include <string>
# include <vector>

struct ServerConfig;
struct LocationConfig;
struct Config;

class ParseError : public std::runtime_error
{
  public:
	int line;
	int col;
	virtual ~ParseError(void) throw();
	ParseError(void);
	ParseError(ParseError const &other);
	ParseError &operator=(ParseError const &other);
	ParseError(std::string const &msg, Token const &at);

  private:
	static std::string format(std::string const &msg, Token const &at);
};

class Parser
{
	public:
		Parser(void);
		Parser(Parser const &other);
		Parser &operator=(Parser const &other);
		~Parser(void);
		explicit Parser(std::vector<Token> const &Tokens);
		Config parseconfig(void);
		LocationConfig parselocation(std::string const &path, ServerConfig const &srv);

	private:
		std::vector<Token> const *_token;
		size_t _pos;

	private:
		bool match(TokenType type) const;
		bool matchIdent(std::string const &kw) const;
		void expectSemicolon(void);
		Token const &consume(void);
		Token const &expectIdent(char const *msg);
		Token const &peek(size_t offset = 0) const;
		Token const &expect(TokenType type, char const *msg);
		ServerConfig parseserver();
		
};

#endif