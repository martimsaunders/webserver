#include "../inc/config/Config.hpp"
#include "../inc/server/Webserv.hpp"

int main(int argc, char **argv){
	if (argc > 2){
		std::cerr << "./webserv <config_file>" << std::endl;
		return (1);
	}
	std::string config_path;
	if (argc == 1) config_path = "configs/full_test.conf";
	else config_path = argv[1];
	try{
		Config cfg = createConfig(config_path);
		Webserv wbs(cfg);
		wbs.run();
	}
	catch(std::exception &e){
		std::cout << "Error: " << e.what() << std::endl;
	}
	return (0);
}