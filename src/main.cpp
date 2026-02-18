/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mprazere <mprazere@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/17 12:18:13 by praders           #+#    #+#             */
/*   Updated: 2026/02/18 16:01:58 by mprazere         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/inc_conf_pars/Config.hpp"
#include "../inc/inc_server/Webserv.hpp"

int main(int argc, char **argv){
	(void)argc;
	try{
		Config cfg = createConfig(argv[1]);
		Webserv wbs(cfg);
		wbs.run();
	}
	catch(std::exception &e){
		std::cout << "Error: " << e.what() << std::endl;
	}
	return (0);
}