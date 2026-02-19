#include "../../inc/inc_conf_pars/Config.hpp"
#include <set>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>

static void validateDuplicateSockets(Config const &cfg)
{
	std::set< std::pair<std::string, int> > used;
	for (size_t i = 0; i < cfg.servers.size(); ++i){
		ServerConfig const &srv = cfg.servers[i];
		std::pair<std::string,int> key(srv.host, srv.port);
		if (!used.insert(key).second){
			std::ostringstream oss;
			oss << "Duplicate host:port in config: " << srv.host << ":" << srv.port;
			throw std::runtime_error(oss.str());
		}
	}
}

static bool isDirectory(std::string const &path){
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return (false);
	return (S_ISDIR(st.st_mode));
}

static bool isWritable(std::string const &path){
	return (access(path.c_str(), W_OK) == 0);
}

static void validatePaths(Config const &cfg){
	for (size_t i = 0; i < cfg.servers.size(); i++){
		ServerConfig const &srv = cfg.servers[i];
		if (!srv.root.empty()){
			if (!isDirectory(srv.root)){
				std::ostringstream oss;
				oss << "Invalid server root directory: " << srv.root;
				throw std::runtime_error(oss.str());
			}
		}
		for (size_t j = 0; j < srv.locations.size(); j++){
			LocationConfig const &loc = srv.locations[j];
			if (!loc.root.empty()){
				if (!isDirectory(loc.root)){
					std::ostringstream oss;
					oss << "Invalid location root directory: " << loc.root << " (location: " << loc.path << ")";
					throw std::runtime_error(oss.str());
				}
			}
			if (loc.upload_enabled){
				if (loc.upload_store.empty()){
					std::ostringstream oss;
					oss << "upload is enabled but upload_store is not defined" << " (location: " << loc.path << ")";
					throw std::runtime_error(oss.str());
				}
				if (!isDirectory(loc.upload_store)){
					std::ostringstream oss;
					oss << "Invalid upload_store directory: " << loc.upload_store << " (location: " << loc.path << ")";
					throw std::runtime_error(oss.str());
				}
				if (!isWritable(loc.upload_store)){
					std::ostringstream oss;
					oss << "upload_store is not writable: " << loc.upload_store << " (location: " << loc.path << ")";
					throw std::runtime_error(oss.str());
				}
			}
		}
	}
}

void validateConfig(Config const &cfg){
	validateDuplicateSockets(cfg);
	validatePaths(cfg);
}