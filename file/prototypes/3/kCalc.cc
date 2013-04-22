int main(int h, char* args[]) {
  ifstream in (args[1]);
  double total = 0;
  int n = 0;
  int which = 0;
  while (true) {
    string t;
    in >> t;
    if (t.compare("--") == 0) {
      which += 1;
      continue;
    }
    else {
      if (which <= 1) {
        string mK = t;
        string r;
        string F;
        in >> r;
        in >> F;
        total += stod(mK) * sqrt(stod(F) / stod(r));
      }
      else if (which == 2) {
        string fK = t;
        string r;
        string m;
        in >> r;
        in >> m;
        total += stod(fK) / sqrt(stod(m) * stod(r));
      }
      else break;
      n += 1;
    }
  }
  cout << total / n << endl;
}
