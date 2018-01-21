#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h> 
#include <pthread.h>

#define GPU_MAX_USHADERS 4

enum {
	HASHA_HOST_TO_USHADER_MST = 0,
	HASHA_HOST_TO_USHADER_SLV,
	HASHA_HOST_TO_VIDEOCTRL_MST
};

enum {
	HASHA_USHADER_UPSTREAM_MST = 0,
	HASHA_USHADER_UPSTREAM_SLV,
	HASHA_USHADER_DOWNSTREAM_MST,
	HASHA_USHADER_DOWNSTREAM_SLV
};

enum {
	HASHA_VIDEOCTRL_SLV = 0
};

typedef struct hw_block_t hw_block_t;

typedef struct hw_hasha_lane_t {
	bool        req;
	bool        ack;
	hw_block_t *src_ptr;
	hw_block_t *dest_ptr;

	pthread_mutex_t mutex;
} hw_hasha_lane_t;


struct hw_block_t {
	char             *name;
	size_t            id;
	size_t            numof_hasha_lanes;
	size_t            hasha_lanes_in_use;
	hw_hasha_lane_t **hasha_ptrptr;
	
	pthread_cond_t    event;
	pthread_mutex_t   mutex;
};

void init_hw_block (hw_block_t *block_ptr, char *name, size_t id, size_t numof_hasha_lanes) {
	
	block_ptr->name = name;
	block_ptr->id   = id;
	
	block_ptr->numof_hasha_lanes  = numof_hasha_lanes;
	block_ptr->hasha_lanes_in_use = 0;	
	
	// array of pointers to hasha lanes
	if (numof_hasha_lanes > 0) {
		block_ptr->hasha_ptrptr = calloc (numof_hasha_lanes, sizeof (hw_hasha_lane_t *));
	}
	else {
		block_ptr->hasha_ptrptr = NULL;
	}

	pthread_condattr_t ca;
	pthread_condattr_init (&ca);
	pthread_cond_init (&block_ptr->event, &ca);
	
	pthread_mutexattr_t ma;
	pthread_mutexattr_init (&ma);
	pthread_mutex_init (&block_ptr->mutex, &ma);

	printf ("init_hw_block executed successfully:\n");
	printf ("\tname:id \"%s\":%zu\n", block_ptr->name, block_ptr->id);
	printf ("\tnumof lanes: %zu\n", block_ptr->numof_hasha_lanes);
	printf ("\tused lanes: %zu\n",  block_ptr->hasha_lanes_in_use);
}

void init_hasha (hw_hasha_lane_t *hasha_ptr) {

	assert (hasha_ptr != NULL);
	
	hasha_ptr->req = false;
	hasha_ptr->ack = false;
	
	pthread_mutexattr_t ma;
	pthread_mutexattr_init (&ma);
	pthread_mutex_init (&hasha_ptr->mutex, &ma);
}

//void connect_block (hw_block_t *block_ptr)

void connect_hasha (
		hw_hasha_lane_t *hasha_ptr,
		hw_block_t      *src_ptr,
		size_t           src_lane_num,
		hw_block_t      *dest_ptr,
		size_t           dest_lane_num) {
	
	printf ("Running connect_hasha...");
	
	assert (hasha_ptr  != NULL);
	assert (src_ptr    != NULL);
	assert (dest_ptr   != NULL);

	hasha_ptr->src_ptr  = src_ptr;
	hasha_ptr->dest_ptr = dest_ptr;

	if (src_lane_num < hasha_ptr->src_ptr->numof_hasha_lanes) {
		if (hasha_ptr->src_ptr->hasha_ptrptr[src_lane_num] != NULL) {
			printf ("\n\tWARNING: overriding previous connection of source lane %zu!\n", src_lane_num);
		}
		else {
			hasha_ptr->src_ptr->hasha_lanes_in_use++;
		}
		hasha_ptr->src_ptr->hasha_ptrptr[src_lane_num] = hasha_ptr;
	}
	else {
		printf ("\n\tERROR: not enough spare lanes in \"%s\":%zu!\n",
			hasha_ptr->src_ptr->name,
			hasha_ptr->src_ptr->id);
		return;
	}

	if (dest_lane_num < hasha_ptr->dest_ptr->numof_hasha_lanes) {
		if (hasha_ptr->dest_ptr->hasha_ptrptr[dest_lane_num] != NULL) {
			printf ("\n\tWARNING: overriding previous connection of destination lane %zu!\n", dest_lane_num);
		}
		else {
			hasha_ptr->dest_ptr->hasha_lanes_in_use++;
		}
		hasha_ptr->dest_ptr->hasha_ptrptr[dest_lane_num] = hasha_ptr;
	}
	else {
		printf ("\n\tERROR: not enough spare lanes in \"%s\":%zu!\n",
			hasha_ptr->dest_ptr->name,
			hasha_ptr->dest_ptr->id);
		return;
	}
	
	printf ("OKAY:\n");
	printf ("\tsource block:\n");
	printf ("\t\tname:id:lane \"%s\":%zu:%zu\n", hasha_ptr->src_ptr->name, hasha_ptr->src_ptr->id, src_lane_num);
	printf ("\t\tnumof lanes: %zu\n",   hasha_ptr->src_ptr->numof_hasha_lanes);
	printf ("\t\tused lanes: %zu\n",    hasha_ptr->src_ptr->hasha_lanes_in_use);
	printf ("\tdestination block:\n");
	printf ("\t\tname:id:lane \"%s\":%zu:%zu\n", hasha_ptr->dest_ptr->name, hasha_ptr->dest_ptr->id, dest_lane_num);
	printf ("\t\tnumof lanes: %zu\n",   hasha_ptr->dest_ptr->numof_hasha_lanes);
	printf ("\t\tused lanes: %zu\n",    hasha_ptr->dest_ptr->hasha_lanes_in_use);

}

