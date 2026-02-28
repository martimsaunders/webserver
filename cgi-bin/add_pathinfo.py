#!/usr/bin/env python3
import os
import html

path_info = os.environ.get("PATH_INFO", "")
parts = [p for p in path_info.split('/') if p]

ok = (len(parts) >= 2)
a = 0
b = 0
if ok:
    try:
        a = int(parts[0])
        b = int(parts[1])
    except ValueError:
        ok = False

print("Status: 200 OK")
print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>CGI ADD PATHINFO</h1>")
print("<p><strong>PATH_INFO:</strong> %s</p>" % html.escape(path_info))
if ok:
    print("<p><strong>%d + %d = %d</strong></p>" % (a, b, a + b))
else:
    print("<p><strong>Usage:</strong> /cgi-bin/add_pathinfo.py/&lt;int1&gt;/&lt;int2&gt;</p>")
print("</body></html>")
