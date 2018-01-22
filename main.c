#include <assert.h> 
#include "hasha.h"
#include "main.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>



static void *host_top (void *p) {

	hasha_block_t *this_ptr = (hasha_block_t *) p;
	
	printf ("host started, waiting...\n");
	
	for (size_t i = 0; i < 5; i++) { // for each frame
		printf ("FRAME %zu\n", i);
		for (size_t j = 0; j < 1; j++) { // for each buffer (framebuf, shadowbuf, etc)
			printf ("BUFFER %zu\n", j);
			for (size_t k = 0; k < 2; k++) { // for each shader (vshader, pshader, ..)
				printf ("SHADER %zu\n", k);
				
				hasha_notify_slv   (this_ptr, HASHA_HOST_TO_USHADER_MST);
				hasha_wait_for_mst (this_ptr, HASHA_HOST_TO_USHADER_SLV);
			}
			
			hasha_notify_slv   (this_ptr, HASHA_HOST_TO_VIDEOCTRL_MST);
		}
	}
	
	printf ("all done\n");
	
	return NULL;
}

static void *ushader_top (void *p) {

	hasha_block_t *this_ptr = (hasha_block_t *) p;
	
	printf ("ushader %zu started, waiting...\n", this_ptr->id);
	
	while (1) {
	
		
		// wait for request, then forward the request to the next ushader
		hasha_wait_for_mst (this_ptr, HASHA_USHADER_UPSTREAM_SLV);
		hasha_notify_slv   (this_ptr, HASHA_USHADER_DOWNSTREAM_MST);

		// work done here
		printf ("ushader %zu executing...\n", this_ptr->id);
		// end of work
		
		hasha_wait_for_mst (this_ptr, HASHA_USHADER_DOWNSTREAM_SLV);
		hasha_notify_slv   (this_ptr, HASHA_USHADER_UPSTREAM_MST);
	}
	
	printf ("ushader %zu stopped\n", this_ptr->id);
	
	return NULL;
}

static void *videoctrl_top (void *p) {

	enum {UPSTREAM_LINK = 0};
	
	printf ("videoctrl started, waiting...\n");
	
	hasha_block_t *this_ptr = (hasha_block_t *) p;
	
	while (1) {
	
		hasha_wait_for_mst (this_ptr, HASHA_VIDEOCTRL_SLV);
		
		// work done here
		printf ("Video Controller is drawing a frame...\n");
		// end of work
	}
	
	printf ("videoctrl stopped\n");
	
	return NULL;
}

int main(int argc, char** argv) {
      
    printf ("Constructing the system...\n");
    
    hasha_block_t host_cpu;
	hasha_block_t ushader[GPU_MAX_USHADERS];
	hasha_block_t videoctrl;
	
	hasha_init_block (&host_cpu,  "Host CPU",         DUMMY_ID,      HOST_MAX_HASHA_MST_LANES,      HOST_MAX_HASHA_SLV_LANES);
	hasha_init_block (&videoctrl, "Video Controller", DUMMY_ID, VIDEOCTRL_MAX_HASHA_MST_LANES, VIDEOCTRL_MAX_HASHA_SLV_LANES);
	for (size_t i = 0; i < GPU_MAX_USHADERS; i++) {
		hasha_init_block (&ushader[i], "Unified Shader", i, USHADER_MAX_HASHA_MST_LANES, USHADER_MAX_HASHA_SLV_LANES);
	}
	
	hasha_link_blocks (&host_cpu,   HASHA_HOST_TO_USHADER_MST,  &ushader[0], HASHA_USHADER_UPSTREAM_SLV);
	hasha_link_blocks (&ushader[0], HASHA_USHADER_UPSTREAM_MST, &host_cpu,   HASHA_HOST_TO_USHADER_SLV);
	for (size_t i = 1; i < GPU_MAX_USHADERS; i++) {
		hasha_link_blocks (&ushader[i-1], HASHA_USHADER_DOWNSTREAM_MST, &ushader[i],   HASHA_USHADER_UPSTREAM_SLV);
		hasha_link_blocks (&ushader[i],   HASHA_USHADER_UPSTREAM_MST,   &ushader[i-1], HASHA_USHADER_DOWNSTREAM_SLV);
	}
	hasha_link_blocks (&host_cpu, HASHA_HOST_TO_VIDEOCTRL_MST, &videoctrl, HASHA_VIDEOCTRL_SLV);
	
	printf ("System constructed!\n");
	
	
	pthread_t host_thread;
	pthread_t ushader_thread [GPU_MAX_USHADERS];
	pthread_t videoctrl_thread;
	
	
	// CREATE THREADS
	pthread_create (&host_thread, NULL, host_top, &host_cpu);
	for (size_t i = 0; i < GPU_MAX_USHADERS; i++) {
		pthread_create (&ushader_thread[i], NULL, ushader_top, &ushader[i]);
	}	
	pthread_create (&videoctrl_thread, NULL, videoctrl_top, &videoctrl);
	
	
	// JOIN HOST
	pthread_join (host_thread, NULL);

	// KILL REMAINING THREADS to emulate halt/reset
	for (int i = 0; i < GPU_MAX_USHADERS; i++) {
		pthread_cancel (ushader_thread[i]);
	}	
	pthread_cancel (videoctrl_thread);
	
	// DONE
    return 0;
}


