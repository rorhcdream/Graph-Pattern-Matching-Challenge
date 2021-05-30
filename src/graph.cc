/**
 * @file graph.cc
 *
 */

#include "graph.h"
#include <list>
#include <set>
#include <cassert>

namespace {
std::vector<Label> transferred_label;
void TransferLabel(const std::string &filename) {
  std::ifstream fin(filename);

  if (!fin.is_open()) {
    std::cout << "Graph file " << filename << " not found!\n";
    exit(EXIT_FAILURE);
  }

  char type;
  int32_t graph_id_;
  size_t num_vertices_;

  std::set<Label> label_set;

  fin >> type >> graph_id_ >> num_vertices_;

  // preprocessing
  while (fin >> type) {
    if (type == 'v') {
      Vertex id;
      Label l;
      fin >> id >> l;

      label_set.insert(l);
    } else if (type == 'e') {
      Vertex v1, v2;
      Label l;
      fin >> v1 >> v2 >> l;
    }
  }

  fin.close();

  transferred_label.resize(
      *std::max_element(label_set.begin(), label_set.end()) + 1, -1);

  Label new_label = 0;
  for (Label l : label_set) {
    transferred_label[l] = new_label;
    new_label += 1;
  }
}
}  // namespace

Graph::Graph(){}

Graph::Graph(const std::string &filename, bool is_query) {
  if (!is_query) {
    TransferLabel(filename);
  }
  std::vector<std::vector<Vertex>> adj_list;

  // Load Graph
  std::ifstream fin(filename);
  std::set<Label> label_set;

  if (!fin.is_open()) {
    std::cout << "Graph file " << filename << " not found!\n";
    exit(EXIT_FAILURE);
  }
  char type;

  fin >> type >> graph_id_ >> num_vertices_;

  adj_list.resize(num_vertices_);

  start_offset_.resize(num_vertices_ + 1);
  label_.resize(num_vertices_);

  num_edges_ = 0;

  // preprocessing
  while (fin >> type) {
    if (type == 'v') {
      Vertex id;
      Label l;
      fin >> id >> l;

      if (static_cast<size_t>(l) >= transferred_label.size())
        l = -1;
      else
        l = transferred_label[l];

      label_[id] = l;
      label_set.insert(l);
    } else if (type == 'e') {
      Vertex v1, v2;
      Label l;
      fin >> v1 >> v2 >> l;

      adj_list[v1].push_back(v2);
      adj_list[v2].push_back(v1);

      num_edges_ += 1;
    }
  }

  fin.close();

  adj_array_.resize(num_edges_ * 2);

  num_labels_ = label_set.size();

  max_label_ = *std::max_element(label_set.begin(), label_set.end());

  label_frequency_.resize(max_label_ + 1);

  start_offset_by_label_.resize(num_vertices_ * (max_label_ + 1));

  start_offset_[0] = 0;
  for (size_t i = 0; i < adj_list.size(); ++i) {
    start_offset_[i + 1] = start_offset_[i] + adj_list[i].size();
  }

  for (size_t i = 0; i < adj_list.size(); ++i) {
    label_frequency_[GetLabel(i)] += 1;

    auto &neighbors = adj_list[i];

    if (neighbors.size() == 0) continue;

    // sort neighbors by ascending order of label first, and descending order of
    // degree second
    std::sort(neighbors.begin(), neighbors.end(), [this](Vertex u, Vertex v) {
      if (GetLabel(u) != GetLabel(v))
        return GetLabel(u) < GetLabel(v);
      else if (GetDegree(u) != GetDegree(v))
        return GetDegree(u) > GetDegree(v);
      else
        return u < v;
    });

    Vertex v = neighbors[0];
    Label l = GetLabel(v);

    start_offset_by_label_[i * (max_label_ + 1) + l].first = start_offset_[i];

    for (size_t j = 1; j < neighbors.size(); ++j) {
      v = neighbors[j];
      Label next_l = GetLabel(v);

      if (l != next_l) {
        start_offset_by_label_[i * (max_label_ + 1) + l].second =
            start_offset_[i] + j;
        start_offset_by_label_[i * (max_label_ + 1) + next_l].first =
            start_offset_[i] + j;
        l = next_l;
      }
    }

    start_offset_by_label_[i * (max_label_ + 1) + l].second =
        start_offset_[i + 1];

    std::copy(adj_list[i].begin(), adj_list[i].end(),
              adj_array_.begin() + start_offset_[i]);
  }
}

bool isDAG(Vertex v, std::vector<std::vector<Vertex>> &adj_list){
  static std::set<Vertex> s;
  if(s.find(v) != s.end()) return false;
  s.insert(v);
  for (auto it = adj_list[v].begin(); it != adj_list[v].end(); it++){
    if(!isDAG(*it, adj_list)) return false;
  }
  s.erase(v);
  return true;
}

