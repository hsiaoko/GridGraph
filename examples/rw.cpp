/*
Copyright (c) 2014-2015 Xiaowei Zhu, Tsinghua University

Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "core/graph.hpp"
static bool Drop(VertexId v) { return rand() % 2 == 0; }

int main(int argc, char** argv) {
  if (argc < 6) {
    fprintf(stderr,
            "usage: rw [path] [mod of sources] [walks per source] [niters] "
            "[memory budget in GB]\n");
    exit(-1);
  }
  std::string path = argv[1];
  long mod = atol(argv[2]);
  long walks_per_source = atol(argv[3]);
  long niters = atol(argv[4]);
  long memory_bytes = (argc >= 5) ? atol(argv[5]) * 1024l * 1024l * 1024l
                                  : 8l * 1024l * 1024l * 1024l;

  Graph graph(path);
  graph.set_memory_bytes(memory_bytes);
  Bitmap* active_in = graph.alloc_bitmap();
  Bitmap* active_second_in = graph.alloc_bitmap();
  Bitmap* active_out = graph.alloc_bitmap();
  BigVector<VertexId> label(graph.path + "/label", graph.vertices);
  // BigVector<VertexId> count_walks(graph.path+"/count_walks", graph.vertices);

  size_t num_vertices = active_out->size;
  printf(
      "Random walks -  path: %c, mod: %ld, walks_per_source: %ld, niters: %ld, "
      "memory_bytes: %ld, num vertices of the graph %d\n",
      path, mod, walks_per_source, niters, memory_bytes, num_vertices);
  unsigned* walks_count = (unsigned*)malloc(sizeof(unsigned) * num_vertices);

  graph.set_vertex_data_bytes(graph.vertices * sizeof(VertexId));

  active_out->fill();
  VertexId active_vertices = graph.stream_vertices<VertexId>([&](VertexId i) {
    label[i] = 0;
    walks_count[i] = 0;
    return 1;
  });
  VertexId active_vertices_in_second_round = 0;

  double start_time = get_time();
  int iteration = 0;
  size_t* offset_per_vertex = (size_t*)malloc(graph.vertices * sizeof(size_t));
  size_t* label_for_each_vertex =
      (size_t*)malloc(graph.vertices * sizeof(size_t));
  memset(offset_per_vertex, 0, sizeof(size_t) * graph.vertices);
  memset(offset_per_vertex, 1, sizeof(size_t) * graph.vertices);
  while (iteration <= niters) {
    iteration++;
    std::swap(active_in, active_out);
    active_out->clear();
    graph.hint(label);
    printf("%7d: %d\n", iteration, active_vertices);
    active_vertices = graph.stream_edges<VertexId>(
        [&](Edge& e) {
          if (iteration == 1) {
            if (e.source % mod == 0) {
              __sync_bool_compare_and_swap(&label[e.source], label[e.source],
                                           iteration);
              active_out->set_bit(e.source);
              return 1;
            }
          } else {
            if (label[e.source] != iteration - 1) return 0;
            if (walks_count[e.source] < walks_per_source) {
              if (Drop(e.target)) return 0;
              __sync_bool_compare_and_swap(&label[e.target], label[e.target],
                                           iteration);
              write_add(&walks_count[e.source], (unsigned)1);
              active_out->set_bit(e.target);
              active_second_in->set_bit(e.source);
              write_add(&active_vertices_in_second_round, 1);
              return 1;
            }
          }
          return 0;
        },
        active_in);

    while (active_vertices_in_second_round > 0) {
      active_vertices_in_second_round = graph.stream_edges<VertexId>(
          [&](Edge& e) {
            if (walks_count[e.source] < walks_per_source) {
              if (Drop(e.target)) return 0;
              __sync_bool_compare_and_swap(&label[e.target], label[e.target],
                                           iteration);
              write_add(&walks_count[e.source], (unsigned)1);
              return 1;
            }
            return 0;
          },
          active_second_in);
      printf("  %7d completion round: %d\n", iteration,
             active_vertices_in_second_round);
    }
    active_second_in->clear();

    graph.stream_vertices<VertexId>([&](VertexId i) {
      walks_count[i] = 0;
      return 1;
    });
  }

  double end_time = get_time();

  printf("RW elapsed  %.2f seconds\n", end_time - start_time);

  return 0;
}