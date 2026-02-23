#!/usr/bin/env python3
import os
import sys

length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
body = sys.stdin.read(length) if length > 0 else ""

print("Content-Type: text/plain")
print()
print("CGI POST ECHO")
print("CONTENT_TYPE=", os.environ.get("CONTENT_TYPE", ""))
print("CONTENT_LENGTH=", os.environ.get("CONTENT_LENGTH", ""))
print("BODY=")
print(body)
