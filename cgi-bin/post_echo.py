#!/usr/bin/env python3
import os
import sys
import html

method = os.environ.get("REQUEST_METHOD", "")
content_type = os.environ.get("CONTENT_TYPE", "")
raw_len = os.environ.get("CONTENT_LENGTH", "0")

try:
    length = int(raw_len)
except ValueError:
    length = 0

body = sys.stdin.read(length) if length > 0 else ""

print("Status: 200 OK")
print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>CGI POST OK</h1>")
print("<p><strong>Method:</strong> %s</p>" % html.escape(method))
print("<p><strong>Content-Type:</strong> %s</p>" % html.escape(content_type))
print("<p><strong>Content-Length:</strong> %d</p>" % len(body))
print("<pre>%s</pre>" % html.escape(body))
print("</body></html>")
