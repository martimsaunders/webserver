#ifndef CONFIG_HPP
# define CONFIG_HPP

# include "ServerConfig.hpp"
# include "Token.hpp"
# include <fstream>
# include <iostream>
# include <string>
# include <vector>

struct	Config
{
	std::vector<ServerConfig> servers;
};

void	validateConfig(Config const &cfg);
void	debugPrintConfig(const Config &cfg);
void	printTok(std::vector<Token> tokens);
Config	createConfig(std::string const &path);

#endif