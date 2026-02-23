#!/usr/bin/env python3
import os

print("Content-Type: text/plain")
print()
for key in sorted(os.environ.keys()):
    if key.startswith("HTTP_") or key in (
        "REQUEST_METHOD", "QUERY_STRING", "CONTENT_TYPE", "CONTENT_LENGTH",
        "SCRIPT_NAME", "PATH_INFO", "SERVER_PROTOCOL"
    ):
        print(f"{key}={os.environ.get(key, '')}")
