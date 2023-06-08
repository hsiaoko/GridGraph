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
		label[i] = 0;
		return 1;
	});
    for(size_t i=0;i <100; i++){
        printf("%d, ", label[i]);
    }
    printf("\n");
        
	int iteration = 0;
    
    VertexId upper_bound = active_vertices;
    VertexId inc_upper_bound = 0;

	while (active_vertices!=0) {
	//while (iteration < 5) {
    //for(size_t i=0;i <33; i++){
    //    printf("%d, \n", label[3063529]);
    //}
		iteration++;
		printf("%7d: %d, %d\n", iteration, active_vertices, upper_bound);
        std::swap(active_in, active_out);
		active_out->clear();
        //active_in->fill();
		graph.hint(label);
		active_vertices = 0; 
        graph.stream_edges<VertexId>([&](Edge & e){
                //printf("src: %d label: %d -  target%d label:%d\n", e.source, label[e.source], e.target, label[e.target]);
			if (e.source < e.target && e.source < upper_bound) {
                    if(label[e.target] == label[e.source]) {
                        label[e.source] = label[e.target]+1;
					    active_out->set_bit(e.source);
                        write_max(&inc_upper_bound, e.source);
                    }
			}
			return 0;
		}, active_in);
        active_vertices = active_out->get_num_bit();
        write_min(&upper_bound, inc_upper_bound);
        inc_upper_bound = 0;
	}
	double end_time = get_time();

	BigVector<VertexId> label_stat(graph.path+"/label_stat", graph.vertices);
	label_stat.fill(0);
	graph.stream_vertices<VertexId>([&](VertexId i){
		
            write_add(&label_stat[label[i]], 1);
		
            return 1;
	
            });
	VertexId colors = graph.stream_vertices<VertexId>([&](VertexId i){
		return label_stat[i]!=0;
	});
	printf("%d colors found in %.2f seconds\n", colors, end_time - start_time);

    for(size_t i=0;i <100; i++){
        printf("%d, ", label[i]);
    }
	return 0;
}
