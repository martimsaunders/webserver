/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseConfig.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mprazere <mprazere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/10 14:46:34 by mprazere          #+#    #+#             */
/*   Updated: 2026/02/10 16:18:09 by mprazere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "InConFile/Config.hpp"
#include "InConFile/Token.hpp"

void skip_ws(std::string const &text, size_t &i){
	while (i  < text.size() && ((text[i] >= 9 && text[i] <= 13) || text[i] == ' '))
		i++;
}

std::vector<Token> tokenize(std::string const &text){
	std::vector<Token> tokens;

	for (size_t i = 0; i < text.size(); i++){
		skip_ws(text, i);
		if (i >= text.size()) break;
		char c = text[i];
		
	}
	return (tokens);
}

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

int main(int argc, char **argv){
	(void)argc;
	try{
		std::string text = readFile(argv[1]);
		std::vector<Token> tokens = tokenize(text);
		Config cfg = parseConfig(tokens);
	}
	catch(std::exception &e){
		std::cout << "Error: " << e.what() << std::endl;
	}
	return (0);
}