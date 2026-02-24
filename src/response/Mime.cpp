#include "../../inc/response/Mime.hpp"

std::map<std::string, std::string> Mime::createExtToMime()
{
    std::map<std::string, std::string> m;

    m["html"] = "text/html";
    m["htm"]  = "text/html";
    m["txt"]  = "text/plain";
    m["csv"]  = "text/csv";
    m["md"]   = "text/markdown";
    m["css"]  = "text/css";
    m["js"]   = "application/javascript";
    m["mjs"]  = "application/javascript";
    m["json"] = "application/json";
    m["xml"]  = "application/xml";
    m["wasm"] = "application/wasm";
    m["png"]  = "image/png";
    m["jpg"]  = "image/jpeg";
    m["jpeg"] = "image/jpeg";
    m["gif"]  = "image/gif";
    m["webp"] = "image/webp";
    m["svg"]  = "image/svg+xml";
    m["ico"]  = "image/x-icon";
    m["bmp"]  = "image/bmp";
    m["tif"]  = "image/tiff";
    m["tiff"] = "image/tiff";
    m["pdf"]  = "application/pdf";
    m["py"]   = "text/x-python";
    m["zip"]  = "application/zip";
    m["tar"]  = "application/x-tar";
    m["gz"]   = "application/gzip";
    m["bin"]  = "application/octet-stream";
    m["mp3"]  = "audio/mpeg";
    m["wav"]  = "audio/wav";
    m["ogg"]  = "audio/ogg";
    m["mp4"]  = "video/mp4";
    m["webm"] = "video/webm";
    m["avi"]  = "video/x-msvideo";
    m["mov"]  = "video/quicktime";

    return m;
}

std::map<std::string, std::string> Mime::createMimeToExt()
{
    std::map<std::string, std::string> m;

    m["text/plain"] = ".txt";
    m["text/html"] = ".html";
    m["text/csv"] = ".csv";
    m["text/markdown"] = ".md";
    m["text/css"] = ".css";
    m["application/javascript"] = ".js";
    m["text/javascript"] = ".js";
    m["application/json"] = ".json";
    m["application/xml"] = ".xml";
    m["text/xml"] = ".xml";
    m["application/wasm"] = ".wasm";

    m["text/x-python"] = ".py";
    m["application/x-python-code"] = ".py";
    m["text/x-script.python"] = ".py";

    m["image/png"] = ".png";
    m["image/jpeg"] = ".jpg";
    m["image/gif"] = ".gif";
    m["image/webp"] = ".webp";
    m["image/svg+xml"] = ".svg";
    m["image/x-icon"] = ".ico";
    m["image/bmp"] = ".bmp";
    m["image/tiff"] = ".tiff";
    m["application/pdf"] = ".pdf";
    m["application/zip"] = ".zip";
    m["application/x-tar"] = ".tar";
    m["application/gzip"] = ".gz";
    m["application/octet-stream"] = ".bin";
    m["audio/mpeg"] = ".mp3";
    m["audio/wav"] = ".wav";
    m["audio/ogg"] = ".ogg";
    m["video/mp4"] = ".mp4";
    m["video/webm"] = ".webm";
    m["video/x-msvideo"] = ".avi";
    m["video/quicktime"] = ".mov";

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

    return "";
}
