#!/usr/bin/env python3
import os
import sys
import html


def parse_boundary(content_type):
    marker = "boundary="
    idx = content_type.find(marker)
    if idx == -1:
        return ""
    boundary = content_type[idx + len(marker):].strip()
    if boundary.startswith('"') and boundary.endswith('"') and len(boundary) >= 2:
        boundary = boundary[1:-1]
    return boundary


def parse_first_part(body, boundary):
    delim = "--" + boundary
    parts = body.split(delim)
    for part in parts:
        p = part.strip("\r\n")
        if not p or p == "--":
            continue

        sep = p.find("\r\n\r\n")
        sep_len = 4
        if sep == -1:
            sep = p.find("\n\n")
            sep_len = 2
        if sep == -1:
            continue

        head = p[:sep]
        data = p[sep + sep_len:]

        filename = ""
        content_type = ""
        for line in head.splitlines():
            low = line.lower()
            if low.startswith("content-disposition:"):
                key = 'filename="'
                k = line.find(key)
                if k != -1:
                    s = k + len(key)
                    e = line.find('"', s)
                    if e != -1:
                        filename = line[s:e]
            elif low.startswith("content-type:"):
                content_type = line.split(":", 1)[1].strip()

        return filename, content_type, data

    return "", "", ""


method = os.environ.get("REQUEST_METHOD", "")
content_type = os.environ.get("CONTENT_TYPE", "")
raw_len = os.environ.get("CONTENT_LENGTH", "0")

try:
    length = int(raw_len)
except ValueError:
    length = 0

body = sys.stdin.read(length) if length > 0 else ""
boundary = parse_boundary(content_type)
filename, part_type, data = ("", "", "")
if boundary:
    filename, part_type, data = parse_first_part(body, boundary)

print("Status: 200 OK")
print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>CGI MULTIPART OK</h1>")
print("<p><strong>Method:</strong> %s</p>" % html.escape(method))
print("<p><strong>Request Content-Type:</strong> %s</p>" % html.escape(content_type))
print("<p><strong>Boundary:</strong> %s</p>" % html.escape(boundary))
print("<p><strong>Filename:</strong> %s</p>" % html.escape(filename))
print("<p><strong>Part Content-Type:</strong> %s</p>" % html.escape(part_type))
print("<p><strong>Part Length:</strong> %d</p>" % len(data))
print("<pre>%s</pre>" % html.escape(data))
print("</body></html>")