void hw_block_set_req_val (hw_block_t *block_ptr, size_t hasha_lane_num, bool val) {
	if (block_ptr->hasha_ptrptr[hasha_lane_num] == NULL) {
		return;
	}
	if (block_ptr->hasha_ptrptr[hasha_lane_num]->dest_ptr == NULL) {
		return;
	}
	pthread_mutex_lock     (&block_ptr->hasha_ptrptr[hasha_lane_num]->dest_ptr->mutex);
	block_ptr->hasha_ptrptr[hasha_lane_num]->req = val;
	pthread_cond_broadcast (&block_ptr->hasha_ptrptr[hasha_lane_num]->dest_ptr->event);
	pthread_mutex_unlock   (&block_ptr->hasha_ptrptr[hasha_lane_num]->dest_ptr->mutex);
}

void hw_block_set_ack_val (hw_block_t *block_ptr, size_t hasha_lane_num, bool val) {
	if (block_ptr->hasha_ptrptr[hasha_lane_num] == NULL) {
		return;
	}
	if (block_ptr->hasha_ptrptr[hasha_lane_num]->src_ptr == NULL) {
		return;
	}
	pthread_mutex_lock     (&block_ptr->hasha_ptrptr[hasha_lane_num]->src_ptr->mutex);
	block_ptr->hasha_ptrptr[hasha_lane_num]->ack = val;
	pthread_cond_broadcast (&block_ptr->hasha_ptrptr[hasha_lane_num]->src_ptr->event);
	pthread_mutex_unlock   (&block_ptr->hasha_ptrptr[hasha_lane_num]->src_ptr->mutex);
}

void hw_block_wait_for_req_val (hw_block_t *block_ptr, size_t hasha_lane_num, bool val) {
	if (block_ptr->hasha_ptrptr[hasha_lane_num] == NULL) {
		return; // no need to wait for req if we're the first in the chain
	}
	
	if (block_ptr->hasha_ptrptr[hasha_lane_num]->src_ptr == NULL) {
		return; // no need to wait for req if we're the first in the chain
	}
	
	pthread_mutex_lock (&block_ptr->mutex);
	while (block_ptr->hasha_ptrptr[hasha_lane_num]->req != val) {
		pthread_cond_wait (&block_ptr->event, &block_ptr->mutex);
	}
	pthread_mutex_unlock  (&block_ptr->mutex);
}

void hw_block_wait_for_ack_val (hw_block_t *block_ptr, size_t hasha_lane_num, bool val) {
	if (block_ptr->hasha_ptrptr[hasha_lane_num] == NULL) {
		return; // no need to wait for req if we're the last in the chain
	}
	
	if (block_ptr->hasha_ptrptr[hasha_lane_num]->dest_ptr == NULL) {
		return; // no need to wait for req if we're the last in the chain
	}
	
	pthread_mutex_lock (&block_ptr->mutex);
	while (block_ptr->hasha_ptrptr[hasha_lane_num]->ack != val) {
		pthread_cond_wait (&block_ptr->event, &block_ptr->mutex);
	}
	pthread_mutex_unlock  (&block_ptr->mutex);
}

void hasha_notify (hw_block_t *this_ptr, size_t lane_num) {
	hw_block_set_req_val      (this_ptr, lane_num, true);
	hw_block_wait_for_ack_val (this_ptr, lane_num, true);
	hw_block_set_req_val      (this_ptr, lane_num, false);
	hw_block_wait_for_ack_val (this_ptr, lane_num, false);
}

void hasha_wait_for (hw_block_t *this_ptr, size_t lane_num) {
	hw_block_wait_for_req_val (this_ptr, lane_num, true);
	hw_block_set_ack_val      (this_ptr, lane_num, true);
	hw_block_wait_for_req_val (this_ptr, lane_num, false);
	hw_block_set_ack_val      (this_ptr, lane_num, false);
}

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


