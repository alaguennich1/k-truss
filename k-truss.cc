// #include "util.h"
#include <vector>
#include <queue>
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <gflags/gflags.h>
using namespace std;

DEFINE_string(G, "", "graph");
DEFINE_string(truss_type, "cycle", "truss type: cycle or flow");

struct graph_t {
  vector<pair<int, int> > edges;
  vector<int> offsets;
  vector<int> degs;
  int n;
  int m;

  graph_t():n(0), m(0) {};
  bool adjacent(int u, int v) const
  {
    return binary_search(edges.begin() + offsets[u], edges.begin() + offsets[u] + degs[u], make_pair(u, v));
  }
  int operator()(int u, int i) const
  {
      return edges[offsets[u] + i].second;
  }
  int edge(int u, int v) const
  {
    int e = lower_bound(edges.begin() + offsets[u], edges.begin() + offsets[u] + degs[u], make_pair(u, v)) - (edges.begin() + offsets[u]);
    return e != degs[u] ? e + offsets[u]: -1;    
  }
};

void load_graph(const string& fn, graph_t& G) {
  ifstream ifs(fn.c_str());
  for (string line; getline(ifs, line); ) {
    if (line.size() > 0 && line[0] == '#') continue;
    istringstream iss(line);
    int u = 0, v = 0;
    iss >> u >> v;
    if (u >= G.n || v >= G.n) G.n = max(u, v) + 1;
    if (u == v) continue;
    G.edges.push_back(make_pair(u, v));
  }
  sort(G.edges.begin(), G.edges.end());
  G.edges.erase(unique(G.edges.begin(), G.edges.end()), G.edges.end());
  G.m = G.edges.size();

  G.degs.resize(G.n);
  G.offsets.resize(G.n);
  for (int e = 0; e < G.m; ++e) ++G.degs[G.edges[e].first];
  for (int u = 0; u < G.n - 1; ++u) G.offsets[u + 1] = G.offsets[u] + G.degs[u];
  assert(G.offsets[G.n - 1] + G.degs[G.n - 1] == G.m);
}

void reverse_graph(const graph_t& G, graph_t& revG) {
  revG.n = G.n;
  revG.m = G.m;
  for (int e = 0; e < G.edges.size(); ++e) revG.edges.push_back(make_pair(G.edges[e].second, G.edges[e].first));
  sort(revG.edges.begin(), revG.edges.end());
  assert(revG.m = revG.edges.size());

  revG.degs.resize(revG.n);
  revG.offsets.resize(revG.n);
  for (int e = 0; e < revG.m; ++e) ++revG.degs[revG.edges[e].first];
  for (int u = 0; u < revG.n - 1; ++u) revG.offsets[u + 1] = revG.offsets[u] + revG.degs[u];
}

vector<int> find_trusses(int u, int v, const graph_t& uG, const graph_t& vG) {
  vector<int> ws;
  if (uG.degs[u] < vG.degs[v]) {
    for (int i = 0; i < uG.degs[u]; ++i) {
      int w = uG(u, i);
      if (w == u || w == v) continue;
      if (vG.adjacent(v, w)) ws.push_back(w);
    }
  } else {
    for (int i = 0; i < vG.degs[v]; ++i) {
      int w = vG(v, i);
      if (w == u || w == v) continue;
      if (uG.adjacent(u, w)) ws.push_back(w);
    }
  }
  return ws;
}

