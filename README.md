*This project has been created as part of the 42 curriculum by mateferr, mprazere, psantos-.*

## Description
`webserv` is a custom HTTP server developed in C++98 for the 42 curriculum.

### Project goal
The objective is to implement a resilient, non-blocking, event-driven server that can handle multiple clients concurrently while supporting a practical subset of HTTP behavior (routing, static files, uploads, deletes, redirects, and CGI).

### Key definitions
- `HTTP request`: message sent by a client (method, target URI, version, headers, optional body).
- `HTTP response`: message returned by the server (status line, headers, optional body).
- `route/location`: configuration block that defines behavior for a URI prefix (allowed methods, root, index, autoindex, upload, redirect, CGI).
- `non-blocking I/O`: sockets and IPC descriptors are used without blocking the whole process while waiting for data.
- `poll-based event loop`: a single loop that monitors file descriptors for readiness and dispatches read/write handling.
- `CGI`: external script execution model where request metadata is passed through environment variables and request body through stdin; script output is parsed and translated to an HTTP response.

### High-level architecture
The server is organized into clear components:
- `config parsing`: reads and validates the server/location configuration.
- `network layer (Webserv)`: creates listening sockets, accepts clients, runs the poll loop, and manages connection lifecycle/timeouts.
- `request parsing`: parses start line, headers, and body (including route-relevant metadata).
- `request handling/routing`: maps a parsed request to the appropriate location behavior and method handler.
- `response building`: builds standardized HTTP responses (status/reason, headers, body).
- `file service`: filesystem operations (existence checks, read/write, directory listing, file metadata).

### Request-response cycle (detailed)
1. `accept`: a client connects to a configured listening socket.
2. `read`: incoming bytes are appended to the client input buffer.
3. `parse`: parser tries to produce one complete request from the buffer.
4. `route`: server finds best matching location and validates method/policies.
5. `dispatch`:
   - static/file flow: GET/POST/DELETE handlers access filesystem according to route rules.
   - CGI flow: CGI is started, input/output are managed asynchronously, and script output is parsed.
6. `build response`: status code, reason phrase, and headers are built; body is attached.
7. `write`: response bytes are sent to client output socket.
9. `cleanup`: finished descriptors/process state are released.

### Implemented capabilities (current scope)
- multiple locations and per-route behavior
- `GET`, `POST`, `DELETE`
- static file serving + index fallback
- autoindex (when enabled)
- upload handling (raw and multipart project-specific path)
- delete route handling
- redirects
- default/custom error pages
- CGI support (Python scripts in current configuration)

## Instructions
### Requirements
- C++ compiler with C++98 support (e.g. `c++`/`clang++`)
- `make`
- (For CGI routes) Python interpreter path configured in the config file (current example uses `/usr/bin/python3`)

### Build
From the project root:

```bash
make
```

Useful targets:

```bash
make clean
make fclean
make re
```

### Run
Run with a configuration file:

```bash
./webserv configs/full_test.conf
```

The included `configs/full_test.conf` starts the server on `127.0.0.1:8080` and includes routes for:
- static pages and index handling
- uploads and delete testing
- autoindex
- redirect
- CGI (`/cgi-bin`)

### Quick Manual Tests
```bash
# root
curl -i "http://127.0.0.1:8080/"

# redirect
curl -i "http://127.0.0.1:8080/redirect"

# upload (raw)
curl -i -X POST -H "Content-Type: text/plain" --data-binary "hello" "http://127.0.0.1:8080/upload/"

# upload (multipart)
curl -i -X POST -F "file=@/absolute/path/to/file.txt" "http://127.0.0.1:8080/upload/"

# CGI GET
curl -i "http://127.0.0.1:8080/cgi-bin/get_info.py?name=test"
```

## Resources
### Core HTTP / CGI references
- RFC 7230: HTTP/1.1 Message Syntax and Routing  
  https://www.rfc-editor.org/rfc/rfc7230
- RFC 7231: HTTP/1.1 Semantics and Content  
  https://www.rfc-editor.org/rfc/rfc7231
- RFC 3875: CGI Version 1.1  
  https://www.rfc-editor.org/rfc/rfc3875
- MDN HTTP reference  
  https://developer.mozilla.org/en-US/docs/Web/HTTP

### AI usage disclosure
AI tools were used as development support, specifically for:
- clarifying protocol details (status codes, headers, CGI semantics)
- discussing edge cases and expected behavior for request parsing, uploads, and CGI handling
- proposing refactor strategies and debugging hypotheses during integration
