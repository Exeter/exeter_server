#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sqlite3.h>
#include <map>
#include <sstream>
#include <string.h>
#include <iterator>
#include <dirent.h>
#include <sys/stat.h>
using namespace std;

string decodeURIComponent (string component) {
  //Decode a URI component into a normal string.
  int len = component.length();
  string r = "";
  int decoded;
  for (int i = 0; i < len; i += 1) {
    if (component[i] == '%') {
      char escape[] = {component[i+1], component[i+2]};
      stringstream d;
      d << hex << escape;
      d >> decoded;
      r += (char) decoded;
      i += 2;
    }
    else r += component[i]; 
  }
  return r;
}

map<string, string> parse_query(string query) {
  //Parse a query into a key/value map.
  int len = query.length();
  map<string, string> r;
  string key = "";
  string current = "";
  for (int i = 0; i < len; i += 1) {
    if (query[i] == '=') {
      key = current;
      current = "";
    }
    else if (query[i] == '&') {
      r[key] = decodeURIComponent(current);
      key = "";
      current = "";
    }
    else current += query[i];
  }
  r[key] = decodeURIComponent(current);
  return r;
}

string read_file(const char* filename) {
  //Read the entire text of a stream. Partial credit to Tyler McHenry.
  ifstream in (filename);
  if (in) {
    string contents;

    //Get the size of the file:
    in.seekg(0, ios::end);
    contents.resize(in.tellg());
    in.seekg(0, ios::beg);
    
    //Read it:
    in.read(&contents[0], contents.size());
    
    //Close and return:
    in.close();
    return contents;
  }
  throw 1;
}

int parse_path(string path, string*& dest) {
  //Parses a path into an array of directory names; returns the length of the array.
  int len = path.length();
  int depth = 0;
  for (int i = 0; i < len; i += 1) if (path[i] == '/') depth += 1;
  dest = new string[depth];
  string current = "";
  int x = 0;
  for (int i = 1; i < len; i += 1) {
    if (path[i] == '/') {
      dest[x] = current;
      x += 1;
      current = "";
    }
    else current += path[i];
  }
  dest[x] = current;
  return depth;
}

string rejoin_path(int n, string* path) {
  string r = "./";
  for (int i = 0; i < n; i += 1) {
    r += "/" + path[i];
  }
  return r;
}

string json_stringify(string a) {
  int len = a.length();
  string r = "\"";
  for (int i = 0; i < len; i += 1) {
    if (a[i] == '\"') r += "\\\"";
    else if (a[i] == '\n') r += "\\n";
    else r += a[i];
  }
  return r + "\"";
}

string json_stringify(int n, string* a) {
  string r = "[";
  for (int i = 0; i < n - 1; i += 1) {
    r += json_stringify(a[i]) + ',';
  }
  r += json_stringify(a[n - 1]) + ']';
  return r;
}

string to_string(const unsigned char* c) {
  string s (reinterpret_cast<const char*>(c));
  return s;
}

void terse_header(string content_type) {
  cout << "Content-type: " << content_type << endl << endl;
}

string get_order(map<string, string>& args) {
  if (args.count("order_by") == 1) {
    if (args["order_by"] == "timestamp DESC") return "timestamp DESC";
    else if (args["order_by"] == "votes") return "upvotes - downvotes DESC";
    else if (args["order_by"] == "name") return "name";
    else if (args["order_by"] == "code") return "codefiles=''";
    else return "name";
  }
  else return "name";
}

bool has_at(string a, string b, int index) {
  int len = b.length();
  if (a.length() - index < len) return false;
  for (int i = 0; i < len; i += 1) {
    if (a[i + index] != b[i]) return false;
  }
  return true;
}

string extract_filename(string disposition) {
  if (has_at(disposition, "filename", 47)) {
    string filename = "";
    for (int i = 57; disposition[i] != '"'; i += 1) {
      if (disposition[i] == '\\') {
        filename += disposition[i+1];
        i += 1;
      }
      else filename += disposition[i];
    }
    return filename;
  }
  else return "bogus_header";
}