Graph *Graph::BuildDAG() const {
  std::vector<std::vector<Vertex>> adj_list(num_vertices_);
  std::vector<std::vector<Vertex>> par_list(num_vertices_);

  std::set<Vertex> visited;
  std::list<Vertex> toVisit;
  size_t count_edges = 0;

  // build adj_list by BFS
  toVisit.push_back(0);
  while (!toVisit.empty()) {
    Vertex v = toVisit.front();
    toVisit.pop_front();

    if (visited.find(v) != visited.end())
      continue;
    visited.insert(v);

    for (size_t i = GetNeighborStartOffset(v); i < GetNeighborEndOffset(v); i++) {
      Vertex u = adj_array_[i];
      if (visited.find(u) != visited.end()) {
        adj_list[u].push_back(v);
        par_list[v].push_back(u);
        count_edges += 1;
      }
      else if(std::find(toVisit.begin(), toVisit.end(), u) == toVisit.end())
        toVisit.push_back(u);
    }
  }

  // declare a new graph
  Graph *result = new Graph();

  // copy invariant members from the original graph
  result->graph_id_ = -1;
  result->num_vertices_ = num_vertices_;
  result->num_edges_ = num_edges_;
  result->num_labels_ = num_labels_;
  result->max_label_ = max_label_;
  result->label_frequency_.resize(max_label_+1);
  std::copy(label_frequency_.begin(), label_frequency_.end(), result->label_frequency_.begin());
  result->label_.resize(num_vertices_);
  std::copy(label_.begin(), label_.end(), result->label_.begin());

  // set result->start_offset_/start_offset_by_label_/adj_array_
  result->start_offset_.resize(num_vertices_ + 1);
  result->start_offset_by_label_.resize(num_vertices_ * (max_label_ + 1));
  result->adj_array_.resize(num_edges_);

  result->start_offset_[0] = 0;
  for (size_t i = 0; i < adj_list.size(); i++) {
    auto &neighbors = adj_list[i];

    // fill start_offset_
    result->start_offset_[i+1]
      = result->start_offset_[i] + neighbors.size();

    // if no outgoing edge, done
    if(!neighbors.size()) continue;

    // sort neighbors by asc label, desc degree
    std::sort(neighbors.begin(), neighbors.end(), [this](Vertex u, Vertex v) {
      if (GetLabel(u) != GetLabel(v))
        return GetLabel(u) < GetLabel(v);
      else if (GetDegree(u) != GetDegree(v))
        return GetDegree(u) > GetDegree(v);
      else
        return u < v;
    });
    
    // fill start_offset_by_label_
    Vertex v = neighbors[0];
    Label l = GetLabel(v);
    result->start_offset_by_label_[i * (max_label_ + 1) + l].first =
        result->start_offset_[i];
    for (size_t j = 1; j < adj_list[i].size(); ++j) {
      v = neighbors[j];
      Label next_l = GetLabel(v);
      if (l != next_l) {
        result->start_offset_by_label_[i * (max_label_ + 1) + l].second =
            result->start_offset_[i] + j;
        result->start_offset_by_label_[i * (max_label_ + 1) + next_l].first =
            result->start_offset_[i] + j;
        l = next_l;
      }
    }
    result->start_offset_by_label_[i * (max_label_ + 1) + l].second =
        result->start_offset_[i + 1];

    // fill adj_array_
    std::copy(neighbors.begin(), neighbors.end(),
              result->adj_array_.begin() + result->start_offset_[i]);
  }  
  
  // fill par array
  result->start_offset_par_.resize(num_vertices_ + 1);
  result->par_array_.resize(num_edges_);
  for (size_t i = 0; i < par_list.size(); i++) {
    auto &neighbors = par_list[i];

    // fill start_offset_par
    result->start_offset_par_[i+1]
      = result->start_offset_par_[i] + neighbors.size();

    // if no outgoing edge, done
    if(!neighbors.size()) continue;

    // fill par_array_
    std::copy(neighbors.begin(), neighbors.end(),
              result->par_array_.begin() + result->start_offset_par_[i]);
  }  

  // /* CORRECTNESS CHECK */

  // assert(count_edges == num_edges_ && "Some edges are lost");
  // assert(isDAG(0, adj_list) && "Not a DAG");

  // for (size_t i = 0; i < num_vertices_; i++) {
  //   for (Label l = 0; l <= max_label_; l++) {
  //     if (result->GetNeighborStartOffset(i, l) == 0)
  //       continue;
  //     assert(result->GetNeighborStartOffset(i) <= result->GetNeighborStartOffset(i, l));
  //     assert(result->GetNeighborEndOffset(i) >= result->GetNeighborEndOffset(i, l));

  //     for (size_t idx = result->GetNeighborStartOffset(i, l); idx < result->GetNeighborEndOffset(i, l); idx++) {
  //       assert(result->GetLabel(result->adj_array_[idx]) == l);
  //     }
  //   }
  // }

  // for (size_t v1 = 0; v1 < num_vertices_; v1++) {
  //   for (size_t v2 = v1; v2 < num_vertices_; v2++) {
  //     if (result->IsChild(v1, v2))
  //       assert(result->IsParent(v2, v1));
  //   }
  // }

  return result;  
}

Graph::~Graph() {}
