/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mprazere <mprazere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/10 15:38:57 by mprazere          #+#    #+#             */
/*   Updated: 2026/02/16 11:35:30 by mprazere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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