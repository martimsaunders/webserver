/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseConfig.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: martimprazeresaunders <martimprazeresau    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/10 14:46:34 by mprazere          #+#    #+#             */
/*   Updated: 2026/02/13 15:33:13 by martimpraze      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../InConFile/Config.hpp"
#include "../InConFile/Token.hpp"
#include "../InConFile/Parser.hpp"
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

int main(int argc, char **argv){
	(void)argc;
	try{
		Config cfg = createConfig(argv[1]);
	}
	catch(std::exception &e){
		std::cout << "Error: " << e.what() << std::endl;
	}
	return (0);
}