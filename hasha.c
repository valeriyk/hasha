#include <assert.h>
#include "hasha.h"
#include <stdio.h>
#include <stdlib.h>
 

void init_hw_block (hasha_block_t *block_ptr, char *name, size_t id, size_t mst_ports_num, size_t slv_ports_num) {
	
	block_ptr->name = name;
	block_ptr->id   = id;
	
	block_ptr->mst_ports_num  = mst_ports_num;
	block_ptr->mst_ports_used = 0;	
	
	block_ptr->slv_ports_num  = slv_ports_num;
	block_ptr->slv_ports_used = 0;	
	
	if (mst_ports_num > 0) {
		block_ptr->mst_port_ptr = calloc (mst_ports_num, sizeof (hasha_port_t));
	}
	else {
		block_ptr->mst_port_ptr = NULL;
	}

	if (slv_ports_num > 0) {
		block_ptr->slv_port_ptr = calloc (slv_ports_num, sizeof (hasha_port_t));
	}
	else {
		block_ptr->slv_port_ptr = NULL;
	}
	
	pthread_condattr_t ca;
	pthread_condattr_init (&ca);
	pthread_cond_init (&block_ptr->event, &ca);
	
	pthread_mutexattr_t ma;
	pthread_mutexattr_init (&ma);
	pthread_mutex_init (&block_ptr->mutex, &ma);

	printf ("init_hw_block executed successfully:\n");
	printf ("\tname:id \"%s\":%zu\n", block_ptr->name, block_ptr->id);
	printf ("\tnumber of lanes: master=%zu, slave=%zu\n", block_ptr->mst_ports_num, block_ptr->slv_ports_num);
	printf ("\tused lanes: master=%zu, slave=%zu\n", block_ptr->mst_ports_used, block_ptr->slv_ports_used);
}

void connect_blocks (hasha_block_t *mst_blk_ptr, size_t mst_port_idx, hasha_block_t *slv_blk_ptr, size_t slv_port_idx) {
	
	printf ("Running connect_blocks...");
	
	assert (mst_blk_ptr != NULL);
	assert (slv_blk_ptr != NULL);

	if (mst_port_idx < mst_blk_ptr->mst_ports_num) {
		if (mst_blk_ptr->mst_port_ptr[mst_port_idx].addr.block_ptr != NULL) {
			printf ("\n\tWARNING: overriding previous connection of master lane %zu!\n", mst_port_idx);
		}
		else {
			mst_blk_ptr->mst_ports_used++;
		}
		mst_blk_ptr->mst_port_ptr[mst_port_idx].addr.block_ptr = slv_blk_ptr;
		mst_blk_ptr->mst_port_ptr[mst_port_idx].addr.port_idx  = slv_port_idx;
	}
	else {
		printf ("\n\tERROR: not enough spare master lanes in %s id=%zu!\n", mst_blk_ptr->name, mst_blk_ptr->id);
		return;
	}

	if (slv_port_idx < slv_blk_ptr->slv_ports_num) {
		if (slv_blk_ptr->slv_port_ptr[slv_port_idx].addr.block_ptr != NULL) {
			printf ("\n\tWARNING: overriding previous connection of slave lane %zu!\n", slv_port_idx);
		}
		else {
			slv_blk_ptr->slv_ports_used++;
		}
		slv_blk_ptr->slv_port_ptr[slv_port_idx].addr.block_ptr = mst_blk_ptr;
		slv_blk_ptr->slv_port_ptr[slv_port_idx].addr.port_idx  = mst_port_idx;
	}
	else {
		printf ("\n\tERROR: not enough spare slave lanes in %s id=%zu!\n", slv_blk_ptr->name, slv_blk_ptr->id);
		return;
	}
		
	printf ("OKAY:\n");
	printf ("\tMaster (%s, id=%zu, lane=%zu) ---> Slave (%s, id=%zu, lane=%zu)\n",
		mst_blk_ptr->name, mst_blk_ptr->id, mst_port_idx,
		slv_blk_ptr->name, slv_blk_ptr->id, slv_port_idx);
	printf ("\t\t%s id=%zu: total master lanes = %zu (%zu used), total slave lanes = %zu (%zu used)\n", \
		mst_blk_ptr->name, mst_blk_ptr->id, mst_blk_ptr->mst_ports_num, mst_blk_ptr->mst_ports_used, mst_blk_ptr->slv_ports_num, mst_blk_ptr->slv_ports_used);
	printf ("\t\t%s id=%zu: total master lanes = %zu (%zu used), total slave lanes = %zu (%zu used)\n", \
		slv_blk_ptr->name, slv_blk_ptr->id, slv_blk_ptr->mst_ports_num, slv_blk_ptr->mst_ports_used, slv_blk_ptr->slv_ports_num, slv_blk_ptr->slv_ports_used);
}

