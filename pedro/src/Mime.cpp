#include "../inc/Mime.hpp"

std::map<std::string, std::string> Mime::createExtToMime()
{
    std::map<std::string, std::string> m;

    m["html"] = "text/html";
    m["htm"]  = "text/html";
    m["txt"]  = "text/plain";
    m["json"] = "application/json";
    m["xml"]  = "application/xml";

    m["png"]  = "image/png";
    m["jpg"]  = "image/jpeg";
    m["jpeg"] = "image/jpeg";
    m["gif"]  = "image/gif";
    m["pdf"]  = "application/pdf";
    m["py"]   = "text/x-python";

    return m;
}

std::map<std::string, std::string> Mime::createMimeToExt()
{
    std::map<std::string, std::string> m;

    m["text/plain"] = ".txt";
    m["text/html"] = ".html";
    m["application/json"] = ".json";
    m["application/xml"] = ".xml";
    m["text/xml"] = ".xml";

    m["text/x-python"] = ".py";
    m["application/x-python-code"] = ".py";
    m["text/x-script.python"] = ".py";

    m["image/png"] = ".png";
    m["image/jpeg"] = ".jpg";
    m["image/gif"] = ".gif";
    m["application/pdf"] = ".pdf";

    return m;
}

std::string Mime::getType(const std::string& extension)
{
    static std::map<std::string, std::string> extToMime = createExtToMime();

    std::string ext = extension;
    if (!ext.empty() && ext[0] == '.')
        ext = ext.substr(1);

    std::map<std::string, std::string>::iterator it = extToMime.find(ext);

    if (it != extToMime.end())
        return it->second;

	//can be binary
    return "application/octet-stream";
}

std::string Mime::getExtension(const std::string& contentType)
{
    static std::map<std::string, std::string> mimeToExt = createMimeToExt();

    std::string type = contentType;

    // Remove charset
    size_t semicolon = type.find(';');
    if (semicolon != std::string::npos)
        type = type.substr(0, semicolon);

    std::map<std::string, std::string>::iterator it = mimeToExt.find(type);

    if (it != mimeToExt.end())
        return it->second;

    return ".txt";
}
