/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationConfig.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mprazere <mprazere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/10 15:39:00 by mprazere          #+#    #+#             */
/*   Updated: 2026/02/11 16:13:46 by mprazere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef Location_Config_HPP
# define Location_Config_HPP

# include <iostream>
# include <map>
# include <vector>

struct		LocationConfig
{
	int		redirect_code;
	bool	allow_get;
	bool	autoindex;
	bool	allow_post;
	bool	allow_delete;
	bool	has_redirect;
	bool	upload_enabled;
	std::string path;
	std::string root;
	std::string index;
	std::string upload_store;
	std::string redirect_target;
	std::map<std::string, std::string> cgi;
};

#endif