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

db = sqlite3.connect("ideas.db")


def init_db():
  c = db.cursor()
  c.execute("CREATE TABLE IF NOT EXISTS ideas (rowid INTEGER PRIMARY KEY ASC, timestamp DATE, name TEXT, description TEXT, upvotes INT, downvotes INT)")
  db.commit()
  
def new_idea(name, description):
  c = db.cursor()
  c.execute("INSERT INTO ideas (timestamp, name, description, upvotes, downvotes) VALUES (DATETIME('now'), ?, ?, 0, 0)", (name, description))
  db.commit()

def load_ideas(n):
  c = db.cursor()
  c.execute("SELECT * FROM ideas ORDER BY timestamp DESC")
  return c.fetchmany(n)

def upvote(n):
  c = db.cursor()
  c.execute("UPDATE ideas SET upvotes = upvotes + 1 WHERE rowid = ?", (n,))
  db.commit()

def downvote(n):
  c = db.cursor()
  c.execute("UPDATE ideas SET downvotes = downvotes + 1 WHERE rowid = ?", (n,))
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
    if (path[0:6] == "/seal"):
      s.send_header("Content-type", "image/png")
    else:
      s.send_header("Content-type","text/html")
    s.end_headers()
    if (path[0:8] == "/submit?"):
      args = urlparse.parse_qs(path[8:])
      print args
      new_idea(args["name"][0],args["description"][0])
      s.wfile.write("Submission complete.")
    elif (path[0:5] == "/load"):
      s.wfile.write(JSON.dumps(load_ideas(100)))
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

init_db()
run(handler = RequestHandler)
