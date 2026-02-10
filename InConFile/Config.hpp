/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mprazere <mprazere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/10 15:38:57 by mprazere          #+#    #+#             */
/*   Updated: 2026/02/10 15:38:57 by mprazere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
# define CONFIG_HPP

# include "ServerConfig.hpp"
# include <fstream>
# include <iostream>
# include <string>
# include <vector>

struct	Config
{
	std::vector<ServerConfig> servers;
};

#endif