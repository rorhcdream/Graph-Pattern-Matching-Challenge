/**
 * @file backtrack.cc
 *
 */

#include "backtrack.h"

Backtrack::Backtrack() {}
Backtrack::~Backtrack() {}

void Backtrack::PrintAllMatches(const Graph &data, const Graph &query,
                                const CandidateSet &cs) {
  std::cout << "t " << query.GetNumVertices() << "\n";

  // query -> DAG
  Graph *DAG = query.BuildDAG();

  // root -> backtracking
    // select minimum candidate set
    // get intersect of parents

  free(DAG);
}
