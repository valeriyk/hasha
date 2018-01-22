#include <assert.h>
#include "hasha.h"
#include <stdio.h>
#include <stdlib.h>
 

void init_hw_block (hw_block_t *block_ptr, char *name, size_t id, size_t numof_hasha_mst, size_t numof_hasha_slv) {
	
	block_ptr->name = name;
	block_ptr->id   = id;
	
	block_ptr->numof_hasha_mst  = numof_hasha_mst;
	block_ptr->hasha_mst_in_use = 0;	
	
	block_ptr->numof_hasha_slv  = numof_hasha_slv;
	block_ptr->hasha_slv_in_use = 0;	
	
	if (numof_hasha_mst > 0) {
		block_ptr->hasha_mst_ptr = calloc (numof_hasha_mst, sizeof (hw_hasha_mst_t));
	}
	else {
		block_ptr->hasha_mst_ptr = NULL;
	}

	if (numof_hasha_slv > 0) {
		block_ptr->hasha_slv_ptr = calloc (numof_hasha_slv, sizeof (hw_hasha_slv_t));
	}
	else {
		block_ptr->hasha_slv_ptr = NULL;
	}
	
	pthread_condattr_t ca;
	pthread_condattr_init (&ca);
	pthread_cond_init (&block_ptr->event, &ca);
	
	pthread_mutexattr_t ma;
	pthread_mutexattr_init (&ma);
	pthread_mutex_init (&block_ptr->mutex, &ma);

	printf ("init_hw_block executed successfully:\n");
	printf ("\tname:id \"%s\":%zu\n", block_ptr->name, block_ptr->id);
	printf ("\tnumber of lanes: master=%zu, slave=%zu\n", block_ptr->numof_hasha_mst, block_ptr->numof_hasha_slv);
	printf ("\tused lanes: master=%zu, slave=%zu\n", block_ptr->hasha_mst_in_use, block_ptr->hasha_slv_in_use);
}

void connect_blocks (hw_block_t *mst_ptr, size_t mst_lane_num, hw_block_t *slv_ptr, size_t slv_lane_num) {
	
	printf ("Running connect_blocks...");
	
	assert (mst_ptr != NULL);
	assert (slv_ptr != NULL);

	if (mst_lane_num < mst_ptr->numof_hasha_mst) {
		if (mst_ptr->hasha_mst_ptr[mst_lane_num].slv_ptr != NULL) {
			printf ("\n\tWARNING: overriding previous connection of master lane %zu!\n", mst_lane_num);
		}
		else {
			mst_ptr->hasha_mst_in_use++;
		}
		mst_ptr->hasha_mst_ptr[mst_lane_num].slv_ptr = slv_ptr;
		mst_ptr->hasha_mst_ptr[mst_lane_num].slv_lane_num  = slv_lane_num;
	}
	else {
		printf ("\n\tERROR: not enough spare master lanes in %s id=%zu!\n", mst_ptr->name, mst_ptr->id);
		return;
	}

	if (slv_lane_num < slv_ptr->numof_hasha_slv) {
		if (slv_ptr->hasha_slv_ptr[slv_lane_num].mst_ptr != NULL) {
			printf ("\n\tWARNING: overriding previous connection of slave lane %zu!\n", slv_lane_num);
		}
		else {
			slv_ptr->hasha_slv_in_use++;
		}
		slv_ptr->hasha_slv_ptr[slv_lane_num].mst_ptr = mst_ptr;
		slv_ptr->hasha_slv_ptr[slv_lane_num].mst_lane_num  = mst_lane_num;
	}
	else {
		printf ("\n\tERROR: not enough spare slave lanes in %s id=%zu!\n", slv_ptr->name, slv_ptr->id);
		return;
	}
		
	printf ("OKAY:\n");
	printf ("\tMaster (%s, id=%zu, lane=%zu) ---> Slave (%s, id=%zu, lane=%zu)\n",
		mst_ptr->name, mst_ptr->id, mst_lane_num,
		slv_ptr->name, slv_ptr->id, slv_lane_num);
	printf ("\t\t%s id=%zu: total master lanes = %zu (%zu used), total slave lanes = %zu (%zu used)\n", \
		mst_ptr->name, mst_ptr->id, mst_ptr->numof_hasha_mst, mst_ptr->hasha_mst_in_use, mst_ptr->numof_hasha_slv, mst_ptr->hasha_slv_in_use);
	printf ("\t\t%s id=%zu: total master lanes = %zu (%zu used), total slave lanes = %zu (%zu used)\n", \
		slv_ptr->name, slv_ptr->id, slv_ptr->numof_hasha_mst, slv_ptr->hasha_mst_in_use, slv_ptr->numof_hasha_slv, slv_ptr->hasha_slv_in_use);
}