/*
void hw_send_req_to_slv (hasha_block_t *block_ptr, size_t mst_port_idx, bool val) {
	
	assert (block_ptr != NULL);
	
	hasha_block_t *slv_blk_ptr = block_ptr->hasha_mst_blk_ptr[mst_port_idx].slv_blk_ptr;
	
	if (slv_blk_ptr == NULL) {
		// this is not an error, having slv_blk_ptr equal NULL may be valid
		return;
	}	
	
	pthread_mutex_lock     (&slv_blk_ptr->mutex);
	size_t slv_port_idx = block_ptr->hasha_mst_blk_ptr[mst_port_idx].slv_port_idx;
	slv_blk_ptr->hasha_slv_blk_ptr[slv_port_idx].req = val;
	pthread_cond_broadcast (&slv_blk_ptr->event);
	pthread_mutex_unlock   (&slv_blk_ptr->mutex);
}

void hw_send_ack_to_mst (hasha_block_t *block_ptr, size_t slv_port_idx, bool val) {
	
	assert (block_ptr != NULL);
		
	hasha_block_t *mst_blk_ptr = block_ptr->hasha_slv_blk_ptr[slv_port_idx].mst_blk_ptr;
	
	if (mst_blk_ptr == NULL) {
		// this is not an error, having mst_blk_ptr equal NULL may be valid
		return;
	}	
	
	pthread_mutex_lock     (&mst_blk_ptr->mutex);
	size_t mst_port_idx = block_ptr->hasha_slv_blk_ptr[slv_port_idx].mst_port_idx;
	mst_blk_ptr->hasha_mst_blk_ptr[mst_port_idx].ack = val;
	pthread_cond_broadcast (&mst_blk_ptr->event);
	pthread_mutex_unlock   (&mst_blk_ptr->mutex);
}
*/

typedef enum {
	REQ_TO_SLV,
	ACK_TO_MST
} hasha_tx_type_t;

typedef enum {
	REQ_FROM_MST,
	ACK_FROM_SLV
} hasha_rx_type_t;

void hasha_tx (hasha_block_t *block_ptr, size_t src_port_idx, bool val, hasha_tx_type_t tx_type) {
	
	assert (block_ptr != NULL);
	
	hasha_block_t *dest_block_ptr = NULL;
	if (tx_type == REQ_TO_SLV) {
		dest_block_ptr = block_ptr->mst_port_ptr[src_port_idx].addr.block_ptr;
	}
	else if (tx_type == ACK_TO_MST) {
		dest_block_ptr = block_ptr->slv_port_ptr[src_port_idx].addr.block_ptr;
	}
	else {
		printf ("ERROR in hasha_tx: Invalid transaction type\n");
		return;
	}
	
	if (dest_block_ptr == NULL) {
		// this is not an error, destination block ptr may be NULL for some blocks
		return;
	}	
	
	size_t dest_port_idx = 0;
	pthread_mutex_lock     (&dest_block_ptr->mutex);
	if (tx_type == REQ_TO_SLV) {
		dest_port_idx = block_ptr->mst_port_ptr[src_port_idx].addr.port_idx;
		dest_block_ptr->slv_port_ptr[dest_port_idx].req = val;
	}
	else if (tx_type == ACK_TO_MST) {
		dest_port_idx = block_ptr->slv_port_ptr[src_port_idx].addr.port_idx;
		dest_block_ptr->mst_port_ptr[dest_port_idx].ack = val;
	}
	pthread_cond_broadcast (&dest_block_ptr->event);
	pthread_mutex_unlock   (&dest_block_ptr->mutex);
}

