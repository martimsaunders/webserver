/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationConfig.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: martimprazeresaunders <martimprazeresau    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/10 15:39:00 by mprazere          #+#    #+#             */
/*   Updated: 2026/02/13 09:40:11 by martimpraze      ###   ########.fr       */
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
	size_t	client_max_body_size;
	std::string path;
	std::string root;
	std::string index;
	std::string upload_store;
	std::string redirect_target;
	std::map<int, std::string> error_pages;
	std::map<std::string, std::string> cgi;
};

#endif