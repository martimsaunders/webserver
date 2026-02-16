/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Socket.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mprazere <mprazere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/16 11:01:06 by mprazere          #+#    #+#             */
/*   Updated: 2026/02/16 16:52:20 by mprazere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SOCKET_HPP
# define SOCKET_HPP

# include <iostream>

struct		Client
{
	int		fd;
	int		listen_fd;
	bool	want_write;
	bool	should_close;
	bool	header_parsed;
	size_t	expected_body;
	std::string in;
	std::string out;
};

struct		ListenSocket
{
	int		fd;
	int		port;
	size_t	server_index;
	std::string host;
};

#endif