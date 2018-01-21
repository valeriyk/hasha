#include <assert.h>
#include "hasha.h"
#include <stdio.h>
#include <stdlib.h>
 

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
	
	assert (block_ptr != NULL);
	
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
	
	assert (block_ptr != NULL);
	
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

