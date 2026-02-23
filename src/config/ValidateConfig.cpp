#include "../../inc/config/Config.hpp"
#include <set>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>

//========================================================================
// validateDuplicateSockets
//========================================================================
// Ensures there are no two "server" blocks trying to use the same socket.
// Each (host, port) pair must be unique to avoid bind() conflicts.
static void validateDuplicateSockets(Config const &cfg)
{
	std::set< std::pair<std::string, int> > used;

	for (size_t i = 0; i < cfg.servers.size(); ++i){
		ServerConfig const &srv = cfg.servers[i];
		std::pair<std::string,int> key(srv.host, srv.port);

		// insert() returns a pair; .second is true if inserted, false if it already existed.
		if (!used.insert(key).second){
			std::ostringstream oss;
			oss << "Duplicate host:port in config: " << srv.host << ":" << srv.port;
			throw std::runtime_error(oss.str());
		}
	}
}

//========================================================================
// Filesystem helpers
//========================================================================
// Checks if the path exists and is a directory.
static bool isDirectory(std::string const &path){
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return (false);
	return (S_ISDIR(st.st_mode));
}

// Checks if the process has write permissions on the path.
// (W_OK = writable)
static bool isWritable(std::string const &path){
	return (access(path.c_str(), W_OK) == 0);
}

//========================================================================
// validatePaths
//========================================================================
// Validates paths defined in the config:
// - server.root must exist and be a directory
// - location.root must exist and be a directory
// - if uploads are enabled:
//     - upload_store must be defined
//     - upload_store must exist and be a directory
//     - upload_store must be writable
static void validatePaths(Config const &cfg){
	for (size_t i = 0; i < cfg.servers.size(); i++){
		ServerConfig const &srv = cfg.servers[i];

		// Validate server root (if configured)
		if (!srv.root.empty()){
			if (!isDirectory(srv.root)){
				std::ostringstream oss;
				oss << "Invalid server root directory: " << srv.root;
				throw std::runtime_error(oss.str());
			}
		}

		// Validate each location in the server
		for (size_t j = 0; j < srv.locations.size(); j++){
			LocationConfig const &loc = srv.locations[j];

			// Validate location root (if configured)
			if (!loc.root.empty()){
				if (!isDirectory(loc.root)){
					std::ostringstream oss;
					oss << "Invalid location root directory: " << loc.root << " (location: " << loc.path << ")";
					throw std::runtime_error(oss.str());
				}
			}

			// If upload is enabled, we need a valid directory with write permissions
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

//========================================================================
// validateConfig
//========================================================================
// Global configuration validation.
// Combines "structural" validations (duplicate sockets) and "semantic" validations (paths).
void validateConfig(Config const &cfg){
	validateDuplicateSockets(cfg);
	validatePaths(cfg);
}