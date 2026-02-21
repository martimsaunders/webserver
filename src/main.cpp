#include "../inc/config/Config.hpp"
#include "../inc/server/Webserv.hpp"

int main(int argc, char **argv){
	if (argc != 2){
		std::cerr << "./webserv <config_file>" << std::endl;
		return (1);
	}
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