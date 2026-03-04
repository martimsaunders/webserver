# ---------- PREP FILES ----------
cat > /Users/pedroborrega/Desktop/full_payload.json <<'EOF'
{"msg":"hello from curl","id":42}
EOF

seq 1 200 | sed 's/^/line /' > /Users/pedroborrega/Desktop/big_text_file.txt

# ---------- ROOT / FORM / RESULT (GET) ----------
curl -i "http://127.0.0.1:8080/"
curl -i "http://127.0.0.1:8080/form"
curl -i "http://127.0.0.1:8080/form/result"


# ---------- STATIC (AUTOINDEX ON) ----------
curl -i "http://127.0.0.1:8080/static/"
curl -i "http://127.0.0.1:8080/static/index.html"   # if present


# ---------- REDIRECT ----------
curl -i "http://127.0.0.1:8080/redirect"            # should return 302 + Location
curl -i -L "http://127.0.0.1:8080/redirect"         # follow redirect


# ---------- UPLOAD PAGE ----------
curl -i "http://127.0.0.1:8080/upload"


# ---------- POST /upload (RAW BYTES) ----------
curl -i -X POST \
  -H "Content-Type: application/json" \
  --data-binary @/Users/pedroborrega/Desktop/full_payload.json \
  "http://127.0.0.1:8080/upload/"

curl -i -X POST \
  -H "Content-Type: text/plain" \
  --data-binary @/Users/pedroborrega/Desktop/big_text_file.txt \
  "http://127.0.0.1:8080/upload/"


# ---------- POST /upload (MULTIPART) ----------
curl -i -X POST \
  -F "file=@/Users/pedroborrega/Desktop/full_payload.json;type=application/json" \
  "http://127.0.0.1:8080/upload/"

curl -i -X POST \
  -F "file=@/Users/pedroborrega/Desktop/big_text_file.txt;type=text/plain" \
  "http://127.0.0.1:8080/upload/"


# ---------- UPLOADS (GET + AUTOINDEX ON) ----------
curl -i "http://127.0.0.1:8080/uploads/"
curl -i "http://127.0.0.1:8080/uploads/full_payload.json"   # if uploaded with that name


# ---------- DELETE ----------
# pick a real uploaded filename from /uploads autoindex and replace <name>
curl -i -X DELETE "http://127.0.0.1:8080/delete/<name>"
curl -i "http://127.0.0.1:8080/uploads/<name>"              # should now be 404


# ---------- CGI GET ----------
curl -i "http://127.0.0.1:8080/cgi-bin/get_info.py?name=pedro&mode=test"



# ---------- CGI POST (RAW BYTES) ----------
curl -i -X POST \
  -H "Content-Type: application/json" \
  --data '{"msg":"hello raw","id":42}' \
  "http://127.0.0.1:8080/cgi-bin/post_echo.py"


# ---------- CGI POST (MULTIPART) ----------
curl -i -X POST \
  -F "file=@/Users/pedroborrega/Desktop/full_payload.json;type=application/json" \
  "http://127.0.0.1:8080/cgi-bin/post_multipart.py"


# ---------- PATH_INFO addition script ----------
curl -i "http://127.0.0.1:8080/cgi-bin/add_pathinfo.py/7/35"


# malformed CGI output (no valid headers/body separator)
curl -i "http://127.0.0.1:8080/cgi-bin/tests/bad_output.py"

# non-zero exit
curl -i "http://127.0.0.1:8080/cgi-bin/tests/exit1.py"

# infinite loop (client-side timeout to avoid hanging terminal)
curl -i "http://127.0.0.1:8080/cgi-bin/tests/loop.py"