void cycle_k_truss(graph_t& G, vector<int>& levels) {
  int m = G.m;
  vector<pair<int, int> >& edges = G.edges;
  graph_t revG;
  reverse_graph(G, revG);
  levels.resize(m);

  vector<int> supp(m);
  vector<bool> removed(m);
  priority_queue<pair<int, int>, vector<pair<int, int> >, greater<pair<int, int> > > pq;
//cout << " hoge" <<endl;
  for (int e = 0; e < m; ++e) {
    int u = edges[e].first, v = edges[e].second;
    // u -> v -> w -> u
    vector<int> ws = find_trusses(u, v, revG, G);
    supp[e] = ws.size();
    if (supp[e] > 0) pq.push(make_pair(supp[e], e));
  }

  for (int k = 1; !pq.empty(); ++k) {
    while (!pq.empty() && pq.top().first <= k) {
      int e = pq.top().second;
      int u = edges[e].first, v = edges[e].second;
      pq.pop();
      if (removed[e]) continue;
      removed[e] = true;
      levels[e] = k;

      vector<int> ws = find_trusses(u, v, revG, G);
      for (int w : ws) {
        int wu = G.edge(w, u);
        int vw = G.edge(v, w);
        assert(wu >= 0 && vw >= 0);
        if (removed[wu] || removed[vw]) continue;
        assert(supp[wu] > 0 && supp[vw] > 0);
        pq.push(make_pair(--supp[wu], wu));
        pq.push(make_pair(--supp[vw], vw));
      }
    }
  }
}

void flow_k_truss(graph_t& G, vector<int>& levels) {
  int m = G.m;
  vector<pair<int, int> >& edges = G.edges;
  graph_t revG;
  reverse_graph(G, revG);
  levels.resize(m);

  vector<int> supp(m);
  vector<bool> removed(m);
  priority_queue<pair<int, int>, vector<pair<int, int> >, greater<pair<int, int> > > pq;

  for (int e = 0; e < m; ++e) {
    int u = edges[e].first, v = edges[e].second;
    supp[e] += find_trusses(u, v, G, revG).size(); // u -> v <- w <- u
    supp[e] += find_trusses(u, v, revG, revG).size(); // u -> v <- w -> u
    supp[e] += find_trusses(u, v, G, G).size(); // u -> v -> w <- u
    if (supp[e] > 0) pq.push(make_pair(supp[e], e));
  }

  for (int k = 1; !pq.empty(); ++k) {
    while (!pq.empty() && pq.top().first <= k) {
      int e = pq.top().second;
      int u = edges[e].first, v = edges[e].second;
      pq.pop();
      if (removed[e]) continue;
      removed[e] = true;
      levels[e] = k;

      vector<int> ws;

      // u -> v <- w <- u
      ws = find_trusses(u, v, G, revG);
      for (int w : ws) {
        int uw = G.edge(u, w);
        int wv = G.edge(w, v);
        assert(uw >= 0 && wv >= 0);
        if (removed[uw] || removed[wv]) continue;
        assert(supp[uw] > 0 && supp[wv] > 0);
        pq.push(make_pair(--supp[uw], uw));
        pq.push(make_pair(--supp[wv], wv));
      }

      // u -> v <- w -> u
      ws = find_trusses(u, v, revG, revG);
      for (int w : ws) {
        int wu = G.edge(w, u);
        int wv = G.edge(w, v);
        assert(wu >= 0 && wv >= 0);
        if (removed[wu] || removed[wv]) continue;
        assert(supp[wu] > 0 && supp[wv] > 0);
        pq.push(make_pair(--supp[wu], wu));
        pq.push(make_pair(--supp[wv], wv));
      }

      // u -> v -> w <- u
      ws = find_trusses(u, v, G, G);
      for (int w : ws) {
        int uw = G.edge(u, w);
        int vw = G.edge(v, w);
        assert(uw >= 0 && vw >= 0);
        if (removed[uw] || removed[vw]) continue;
        assert(supp[uw] > 0 && supp[vw] > 0);
        pq.push(make_pair(--supp[uw], uw));
        pq.push(make_pair(--supp[vw], vw));
      }

    }
  }
}


int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  graph_t G;
  load_graph(FLAGS_G, G);

  cout << "# G = " << FLAGS_G << endl;
  cout << "# truss_type = " << FLAGS_truss_type << endl;

  if (FLAGS_truss_type == "cycle") {
    vector<int> levels;
    cycle_k_truss(G, levels);
    for (int e = 0; e < G.m; ++e) cout << G.edges[e].first << " " << G.edges[e].second << " " << levels[e] << endl;
  } else {
    vector<int> levels;
    flow_k_truss(G, levels);
    for (int e = 0; e < G.m; ++e) cout << G.edges[e].first << " " << G.edges[e].second << " " << levels[e] << endl;
  }
}