static inline void hasha_assert_req_to_slv (hasha_block_t *block_ptr, size_t mst_port_idx) {
	hasha_tx (block_ptr, mst_port_idx, true,  REQ_TO_SLV);
}
static inline void hasha_clear_req_to_slv  (hasha_block_t *block_ptr, size_t mst_port_idx) {
	hasha_tx (block_ptr, mst_port_idx, false, REQ_TO_SLV);
}
static inline void hasha_toggle_req_to_slv (hasha_block_t *block_ptr, size_t mst_port_idx) {
	printf ("hasha_toggle_req_to_slv UNIMPLEMENTED!\n");
}

static inline void hasha_assert_ack_to_mst (hasha_block_t *block_ptr, size_t slv_port_idx) {
	hasha_tx (block_ptr, slv_port_idx, true, ACK_TO_MST);
}
static inline void hasha_clear_ack_to_mst  (hasha_block_t *block_ptr, size_t slv_port_idx) {
	hasha_tx (block_ptr, slv_port_idx, false, ACK_TO_MST);
}
static inline void hasha_toggle_ack_to_mst (hasha_block_t *block_ptr, size_t slv_port_idx) {
	printf ("hasha_toggle_ack_to_mst UNIMPLEMENTED!\n");
}


void hw_wait_for_mst_req (hasha_block_t *block_ptr, size_t slv_port_idx, bool val) {
	
	assert (block_ptr != NULL);
	
	if (block_ptr->slv_port_ptr[slv_port_idx].addr.block_ptr == NULL) {
		return; // no need to wait for req if we're the first in the chain
	}
		
	pthread_mutex_lock (&block_ptr->mutex);
	while (block_ptr->slv_port_ptr[slv_port_idx].req != val) {
		pthread_cond_wait (&block_ptr->event, &block_ptr->mutex);
	}
	pthread_mutex_unlock  (&block_ptr->mutex);
}

void hw_wait_for_slv_ack (hasha_block_t *block_ptr, size_t mst_port_idx, bool val) {
	
	assert (block_ptr != NULL);

	if (block_ptr->mst_port_ptr[mst_port_idx].addr.block_ptr == NULL) {
		return; // no need to wait for req if we're the last in the chain
	}
	
	pthread_mutex_lock (&block_ptr->mutex);
	while (block_ptr->mst_port_ptr[mst_port_idx].ack != val) {
		pthread_cond_wait (&block_ptr->event, &block_ptr->mutex);
	}
	pthread_mutex_unlock  (&block_ptr->mutex);
}