int main(int n, char* args[], char** env) {
  if (strcmp(getenv("REQUEST_METHOD"), "GET") == 0) {
    string* path;
    int path_length = parse_path(getenv("PATH_INFO"), path);
    map<string, string> args = parse_query(getenv("QUERY_STRING"));
    sqlite3* db;
    sqlite3_open("ideas.db", &db);
    if (path_length == 0 || path[0] == "") {
      cout << "Refresh: 0; url=/cgi-bin/ideas/server/index.html" << endl;
      cout << "Content-type: text/html" << endl << endl;
    }
    else if (path[0] == "index.html") {
      terse_header("text/html");
      cout << read_file("front_end.html");
    }
    else if (path[0] == "file") {
      terse_header("text/plain");
      cout << read_file((rejoin_path(path_length, path).c_str()));
    }
    else if (path[0] == "load") {
      terse_header("text/plain");
      sqlite3_stmt* stmt;
      const char* pzTail;
      sqlite3_prepare_v2(db, ("SELECT * FROM ideas ORDER BY " + get_order(args)).c_str(), -1, &stmt, &pzTail);
      cout << "{\"posts\":[";
      string array_insides = "";
      while (sqlite3_step(stmt) == SQLITE_ROW) {
        int n = sqlite3_column_count(stmt);
        string columns[n];
        for (int i = 0; i < n; i += 1) columns[i] = to_string(sqlite3_column_text(stmt, i));
        array_insides += json_stringify(n, columns) + ',';
      }
      array_insides.erase(array_insides.size() - 1);
      cout << array_insides << "]}";
      sqlite3_finalize(stmt);
    }
    else if (path[0] == "submit") {
      terse_header("text/plain");
      sqlite3_stmt* stmt;
      const char* pzTail;
      sqlite3_prepare_v2(db,"INSERT INTO ideas (timestamp, name, description, upvotes, downvotes, codefiles) VALUES (DATETIME('now'), ?, ?, 0, 0, '')", -1, &stmt, &pzTail);
      sqlite3_bind_text(stmt, 1, args["name"].c_str(), strlen(args["name"].c_str()), SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, args["description"].c_str(), strlen(args["description"].c_str()), SQLITE_TRANSIENT);
      cout << "{sql_code:" << sqlite3_step(stmt) << "}";
      sqlite3_finalize(stmt);
    }
    else if (path[0] == "upvote") {
      terse_header("text/plain");
      sqlite3_stmt* stmt;
      const char* pzTail;
      sqlite3_prepare_v2(db, "UPDATE ideas SET upvotes=upvotes+1 WHERE rowid=?", -1, &stmt, &pzTail);
      sqlite3_bind_int(stmt, 1, atoi(args["rowid"].c_str()));
      sqlite3_step(stmt);
      sqlite3_finalize(stmt);
      cout << "{\"success\":true}";
    }
    else if (path[0] == "downvote") {
      terse_header("text/plain");
      sqlite3_stmt* stmt;
      const char* pzTail;
      sqlite3_prepare_v2(db, "UPDATE ideas SET downvotes=downvotes+1 WHERE rowid=?", -1, &stmt, &pzTail);
      sqlite3_bind_int(stmt, 1, atoi(args["rowid"].c_str()));
      sqlite3_step(stmt);
      sqlite3_finalize(stmt);
      cout << "{\"success\":true}";
    }
    else {
      terse_header("text/plain");
      cout << "Unrecognizable path: (" << path_length << ") " << rejoin_path(path_length, path) << endl;
    }
    sqlite3_close(db);
  }
  else if (strcmp(getenv("REQUEST_METHOD"),"POST") == 0) {
    string* path;
    int path_length = parse_path(getenv("PATH_INFO"), path);
    map<string, string> args = parse_query(getenv("QUERY_STRING"));
    sqlite3* db;
    sqlite3_open("ideas.db", &db);
    if (path[0] == "save") {
      terse_header("text/plain");
      cin >> noskipws;
      //Deal with headers:
      string line;
      string filename;
      while (getline(cin, line)) {
        if (has_at(line, "Content-Disposition", 0)) {
          filename = extract_filename(line);
        }
        else if (line.length() == 0) {
          break;
        }
      }
      //Get the body:
      string entire_file = "";
      string line_1;
      string line_2;
      while (getline(cin, line)) {
        entire_file += line_2;
        line_2 = line_1;
        line_1 = line + '\n';
      }
      sqlite3_stmt* stmt;
      const char* pzTail;
      int okay = sqlite3_prepare_v2(db, "UPDATE ideas SET codefiles = codefiles || ';' || ?, timestamp = DATETIME('now') WHERE rowid = ?", -1, &stmt, &pzTail);
      if (okay != SQLITE_OK) {
        cout << "{\"success\":false, \"okay\":" << okay << "}";
        sqlite3_close(db);
        return 0;
      }
      string actual_filename = "prototypes/" + args["rowid"] + "/" + filename;
      sqlite3_bind_text(stmt, 1, actual_filename.c_str(), strlen(actual_filename.c_str()), SQLITE_TRANSIENT);
      sqlite3_bind_int(stmt, 2, atoi(args["rowid"].c_str()));
      int sql_result = sqlite3_step(stmt);
      if (sql_result == 101) {
        if (opendir(("file/prototypes/" + args["rowid"]).c_str()) == NULL) {
          mkdir(("file/prototypes/" + args["rowid"]).c_str(), S_IRWXU | S_IRWXO);
        }
        ofstream w (("file/" + actual_filename).c_str());
        w << entire_file;
        cout << "{\"success\":true,\"filename\":\"" << actual_filename << "\"}";
      }
      else {
        cout << "{\"success\":false, \"sql_number\":" << sql_result << "}";
      }
    }
    else {
      terse_header("text/plain");
      cout << "Unrecognizable path." << endl;
    }
    sqlite3_close(db);
  }
  else {
    terse_header("text/plain");
    cout << "Unrecognizable request method." << endl;
  }
}
