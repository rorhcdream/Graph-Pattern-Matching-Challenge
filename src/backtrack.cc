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

void checkMatch(const Graph &data, const Graph &query, const map<Vertex, Vertex> &uvmatch){
  assert (uvmatch.size() == query.GetNumVertices() && "uvmatch is weird");
  
  Vertex u1, v1, u2, v2;
  for (auto it1 = uvmatch.begin(), end = uvmatch.end(); it1 != end;){
    u1 = it1->first; v1 = it1->second;

    assert (query.GetLabel(u1) == data.GetLabel(v1) && "label is different");

    for (auto it2 = ++it1, end = uvmatch.end(); it2 != end; ++it2){
      
      u2 = it2->first; v2 = it2->second;
      assert (!(query.IsNeighbor(u1, u2) && !data.IsNeighbor(v1, v2)) && "lost edge");
      assert (v1 != v2 && "same vertex matched");
    }
  }
}

void Backtrack::PrintAllMatches(const Graph &data, const Graph &query,
                                const CandidateSet &cs) {
  clock_t start = clock();
  int count = 0;
  
  printf("t %lu\n", query.GetNumVertices());

  // query -> DAG
  Graph *DAG = query.BuildDAG();
  
  const size_t numVertices = DAG->GetNumVertices();
  
  vector<vector<Vertex>> backtrack(numVertices+1);    // 각 레벨에서 백트래킹을 위해 앞으로 탐색해야 할 v들을 저장
  set<tuple<int, Vertex, vector<Vertex>>> extendNext;   // 현재 확장할 수 있는 u 목록을 저장
  vector<vector<tuple<int, Vertex, vector<Vertex>>>> extendNextAdded(numVertices+1);  // 각 레벨에서 extendNext에 추가한 u 목록을 저장
  vector<tuple<int, Vertex, vector<Vertex>>> extendNextRemoved(numVertices+1);  // 레벨을 올릴 때 extendNext에서 지운 u를 저장
  vector<Vertex> matchedVInLevel(numVertices+1);  // 해당 레벨에서 매치한 v 저장
  bool levelDown = false;   // 레벨이 내려온 상태인지 여부 저장

  vector<Vertex> uvector(numVertices+1);
  vector<int> idx(numVertices+1, 0);
  map<Vertex, Vertex> uvmatch;
  set<Vertex> matchedV;
  
  // start = root
  Vertex root = 0;
  for (size_t ci = 0; ci < cs.GetCandidateSize(root); ci++)
    backtrack[1].push_back(cs.GetCandidate(root, ci));
  uvector[1] = root;
  
  long level = 1; // currently (level-1) vertices matched
  while (level != 0) {
    if (levelDown) {
      levelDown = false;
      extendNext.insert(extendNextRemoved[level]);
      matchedV.erase(matchedVInLevel[level]);
    }
    for (auto it = extendNextAdded[level].begin(); it != extendNextAdded[level].end(); it++) {
      extendNext.erase(*it);
    }

    Vertex u = uvector[level];

    // current level search done
    if (idx[level] >= (long)backtrack[level].size()) {
      levelDown = true;
      uvmatch.erase(u);
      level--;
      idx[level]++;
      continue;
    }

    Vertex v = backtrack[level][idx[level]];

    // v already matched
    if (matchedV.find(v) != matchedV.end()) {
      idx[level]++;
      continue;
    }

    uvmatch[u] = v;
    matchedV.insert(v);
    extendNextAdded[level].clear();  

    // if all u matched, print result
    if (level == (long)numVertices) {
      // assert (extendNext.size() == 0 && "extend next size should be 0");
      
      printf("a");
      for (size_t i = 0; i < numVertices; i++)
        printf(" %d", uvmatch[i]);
      printf("\n");
      
      count++;
      if (count == 100000) {
        printf("count: %d, time: %ld\n", count, clock() - start);
        return;
      }
      
      // // CHECK MATCH
      // checkMatch(data, query, uvmatch);

      idx[level]++;
      matchedV.erase(v);
      continue;
    }

    // for all u's extendable children cu, add it to extendNext
    for (size_t i = DAG->GetNeighborStartOffset(u); i < DAG->GetNeighborEndOffset(u); i++) {
      Vertex cu = DAG->GetNeighbor(i);
      vector<Vertex> candidates;
      
      // if a parent of cu is not matched, skip
      bool hasAllParentMatched = true;
      for (size_t pi = DAG->GetParentStartOffset(cu); pi < DAG->GetParentEndOffset(cu); pi++) {
        Vertex p_cu = DAG->GetParent(pi);
        if (uvmatch.find(p_cu) == uvmatch.end()) {
          hasAllParentMatched = false;
          break;
        }
      }
      if (!hasAllParentMatched)
        continue;

      // find all possible v of cu, add it to extendNext
      for (size_t ci = 0; ci < cs.GetCandidateSize(cu); ci++) {
        Vertex cv = cs.GetCandidate(cu, ci);
        bool extendable = true;
        for (size_t pi = DAG->GetParentStartOffset(cu); pi < DAG->GetParentEndOffset(cu); pi++) {
          Vertex p_cu = DAG->GetParent(pi);
          if (!data.IsNeighbor(uvmatch[p_cu], cv)) {
            extendable = false;
            break;
          }
        }
        if (extendable)
          candidates.push_back(cv);
      }
      // there is a cu that cannot be matched
      if (candidates.size() == 0) {
        idx[level]++;
        matchedV.erase(v);
      }
      // for (auto it = extendNext.begin(); it != extendNext.end(); it++) {
      //   assert (get<1>(*it) != cu && "cu");
      // }

      auto et = make_tuple(candidates.size(), cu, candidates);
      extendNext.insert(et);
      extendNextAdded[level].push_back(et);
    }
    
    // if not extendable, proceed in same level
    if (extendNext.size() == 0) {
      idx[level]++;
      matchedV.erase(v);
    }
    // if extendable, go to next level
    else {
      auto p = *extendNext.begin();
      extendNext.erase(p);
      extendNextRemoved[level] = p;
      matchedVInLevel[level] = v;
      Vertex nextu = get<1>(p);
      vector<Vertex> candidates = get<2>(p);
      
      level++;
      idx[level] = 0;
      uvector[level] = nextu;
      backtrack[level] = candidates;
      extendNextAdded[level].clear();
    }
  }

  delete DAG;
}
