#include <assert.h> 
#include "hasha.h"
#include "main.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>



static void *host_top (void *p) {

	hw_block_t *this_ptr = (hw_block_t *) p;
	
	printf ("host started, waiting...\n");
	
	for (size_t i = 0; i < 5; i++) { // for each frame
		printf ("FRAME %zu\n", i);
		for (size_t j = 0; j < 1; j++) { // for each buffer (framebuf, shadowbuf, etc)
			printf ("BUFFER %zu\n", j);
			for (size_t k = 0; k < 2; k++) { // for each shader (vshader, pshader, ..)
				printf ("SHADER %zu\n", k);
				
				hasha_notify   (this_ptr, HASHA_HOST_TO_USHADER_MST);				
				hasha_wait_for (this_ptr, HASHA_HOST_TO_USHADER_SLV);
			}
			
			hasha_notify   (this_ptr, HASHA_HOST_TO_VIDEOCTRL_MST);				
		}
	}
	
	printf ("all done\n");
	
	return NULL;
}

static void *ushader_top (void *p) {

	hw_block_t *this_ptr = (hw_block_t *) p;
	
	printf ("ushader %zu started, waiting...\n", this_ptr->id);
	
	while (1) {
	
		
		// wait for request, then forward the request to the next ushader
		hasha_wait_for (this_ptr, HASHA_USHADER_UPSTREAM_SLV);
		hasha_notify   (this_ptr, HASHA_USHADER_DOWNSTREAM_MST);				

				
		// work done here
		printf ("ushader %zu executing...\n", this_ptr->id);
		// end of work
		
		hasha_wait_for (this_ptr, HASHA_USHADER_DOWNSTREAM_SLV);
		hasha_notify   (this_ptr, HASHA_USHADER_UPSTREAM_MST);
	}
	
	printf ("ushader %zu stopped\n", this_ptr->id);
	
	return NULL;
}

static void *videoctrl_top (void *p) {

	enum {UPSTREAM_LINK = 0};
	
	printf ("videoctrl started, waiting...\n");
	
	hw_block_t *this_ptr = (hw_block_t *) p;
	
	while (1) {
	
		hasha_wait_for (this_ptr, HASHA_VIDEOCTRL_SLV);
		
		// work done here
		printf ("Video Controller is drawing a frame...\n");
		// end of work
	}
	
	printf ("videoctrl stopped\n");
	
	return NULL;
}

int main(int argc, char** argv) {
      
    printf ("Constructing the system...\n");

	const size_t    HOST_MAX_HASHA_LANES = 3;
	const size_t USHADER_MAX_HASHA_LANES = 4;
	const size_t VIDEOCTRL_MAX_HASHA_LANES = 1;

	const size_t DUMMY_ID = 0;
	    
    hw_block_t host_cpu;
	hw_block_t ushader[GPU_MAX_USHADERS];
	hw_block_t videoctrl;
	

		
	init_hw_block (&host_cpu, "Host CPU", DUMMY_ID, HOST_MAX_HASHA_LANES);
	for (size_t i = 0; i < GPU_MAX_USHADERS; i++) {
		init_hw_block (&ushader[i], "Unified Shader", i, USHADER_MAX_HASHA_LANES);
	}
	init_hw_block (&videoctrl, "Video Controller", DUMMY_ID, VIDEOCTRL_MAX_HASHA_LANES);
	
	printf ("HW blocks have been instantiated.\n");
	
	hw_hasha_lane_t hasha_host2ushader[GPU_MAX_USHADERS];
	hw_hasha_lane_t hasha_ushader2host[GPU_MAX_USHADERS];
	hw_hasha_lane_t hasha_host2videoctrl;
	
	for (size_t i = 0; i < GPU_MAX_USHADERS; i++) {	
		init_hasha (&hasha_host2ushader[i]);
		init_hasha (&hasha_ushader2host[i]);
	}
	init_hasha (&hasha_host2videoctrl);
	
	connect_hasha (&hasha_host2ushader[0], &host_cpu,   HASHA_HOST_TO_USHADER_MST,  &ushader[0], HASHA_USHADER_UPSTREAM_SLV);
	connect_hasha (&hasha_ushader2host[0], &ushader[0], HASHA_USHADER_UPSTREAM_MST, &host_cpu,   HASHA_HOST_TO_USHADER_SLV);
	for (size_t i = 1; i < GPU_MAX_USHADERS; i++) {
		connect_hasha (&hasha_host2ushader[i], &ushader[i-1], HASHA_USHADER_DOWNSTREAM_MST, &ushader[i],   HASHA_USHADER_UPSTREAM_SLV);
		connect_hasha (&hasha_ushader2host[i], &ushader[i],   HASHA_USHADER_UPSTREAM_MST,   &ushader[i-1], HASHA_USHADER_DOWNSTREAM_SLV);
	}
	connect_hasha (&hasha_host2videoctrl, &host_cpu, HASHA_HOST_TO_VIDEOCTRL_MST, &videoctrl, HASHA_VIDEOCTRL_SLV);
	
	printf ("Handshaking lanes have been instantiated.\n");
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

