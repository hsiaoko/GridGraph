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
#include "core/bitmap.hpp"
#include <stdlib.h>



struct MatchSets {
      Bitmap* indicator_ = nullptr;
      Bitmap** sim_sets_ = nullptr;
      size_t x_ = 0;
      size_t y_ = 0;
            
      MatchSets(const size_t x, const size_t y, const bool init = false) {
          x_ = x;
          y_ = y;
          indicator_ = new Bitmap(x);
          indicator_->clear();
          sim_sets_ = (Bitmap**)malloc(sizeof(Bitmap*) * x);
          
          if (!init)
              for (size_t i = 0; i < x; i++) sim_sets_[i] = nullptr;
          else {
              for (size_t i = 0; i < x; i++) {
                  sim_sets_[i] = new Bitmap(y);
                  sim_sets_[i]->clear();
              }
          }
      }

      ~MatchSets() {
          if (indicator_ != nullptr) delete indicator_;
          if (sim_sets_ != nullptr) {
              for (size_t i = 0; i < x_; i++)
                  if (sim_sets_[i] == nullptr) delete sim_sets_[i];
              free(sim_sets_);
          }
      }
};

struct Vertex {
    unsigned vid_;
    size_t indegree_ = 0;
    size_t outdegree_ = 0;
    unsigned label_;
    unsigned *out_edges_ = nullptr;
};

struct ImmutableCSR {
    size_t num_vertexes_ = 0;
    size_t num_edges_ = 0;
    Vertex ** vertexes_ = nullptr;
};


ImmutableCSR* InitPatternNCluque(size_t n) {
    ImmutableCSR * graph = new ImmutableCSR;
    graph->num_vertexes_ = n;
    graph->num_edges_ = n * (n - 1);
    graph->vertexes_ = (Vertex **)malloc(sizeof(Vertex*) * graph->num_vertexes_);
    for(size_t i = 0; i < graph->num_vertexes_; i++) {
       Vertex *vertex = new Vertex;
       vertex->vid_ = i;
       vertex->label_ = rand() % 5;
       vertex->indegree_ = n-1;
       vertex->outdegree_ = n-1;
       vertex->out_edges_ = (unsigned *)malloc(sizeof(unsigned) * n-1);
       for(size_t j = 0; j < graph->num_vertexes_; j++){
           if(i == j) continue;
           vertex->out_edges_[j] = i;
       }
       graph->vertexes_[i] = vertex;
    }

    return graph;    
}

int main(int argc, char ** argv) {
	if (argc<2) {
		fprintf(stderr, "usage: wcc [path] [memory budget in GB]\n");
		exit(-1);
	}
	double start_time = get_time();
	std::string path = argv[1];
	long memory_bytes = (argc>=3)?atol(argv[2])*1024l*1024l*1024l:8l*1024l*1024l*1024l;

	Graph graph(path);
	graph.set_memory_bytes(memory_bytes);
	Bitmap * active_in = graph.alloc_bitmap();
	Bitmap * active_out = graph.alloc_bitmap();
	BigVector<VertexId> label(graph.path+"/label", graph.vertices);
	graph.set_vertex_data_bytes( graph.vertices * sizeof(VertexId) );

	active_out->fill();
	VertexId active_vertices = graph.stream_vertices<VertexId>([&](VertexId i){
		label[i] = rand() % 5;
		return 1;
	});
    
    int n = 5;
    ImmutableCSR * pattern = InitPatternNCluque(n);
    MatchSets match_set(active_vertices, n);
    
	int iteration = 0;
	while (active_vertices!=0) {
		iteration++;
		printf("%7d: %d\n", iteration, active_vertices);
		std::swap(active_in, active_out);
		active_out->clear();
		graph.hint(label);
		active_vertices = graph.stream_edges<VertexId>([&, &pattern, &match_set](Edge & e){

                // find candidates
                if(iteration == 0) {
                    bool tag = false;
                    for(size_t pv = 0; pv < pattern->num_vertexes_; pv++){
                        if(pattern->vertexes_[pv]->label_ == label[e.source]) {
                            match_set.indicator_->set_bit(e.source);
                            match_set.sim_sets_[e.source]->set_bit(pv);
                            printf("%d match %d\n", e.source, pv);
                            tag = true;
                        }
                    }
                    
                    
                    if(tag) return 1;

                    else return 0;

                    
                    
                    
                    // filter candidates
                    } else {
                            
                
                    }

                if (label[e.source]<label[e.target]) {
                    if (write_min(&label[e.target], label[e.source])) {
                        active_out->set_bit(e.target);
                        return 1;
                    }
                }
			return 0;
		}, active_in);
	}
	double end_time = get_time();

	BigVector<VertexId> label_stat(graph.path+"/label_stat", graph.vertices);
	label_stat.fill(0);
	graph.stream_vertices<VertexId>([&](VertexId i){
		write_add(&label_stat[label[i]], 1);
		return 1;
	});
	VertexId components = graph.stream_vertices<VertexId>([&](VertexId i){
		return label_stat[i]!=0;
	});
	printf("%d components found in %.2f seconds\n", components, end_time - start_time);

    for(size_t i=0;i <35; i++){
        printf("%d, ", label[i]);
    }
	return 0;
}
