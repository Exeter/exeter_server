#!/usr/bin/python
'''
  Copyright (c) 2013 Anthony Bau
  MIT license: http://opensource.org/licenses/MIT
  
  This is actually very little. It is meant to be the glue between some front-end, SQL, and a neat statistical engine in C++ (not yet made).
  Yep.
'''
import BaseHTTPServer
import sqlite3
import urlparse
import simplejson as JSON
import os

db = sqlite3.connect("ideas.db")


def init_db():
  c = db.cursor()
  c.execute("CREATE TABLE IF NOT EXISTS ideas (rowid INTEGER PRIMARY KEY ASC, timestamp DATE, name TEXT, description TEXT, upvotes INT, downvotes INT, codefiles TEXT)")
  db.commit()
  
def new_idea(name, description):
  c = db.cursor()
  c.execute("INSERT INTO ideas (timestamp, name, description, upvotes, downvotes, codefiles) VALUES (DATETIME('now'), ?, ?, 0, 0, '')", (name, description))
  db.commit()

def load_ideas(n, s):
  c = db.cursor()
  print s
  c.execute("SELECT * FROM ideas ORDER BY %s" % ("name" if (s == "name") else "upvotes - downvotes DESC" if (s == "votes") else "timestamp DESC"))
  return c.fetchmany(n)

def upvote(n):
  c = db.cursor()
  c.execute("UPDATE ideas SET upvotes = upvotes + 1, timestamp=DATETIME('now') WHERE rowid = ?", (n,))
  db.commit()

def downvote(n):
  c = db.cursor()
  c.execute("UPDATE ideas SET downvotes = downvotes + 1, timestamp=DATETIME('now') WHERE rowid = ?", (n,))
  db.commit()

def addCode(n, content):
  print content
  c = db.cursor()
  c.execute("UPDATE ideas SET codefiles = codefiles || ';' || ?, timestamp=DATETIME('now') WHERE rowid = ?", (content,n))
  db.commit()

def run(server = BaseHTTPServer.HTTPServer,
        handler = BaseHTTPServer.BaseHTTPRequestHandler):
    max_id = 0
    address = ('', 8080)
    httpd = server(address, handler)
    httpd.serve_forever()

class RequestHandler (BaseHTTPServer.BaseHTTPRequestHandler):
  def do_GET(s):
    path = s.path
    s.send_response(200)
    if (path == "/"):
      s.send_header("Content-type","text/html")
    else:
      s.send_header("Content-type","text/plain")
    s.end_headers()
    if (path[0:8] == "/submit?"):
      args = urlparse.parse_qs(path[8:])
      print args
      new_idea(args["name"][0],args["description"][0])
      s.wfile.write("Submission complete.")
    elif (path[0:6] == "/file/" and path[6] != "."):
      s.wfile.write(open("./" + path[6:]).read())
    elif (path[0:6] == "/load?"):
      order = urlparse.parse_qs(path[6:])
      s.wfile.write(JSON.dumps(load_ideas(100, order["order_by"][0])))
    elif (path[0:5] == "/seal"):
      s.wfile.write(open("Phillips_Exeter_Academy_Seal.png","rb").read())
    elif (path[0:8] == "/upvote?"):
      args = urlparse.parse_qs(path[8:])
      print args
      upvote(int(args["rowid"][0]))
    elif (path[0:10] == "/downvote?"):
      args = urlparse.parse_qs(path[10:])
      print args
      downvote(int(args["rowid"][0]))
    else:
      s.wfile.write(open("front_end.html").read())
  def do_POST(s):
    path = s.path
    s.send_response(200)
    s.send_header("Content-type","text/plain")
    s.end_headers()
    if (path[0:5] == "/save"):
      args = urlparse.parse_qs(path[6:])
      print args
      lines = s.rfile.read(int(s.headers.getheader("content-length"))).split('\n')
      filename = "prototypes/" + args["rowid"][0] + "/" + lines[1][lines[1].index("filename=\"")+10:len(lines[1])-2]
      content = "\n".join(lines[4:len(lines)-2])
      if not os.path.exists("prototypes/" + args["rowid"][0]):
        os.makedirs("prototypes/" + args["rowid"][0])
      open(filename, "w").write(content)
      addCode(int(args["rowid"][0]),filename)
      s.wfile.write("{\"success\":true}")
      print "{\"success\":true}"

init_db()
run(handler = RequestHandler)
