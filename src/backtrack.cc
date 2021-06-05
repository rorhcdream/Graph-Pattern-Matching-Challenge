/**
 * @file backtrack.cc
 *
 */
#include "backtrack.h"
#include <cassert>
#include <ctime>

using namespace std;

Backtrack::Backtrack() {}
Backtrack::~Backtrack() {}

/**
 * @brief Checks if a match is a correct embedding query->data.
 *
 * @return Assertions succeed if correct.
 */
void checkMatch(const Graph &data, const Graph &query, const map<Vertex, Vertex> &uv_map){
  assert (uv_map.size() == query.GetNumVertices() && "uv_map is weird");
  
  Vertex u1, v1, u2, v2;
  for (auto it1 = uv_map.begin(), end = uv_map.end(); it1 != end;){
    u1 = it1->first; v1 = it1->second;

    assert (query.GetLabel(u1) == data.GetLabel(v1) && "label is different");

    for (auto it2 = ++it1, end = uv_map.end(); it2 != end; ++it2){
      
      u2 = it2->first; v2 = it2->second;
      assert (!(query.IsNeighbor(u1, u2) && !data.IsNeighbor(v1, v2)) && "lost edge");
      assert (v1 != v2 && "same vertex matched");
    }
  }
}

/**
 * @brief Performs backtracking embedding search and produces the output file.
 *
 * @return void
 */
void Backtrack::PrintAllMatches(const Graph &data, const Graph &query,
                                const CandidateSet &cs) {
  /* Unused */
  // clock_t start = clock();
  // int count = 0;
  
  // first output line
  printf("t %lu\n", query.GetNumVertices());

  // query -> DAG
  Graph *DAG = query.BuildDAG(cs);
  
  // start matching
  const size_t numVertices = DAG->GetNumVertices();
  
  // Denote a vertex in query 'u', and a vertex in cs 'v'

  // list of v we need to visit at each level
  vector<vector<Vertex>> backtrack(numVertices+1);
  // set of currently extendable u and its possible match v
  set<tuple<int, Vertex, vector<Vertex>>> extendNext;
  // stack of add diff for extendNext
  vector<vector<tuple<int, Vertex, vector<Vertex>>>> extendNextAdded(numVertices+1);
  // stack of remove diff for extendNext
  vector<tuple<int, Vertex, vector<Vertex>>> extendNextRemoved(numVertices+1);
  // record of u matched at each level
  vector<Vertex> u_vector(numVertices+1);
  // record of v matched at each level
  vector<Vertex> v_vector(numVertices+1);
  // index of v to visit next at each level
  vector<int> idx(numVertices+1, 0);
  // map of matched pairs <u, v>
  map<Vertex, Vertex> uv_map;
  // set of v currently matched
  set<Vertex> v_set;
  // indicate if we got up or down a level
  bool levelDown = false;  
  // set of result string "a ...\n"
  set<string> found;
  

  // visit root of DAG at level 1
  Vertex root = DAG->GetRoot();
  for (size_t ci = 0; ci < cs.GetCandidateSize(root); ci++)
    backtrack[1].push_back(cs.GetCandidate(root, ci));
  u_vector[1] = root;
  
  // currently (level-1) vertices matched
  long level = 1;
  while (level != 0) {
    if (levelDown) {
      levelDown = false;
      extendNext.insert(extendNextRemoved[level]);
      v_set.erase(v_vector[level]);
    }

    for (auto it = extendNextAdded[level].begin(); it != extendNextAdded[level].end(); it++) {
      extendNext.erase(*it);
    }

    Vertex u = u_vector[level];

    // current level search done
    if (idx[level] >= (long)backtrack[level].size()) {
      levelDown = true;
      uv_map.erase(u);
      level--;
      idx[level]++;
      continue;
    }

    Vertex v = backtrack[level][idx[level]];

    // v already matched
    if (v_set.find(v) != v_set.end()) {
      idx[level]++;
      continue;
    }

    uv_map[u] = v;
    v_set.insert(v);
    extendNextAdded[level].clear();  

    // if all u matched, print result
    if (level == (long)numVertices) {
      // assert (extendNext.size() == 0 && "extend next size should be 0");
      
      // // DUPLICATE EMBEDDING CHECK
      // string result = "a";
      // for (size_t i = 0; i < numVertices; i++)
      //   result += " " + to_string(uv_map[i]);
      // result += "\n";
      // bool inserted = found.insert(result).second;
      // assert (inserted && "already inserted");

      printf("a");
      for (size_t i = 0; i < numVertices; i++)
        printf(" %d", uv_map[i]);
      printf("\n");
      
      // count++;
      // if (count == 100000) {
      //   printf("count: %d, time: %ld\n", count, clock() - start);
      //   return;
      // }
      
      // // CORRECT EMBEDDING CHECK
      // checkMatch(data, query, uv_map);

      // idx[level]++;
      // v_set.erase(v);
      // continue;
    }

    // for all u's extendable children cu, add it to extendNext
    bool cu_extendable = true;
    for (size_t i = DAG->GetNeighborStartOffset(u); i < DAG->GetNeighborEndOffset(u); i++) {
      Vertex cu = DAG->GetNeighbor(i);
      vector<Vertex> candidates;
      
      // if a parent of cu is not matched, skip
      bool hasAllParentMatched = true;
      for (size_t pi = DAG->GetParentStartOffset(cu); pi < DAG->GetParentEndOffset(cu); pi++) {
        Vertex p_cu = DAG->GetParent(pi);
        if (uv_map.find(p_cu) == uv_map.end()) {
          hasAllParentMatched = false;
          break;
        }
      }
      if (!hasAllParentMatched)
        continue;

      // find all possible v of cu, add it to extendNext
      for (size_t ci = 0; ci < cs.GetCandidateSize(cu); ci++) {
        Vertex cv = cs.GetCandidate(cu, ci);
        bool cv_extendable = true;
        if (v_set.find(cv) == v_set.end()) {
          for (size_t pi = DAG->GetParentStartOffset(cu); pi < DAG->GetParentEndOffset(cu); pi++) {
            Vertex p_cu = DAG->GetParent(pi);
            if (!data.IsNeighbor(uv_map[p_cu], cv)) { 
              cv_extendable = false;
              break;
            }
          }
          if (cv_extendable)
            candidates.push_back(cv);
        }
      }
      // there is a cu that cannot be matched
      if (candidates.size() == 0) {
        cu_extendable = false;
        break;
      }
      // for (auto it = extendNext.begin(); it != extendNext.end(); it++) {
      //   assert (get<1>(*it) != cu && "cu");
      // }

      auto et = make_tuple(candidates.size(), cu, candidates);
      extendNext.insert(et);
      extendNextAdded[level].push_back(et);
    }
    
    // if not extendable, proceed in same level
    if (!cu_extendable || extendNext.size() == 0) {
      idx[level]++;
      v_set.erase(v);
    }
    // if extendable, go to next level
    else {
      auto p = *extendNext.begin();
      extendNext.erase(p);
      extendNextRemoved[level] = p;
      v_vector[level] = v;
      Vertex nextu = get<1>(p);
      vector<Vertex> candidates = get<2>(p);
      
      level++;
      idx[level] = 0;
      u_vector[level] = nextu;
      backtrack[level] = candidates;
      extendNextAdded[level].clear();
    }
  }

  delete DAG;
}