void hasha_rx (hasha_block_t *block_ptr, size_t dest_port_idx, bool val, hasha_rx_type_t rx_type) {
	
	assert (block_ptr != NULL);
	
	hasha_block_t *src_block_ptr = NULL;
	
	if (rx_type == REQ_FROM_MST) {
		src_block_ptr = block_ptr->slv_port_ptr[dest_port_idx].addr.block_ptr;
	}
	else if (rx_type == ACK_FROM_SLV) {
		src_block_ptr = block_ptr->mst_port_ptr[dest_port_idx].addr.block_ptr;
	}
	else {
		printf ("ERROR in hasha_rx: Invalid transaction type\n");
		return;
	}
	
	//~ switch (rx_type ){
		//~ case REQ_FROM_MST: src_block_ptr = block_ptr->hasha_slv_blk_ptr[dest_port_idx].mst_blk_ptr;
		                   //~ break;
		//~ case ACK_FROM_SLV: src_block_ptr = block_ptr->hasha_mst_blk_ptr[dest_port_idx].slv_blk_ptr;
		                   //~ break;
		//~ default:           printf ("ERROR in hasha_rx: Invalid transaction type\n");
		                   //~ return;
	//~ }
	
	if (src_block_ptr == NULL) {
		// this is not an error, source block ptr may be NULL for some blocks
		return;
	}
		
	pthread_mutex_lock (&block_ptr->mutex);
	if (rx_type == REQ_FROM_MST) {
		while (block_ptr->slv_port_ptr[dest_port_idx].req != val) {
			pthread_cond_wait (&block_ptr->event, &block_ptr->mutex);
		}
	}
	else if (rx_type == ACK_FROM_SLV) {
		while (block_ptr->slv_port_ptr[dest_port_idx].ack != val) {
			pthread_cond_wait (&block_ptr->event, &block_ptr->mutex);
		}
	}
	//~ while (1) {
		//~ volatile bool rx_bit;
		//~ switch (rx_type) {
			//~ case REQ_FROM_MST: rx_bit = block_ptr->hasha_slv_blk_ptr[dest_port_idx].req;
			                   //~ break;
			//~ case ACK_FROM_SLV: rx_bit = block_ptr->hasha_slv_blk_ptr[dest_port_idx].ack;
			                   //~ break;
			//~ default:           rx_bit = val;
			                   //~ break;
		//~ }
		
		//~ if (rx_bit == val) {
			//~ break;
		//~ }
		
		//~ pthread_cond_wait (&block_ptr->event, &block_ptr->mutex);
	//~ }
	pthread_mutex_unlock  (&block_ptr->mutex);
}

static inline void hasha_wait_posedge_req_from_mst (hasha_block_t *slv_blk_ptr, size_t slv_port_idx) {
	hasha_rx (slv_blk_ptr, slv_port_idx, true,  REQ_FROM_MST);
}

static inline void hasha_wait_negedge_req_from_mst (hasha_block_t *slv_blk_ptr, size_t slv_port_idx) {
	hasha_rx (slv_blk_ptr, slv_port_idx, false, REQ_FROM_MST);
}

static inline void hasha_wait_posedge_ack_from_slv (hasha_block_t *mst_blk_ptr, size_t mst_port_idx) {
	hasha_rx (mst_blk_ptr, mst_port_idx, true,  ACK_FROM_SLV);
}

static inline void hasha_wait_negedge_ack_from_slv (hasha_block_t *mst_blk_ptr, size_t mst_port_idx) {
	hasha_rx (mst_blk_ptr, mst_port_idx, false, ACK_FROM_SLV);
}




void hasha_notify_slv (hasha_block_t *mst_blk_ptr, size_t mst_port_idx) {
	//hw_send_req_to_slv  (mst_blk_ptr, mst_port_idx, true);
	hasha_assert_req_to_slv (mst_blk_ptr, mst_port_idx);
	
	hw_wait_for_slv_ack (mst_blk_ptr, mst_port_idx, true);
	//hasha_wait_posedge_ack_from_slv (mst_blk_ptr, mst_port_idx);
	
	//hw_send_req_to_slv  (mst_blk_ptr, mst_port_idx, false);
	hasha_clear_req_to_slv  (mst_blk_ptr, mst_port_idx);
	
	hw_wait_for_slv_ack (mst_blk_ptr, mst_port_idx, false);
	//hasha_wait_negedge_ack_from_slv (mst_blk_ptr, mst_port_idx);
}

void hasha_wait_for_mst (hasha_block_t *slv_blk_ptr, size_t slv_port_idx) {
	hw_wait_for_mst_req (slv_blk_ptr, slv_port_idx, true);
	//hasha_wait_posedge_req_from_mst (slv_blk_ptr, slv_port_idx);
	
	//hw_send_ack_to_mst  (slv_blk_ptr, slv_port_idx, true);
	hasha_assert_ack_to_mst (slv_blk_ptr, slv_port_idx);
	
	hw_wait_for_mst_req (slv_blk_ptr, slv_port_idx, false);
	//hasha_wait_negedge_req_from_mst (slv_blk_ptr, slv_port_idx);
	
	//hw_send_ack_to_mst  (slv_blk_ptr, slv_port_idx, false);
	hasha_clear_ack_to_mst (slv_blk_ptr, slv_port_idx);
}

