#pragma once
#include <string>
#include <vector>
#include <map>
#include "Location.hpp"

class ServerConfig
{
	private:
		std::string ip;
		int port;
		std::string root;
		size_t maxBodySize;
		std::map<int, std::string> errorPages;
		std::vector<Location> locations;

	public:
		ServerConfig();

		// Getters
		const std::string& getIp() const;
		int getPort() const;
		const std::string& getRoot() const;
		size_t getMaxBodySize() const;
		const std::map<int, std::string>& getErrorPages() const;
		const std::vector<Location>& getLocations() const;

		// Setters
		void setIp(const std::string& ip_);
		void setPort(int p);
		void setRoot(const std::string& r);
		void setMaxBodySize(size_t size);
		void addErrorPage(int code, const std::string& path);
		void addLocation(const Location& loc);

		// Helper
		const Location* findLocation(const std::string& uri) const;
};
