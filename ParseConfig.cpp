/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseConfig.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mprazere <mprazere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/10 14:46:34 by mprazere          #+#    #+#             */
/*   Updated: 2026/02/11 15:41:17 by mprazere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "InConFile/Config.hpp"
#include "InConFile/Token.hpp"
#include <cctype>
#include <stdexcept>

std::string readFile(std::string const &path){
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

Config parseConfig(std::vector<Token> const &tokens){
	Config cfg;
	
	return (cfg);
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

int main(int argc, char **argv){
	(void)argc;
	try{
		std::string text = readFile(argv[1]);
		std::vector<Token> tokens = tokenize(text);
		//printTok(tokens);
		Config cfg = parseConfig(tokens);
	}
	catch(std::exception &e){
		std::cout << "Error: " << e.what() << std::endl;
	}
	return (0);
}