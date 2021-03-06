

#include <iostream>
#include "tbb/tbb.h"

#define RADIUS 3
#define BLOCK_SIZE 32
#define NELEMENTS 1048576

struct dim3 {
  unsigned int x, y, z;
  dim3(unsigned x, unsigned y = 1, unsigned z = 1)
      : x(x), y(y), z(z) {}
};

//This function is the translation of the former cuda kernel!
void stencil_1d(int * in, int * out, dim3 gridDim, dim3 blockDim, dim3 blockIdx) {
	dim3 threadIdx(0,0,0);
	int tid;
	int numThreads = 32;
	int g_index[numThreads];
	int s_index[numThreads];
	int result[numThreads];
	int offset[numThreads];


    int temp[BLOCK_SIZE + 2 * RADIUS];
    //thread_loop_begin
    for(threadIdx.z=0; threadIdx.z < blockDim.z; threadIdx.z++){
    	for(threadIdx.y=0; threadIdx.y < blockDim.y; threadIdx.y++){
    		for(threadIdx.x=0; threadIdx.x < blockDim.x; threadIdx.x++){
    			tid=threadIdx.x + threadIdx.y*blockDim.x + threadIdx.z*blockDim.y;

     
    			g_index[tid] = threadIdx.x + blockIdx.x * blockDim.x;
    			s_index[tid] = threadIdx.x + RADIUS;

    			// Read input elements into shared memory
    			temp[s_index[tid]] = in[g_index[tid]];
     
    			if (threadIdx.x < RADIUS) {
     
     
    				temp[s_index[tid] - RADIUS] = g_index[tid] - RADIUS >= 0? in[g_index[tid] - RADIUS]: 0;
         
         
    				temp[s_index[tid] + BLOCK_SIZE] = g_index[tid] + BLOCK_SIZE < NELEMENTS ? in[g_index[tid] + BLOCK_SIZE]: 0;
    			}

  
    		}
    	}
    }//thread_loop_end
    //__syncthreads();
    //thread_loop_begin
    for(threadIdx.z=0; threadIdx.z < blockDim.z; threadIdx.z++){
    	for(threadIdx.y=0; threadIdx.y < blockDim.y; threadIdx.y++){
    		for(threadIdx.x=0; threadIdx.x < blockDim.x; threadIdx.x++){
    			tid=threadIdx.x + threadIdx.y*blockDim.x + threadIdx.z*blockDim.y;


    			// Apply the stencil
    			result[tid] = 0;
     
    			for(offset[tid] = -RADIUS ; offset[tid] <= RADIUS ; offset[tid]++)
    				result[tid] += temp[s_index[tid] + offset[tid]];

    			// Store the result
    			out[g_index[tid]] = result[tid];
    			//std::cout << "val " << tid  << " " << out[g_index[tid]] << "\n";
    		}
    	}
    }
}

//Hand written "host" code, the ideal translation of the former host code in
// the CUDA version should be like this
int main(void) {
	int * h_in, *h_out;
	size_t bytes = NELEMENTS*sizeof(int);
	h_in = (int*)malloc(bytes);
    h_out = (int*)malloc(bytes);

    //initialization
    for( int i = 0; i < NELEMENTS; i++ ) {
      h_in[i] = 1;
    }

    dim3 blockSize(32,1,1);
    dim3 gridSize((int)ceil((float)NELEMENTS/blockSize.x), 1, 1);

    //translated parallel
    //???
    tbb::tick_count t0 = tbb::tick_count::now();
    tbb::parallel_for(0, (int) gridSize.z, 1, [=](int z) {
    	tbb::parallel_for(0, (int) gridSize.y, 1, [=](int y) {
    		tbb::parallel_for(0, (int) gridSize.x, 1, [=](int x) {
    			stencil_1d(h_in, h_out, gridSize, blockSize, dim3(x,y,z));
    		});
    	});
    });
    tbb::tick_count t1 = tbb::tick_count::now();
    printf("time for action = %g seconds\n", (t1-t0).seconds() );
    //std::cout << "work took" <<  (t1-t0).seconds() << "seconds\n";

    //for(int j=0; j < NELEMENTS; j++){
    //	std::cout << "h_out[" << j << "]= " << h_out[j] << "\n";
    //}


	free(h_in);
	free(h_out);
	return 0;
}
