/**
 * @file graph.cc
 *
 */

#include "graph.h"
#include <queue>
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

bool isCyclic(Vertex v, std::vector<std::vector<Vertex>> &adj_list){
  static std::vector<Vertex> visited;
  if(std::find(visited.begin(), visited.end(), v) != visited.end())
    return true;
  visited.push_back(v);
  for (auto it = adj_list[v].begin(); it != adj_list[v].end(); it++){
    if(isCyclic(*it, adj_list)) return true;
  }
  visited.pop_back();
  return false;
}

Graph *Graph::BuildDAG() const {
  std::vector<std::vector<Vertex>> adj_list(num_vertices_);

  std::set<Vertex> visited;
  std::queue<Vertex> toVisit;
  toVisit.push(0);
  
  // build adj_list
  while (!toVisit.empty()) {
    Vertex v = toVisit.front();
    toVisit.pop();

    if (visited.find(v) != visited.end())
      continue;
    visited.insert(v);

    for (size_t i = GetNeighborStartOffset(v); i < GetNeighborEndOffset(v); i++) {
      if (visited.find(adj_array_[i]) != visited.end()) 
        adj_list[adj_array_[i]].push_back(v);
      else
        toVisit.push(adj_array_[i]);
    }
  }

  // allocate new graph, copy basic values
  Graph *result = new Graph();
  result->graph_id_ = -1;
  result->num_vertices_ = num_vertices_;
  result->num_edges_ = num_edges_;
  result->num_labels_ = num_labels_;
  result->max_label_ = max_label_;
  result->label_frequency_.resize(max_label_+1);
  result->label_.resize(num_vertices_);
  result->start_offset_by_label_.resize(num_vertices_ * (max_label_ + 1));
  std::copy(label_frequency_.begin(), label_frequency_.end(), result->label_frequency_.begin());
  std::copy(label_.begin(), label_.end(), result->label_.begin());

  // fill result->adj_array / start_offset / start_offset_by_label
  size_t count_edges = 0;
  result->start_offset_.resize(num_vertices_);
  for (size_t i = 0; i < num_vertices_; i++) {
    result->start_offset_[i] = result->adj_array_.size();

    // sort by label
    std::sort(adj_list[i].begin(), adj_list[i].end(), [this](Vertex u, Vertex v) {
      if (GetLabel(u) != GetLabel(v))
        return GetLabel(u) < GetLabel(v);
      else if (GetDegree(u) != GetDegree(v))
        return GetDegree(u) > GetDegree(v);
      else
        return u < v;
    });

    // push to adj array
    for (auto it = adj_list[i].begin(); it != adj_list[i].end(); it++) {
      // assert(std::find(adj_list[*it].begin(), adj_list[*it].end(), i) == adj_list[*it].end() && "Should not have edge with reverse direction");
      count_edges++;
      result->adj_array_.push_back(*it);
    }
    
    // fill start_offset_by_label
    if (!adj_list[i].size()) 
      continue;
    Vertex u = adj_list[i][0];
    Label l = GetLabel(u);

    result->start_offset_by_label_[i * (max_label_ + 1) + l].first = start_offset_[i];

    for (size_t j = 1; j < adj_list[i].size(); ++j) {
      u = adj_list[i][j];
      Label next_l = GetLabel(u);

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
  }  

  // assert(count_edges == num_edges_ && "Some edges are lost");
  // assert(!isCyclic(0, adj_list) && "DAG is cyclic");

  // for (size_t i = 0; i < num_vertices_; i++) {
  //   for (size_t l = 0; l <= max_label_; l++) {
  //     if (result->GetNeighborStartOffset(i, l) == 0)
  //       continue;
  //     assert(result->GetNeighborStartOffset(i) <= result->GetNeighborStartOffset(i, l));
  //     assert(result->GetNeighborEndOffset(i) >= result->GetNeighborEndOffset(i, l));

  //     for (size_t idx = result->GetNeighborStartOffset(i, l); idx < result->GetNeighborEndOffset(i, l); idx++) {
  //       assert(result->GetLabel(result->adj_array_[idx]) == l);
  //     }
  //   }
  // }

  return result;  
}

Graph::~Graph() {}
