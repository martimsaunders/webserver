
//Main Server Logic
/* 
Startup:
    Parse config file
    Create ServerConfig objects
    Create ServerSocket per IP:Port

Runtime:
    poll()

When client connects:
    Accept()
    Attach correct ServerConfig based on listening socket

When request fully parsed:
    RequestHandler::handleRequest(request, serverConfig)
        ↓
    Returns HttpResponse
        ↓
    HTTP layer serializes
        ↓
    Network layer sends 
*/


//Application Layer logic
/*
When request fully parsed:
    RequestHandler::handleRequest(request, serverConfig)
        │
        ▼
    Match Location block
        │
        ├── If no matching Location → ErrorResponseBuilder
        ▼
    Validate HTTP method
        │
        ├── If method not allowed → ErrorResponseBuilder
        ▼
    Resolve filesystem path
        │
        ├── If redirect configured → build redirect response
        ▼
    Dispatch to correct method handler
        │
        ├── GET  → FileService read file / directory / autoindex
        ├── POST → FileService upload file OR CGIHandler if CGI
        └── DELETE → FileService delete file
        │
        ▼
    If CGI required → CGIHandler executes script
        │
        ▼
    Check for errors / max body size / forbidden access
        │
        └── If error → ErrorResponseBuilder builds error response
        ▼
    Build HttpResponse object
        │
        ▼
    Return HttpResponse to HTTP layer
*/


// Clean architecture
/*
HttpRequest        <- just data (method, uri, headers, body)
Location           <- knows CGI extensions, root, methods allowed
ServerConfig       <- owns locations
RequestHandler     <- orchestrates logic, queries Location
CGIHandler         <- executes CGI scripts
HttpResponse       <- structured response container
*/

int main (void)
{
	
}