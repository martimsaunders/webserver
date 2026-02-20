#include "../inc/config/Config.hpp"
#include "../inc/server/Webserv.hpp"

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