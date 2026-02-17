/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParseConfig.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: praders <praders@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/10 14:46:34 by mprazere          #+#    #+#             */
/*   Updated: 2026/02/17 12:18:00 by praders          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/inc_conf_pars/Config.hpp"
#include "../../inc/inc_conf_pars/Token.hpp"
#include "../../inc/inc_conf_pars/Parser.hpp"
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