void hw_send_req_to_slv (hw_block_t *block_ptr, size_t mst_lane_num, bool val) {
	
	assert (block_ptr != NULL);
	
	hw_block_t *slv_ptr = block_ptr->hasha_mst_ptr[mst_lane_num].slv_ptr;
	
	if (slv_ptr == NULL) {
		/*printf ("ERROR in hw_send_req_to_slv: cannot send req because the slave \
			block pointer associated with hasha_mst_ptr[%zu] entry is NULL!\n", mst_lane_num);
		printf ("Check for errors and warnings during construction phase.\n");*/
		return;
	}	
	
	pthread_mutex_lock     (&slv_ptr->mutex);
	size_t slv_lane_num = block_ptr->hasha_mst_ptr[mst_lane_num].slv_lane_num;
	slv_ptr->hasha_slv_ptr[slv_lane_num].req = val;
	pthread_cond_broadcast (&slv_ptr->event);
	pthread_mutex_unlock   (&slv_ptr->mutex);
}

void hw_send_ack_to_mst (hw_block_t *block_ptr, size_t slv_lane_num, bool val) {
	
	assert (block_ptr != NULL);
		
	hw_block_t *mst_ptr = block_ptr->hasha_slv_ptr[slv_lane_num].mst_ptr;
	
	if (mst_ptr == NULL) {
		/*printf ("ERROR in hw_send_ack_to_mst: cannot send ack because the master \
			block pointer associated with hasha_slv_ptr[%zu] entry is NULL!\n", slv_lane_num);
		printf ("Check for errors and warnings during construction phase.\n");*/
		return;
	}	
	
	pthread_mutex_lock     (&mst_ptr->mutex);
	size_t mst_lane_num = block_ptr->hasha_slv_ptr[slv_lane_num].mst_lane_num;
	mst_ptr->hasha_mst_ptr[mst_lane_num].ack = val;
	pthread_cond_broadcast (&mst_ptr->event);
	pthread_mutex_unlock   (&mst_ptr->mutex);
}


void hw_wait_for_mst_req (hw_block_t *block_ptr, size_t slv_lane_num, bool val) {
	
	assert (block_ptr != NULL);
	
	if (block_ptr->hasha_slv_ptr[slv_lane_num].mst_ptr == NULL) {
		return; // no need to wait for req if we're the first in the chain
	}
		
	pthread_mutex_lock (&block_ptr->mutex);
	while (block_ptr->hasha_slv_ptr[slv_lane_num].req != val) {
		pthread_cond_wait (&block_ptr->event, &block_ptr->mutex);
	}
	pthread_mutex_unlock  (&block_ptr->mutex);
}

void hw_wait_for_slv_ack (hw_block_t *block_ptr, size_t mst_lane_num, bool val) {
	
	assert (block_ptr != NULL);

	if (block_ptr->hasha_mst_ptr[mst_lane_num].slv_ptr == NULL) {
		return; // no need to wait for req if we're the last in the chain
	}
	
	pthread_mutex_lock (&block_ptr->mutex);
	while (block_ptr->hasha_mst_ptr[mst_lane_num].ack != val) {
		pthread_cond_wait (&block_ptr->event, &block_ptr->mutex);
	}
	pthread_mutex_unlock  (&block_ptr->mutex);
}

void hasha_notify_slv (hw_block_t *this_ptr, size_t lane_num) {
	hw_send_req_to_slv  (this_ptr, lane_num, true);
	hw_wait_for_slv_ack (this_ptr, lane_num, true);
	hw_send_req_to_slv  (this_ptr, lane_num, false);
	hw_wait_for_slv_ack (this_ptr, lane_num, false);
}

void hasha_wait_for_mst (hw_block_t *this_ptr, size_t lane_num) {
	hw_wait_for_mst_req (this_ptr, lane_num, true);
	hw_send_ack_to_mst  (this_ptr, lane_num, true);
	hw_wait_for_mst_req (this_ptr, lane_num, false);
	hw_send_ack_to_mst  (this_ptr, lane_num, false);
}

