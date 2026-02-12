/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseConfig.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: praders <praders@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/10 14:46:34 by mprazere          #+#    #+#             */
/*   Updated: 2026/02/12 15:46:57 by praders          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../InConFile/Config.hpp"
#include "../InConFile/Token.hpp"
#include "../InConFile/Parser.hpp"
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
		Parser p(tokens);
		Config server;
		server = p.parseconfig();
		std::cout << "Parsed servers: " << server.servers.size() << "\n";
        for (size_t i = 0; i < server.servers.size(); ++i)
        {
            std::cout << "---- server[" << i << "] ----\n";
            std::cout << "host: " << server.servers[i].host << "\n";
            std::cout << "port: " << server.servers[i].port << "\n";
            std::cout << "root: " << server.servers[i].root << "\n";
            std::cout << "index: " << server.servers[i].index << "\n";
            std::cout << "client_max_body_size: " << server.servers[i].client_max_body_size << "\n";
            std::cout << "error_pages: " << server.servers[i].error_pages.size() << "\n";
            std::cout << "locations: " << server.servers[i].locations.size() << "\n";
        }
		
	}
	catch(std::exception &e){
		std::cout << "Error: " << e.what() << std::endl;
	}
	return (0);
}