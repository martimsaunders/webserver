/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: praders <praders@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/17 12:18:13 by praders           #+#    #+#             */
/*   Updated: 2026/02/17 17:11:51 by praders          ###   ########.fr       */
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