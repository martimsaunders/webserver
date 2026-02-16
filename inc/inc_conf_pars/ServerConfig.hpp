/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mprazere <mprazere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/10 15:39:03 by mprazere          #+#    #+#             */
/*   Updated: 2026/02/11 15:47:44 by mprazere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_CONFIG_HPP
# define SERVER_CONFIG_HPP

# include "LocationConfig.hpp"
# include <iostream>
# include <map>
# include <vector>

struct		ServerConfig
{
	int		port;
	size_t	client_max_body_size;
	std::string host;
	std::string root;
	std::string index;
	std::map<int, std::string> error_pages;
	std::vector<LocationConfig> locations;
};

#endif