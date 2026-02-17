#pragma once
#include <string>
#include <vector>

class Location
{
	private:
		std::string path;                 
		std::string root;                 
		std::vector<std::string> allowedMethods;
		bool autoindex;
		std::string indexFile;
		bool uploadEnabled;
		std::string uploadPath;
		std::string redirect;
		std::string cgiExtension;
		std::string cgiPath;

	public:
		Location();

		//Getters
		const std::string& getPath() const;
		const std::string& getRoot() const;
		const std::vector<std::string>& getAllowedMethods() const;
		bool getAutoindex() const;
		const std::string& getIndexFile() const;
		bool getUploadEnabled() const;
		const std::string& getUploadPath() const;
		const std::string& getRedirect() const;
		const std::string& getCgiExtension() const;
		const std::string& getCgiPath() const;

		// Setters
		void setPath(const std::string& p);
		void setRoot(const std::string& r);
		void setAllowedMethods(const std::vector<std::string>& methods);
		void addAllowedMethod(const std::string& method);
		void setAutoindex(bool enabled);
		void setIndexFile(const std::string& file);
		void setUploadEnabled(bool enabled);
		void setUploadPath(const std::string& path);
		void setRedirect(const std::string& url);
		void setCgiExtension(const std::string& ext);
		void setCgiPath(const std::string& path);

		//Helper methods
		bool isMethodAllowed(const std::string& method) const;
		std::string resolvePath(const std::string& uri) const;
		bool needsRedirect() const;
		bool isCgiRequest(const std::string& uri) const;
		bool matchesCgi(const std::string& extension) const;
};
