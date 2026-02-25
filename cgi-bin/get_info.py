#!/usr/bin/env python3
import os
import html

method = os.environ.get("REQUEST_METHOD", "")
query = os.environ.get("QUERY_STRING", "")
path_info = os.environ.get("PATH_INFO", "")
script_name = os.environ.get("SCRIPT_NAME", "")

print("Status: 200 OK")
print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>CGI GET OK</h1>")
print("<p><strong>Method:</strong> %s</p>" % html.escape(method))
print("<p><strong>Script Name:</strong> %s</p>" % html.escape(script_name))
print("<p><strong>Path Info:</strong> %s</p>" % html.escape(path_info))
print("<p><strong>Query String:</strong> %s</p>" % html.escape(query))
print("</body></html>")
