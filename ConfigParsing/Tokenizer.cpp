/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Tokenizer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: praders <praders@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/11 13:08:50 by mprazere          #+#    #+#             */
/*   Updated: 2026/02/12 15:47:10 by praders          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../InConFile/Config.hpp"
#include "../InConFile/Token.hpp"
#include <cctype>

static void skip(std::string const &text, size_t &i, int &line, int &col){
	if (i >= text.size()) return;
	if (text[i] == '\n'){ line++; col = 1; }
	else col++;
	i++;
}

void skipWs(std::string const &text, size_t &i, int &line, int &col){
	while (i  < text.size() && std::isspace(static_cast<unsigned char>(text[i])))
		skip(text, i, line, col);
}

void skipComment(std::string const &text, size_t &i, int &line, int &col){
	while (i < text.size() && text[i] != '\n')
		skip(text, i, line, col);
	if (i < text.size() && text[i] == '\n')
		skip(text, i, line, col);
}

static bool isDelimeter(char c){
	return (std::isspace(static_cast<unsigned char>(c)) || c == '{' || c == '}' || c == ';' || c == '#');
}

static bool isAllDigits(std::string const &str){
	if (str.empty()) return (false);
	for (size_t i = 0; i < str.size(); i++)
		if (!std::isdigit(static_cast<unsigned char>(str[i])))
			return (false);
	return (true);
}

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

std::vector<Token> tokenize(std::string const &text){
	std::vector<Token> tokens;
	size_t i = 0;
	int line = 1;
	int col = 1;
	while (i < text.size()){
		skipWs(text, i, line, col);
		if (i >= text.size()) break;
		if (text[i] == '#'){
			skipComment(text, i, line, col);
			continue;
		}
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
		int const tokLine = line;
		int const tokCol = col;
		std::string word;
		while (i < text.size() && !isDelimeter(text[i])){
			word += text[i];
			skip(text, i, line, col);
		}
		Token tokWord;
		tokWord.line = tokLine;
		tokWord.col = tokCol;
		tokWord.value = word;
		if (isAllDigits(word)) tokWord.type = Number;
		else if (isIdentifier(word)) tokWord.type = Identifier;
		else tokWord.type = String;
		tokens.push_back(tokWord);
	}
	Token tokEnd;
	tokEnd.line = line;
	tokEnd.col = col;
	tokEnd.value = "";
	tokEnd.type = End;
	tokens.push_back(tokEnd);
	return (tokens);
}
