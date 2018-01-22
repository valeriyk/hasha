#include <assert.h>
#include "hasha.h"
#include <stdio.h>
#include <stdlib.h>
 

void hasha_init_block (hasha_block_t *block_ptr, char *name, size_t id, size_t mst_ports_num, size_t slv_ports_num) {
	
	printf ("Running hasha_init_block... ");
	
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

	printf ("OKAY! block name: %s, id: %zu, has %zu master ports (%zu used) and %zu slave ports (%zu used)\n", \
		block_ptr->name, block_ptr->id, \
		block_ptr->mst_ports_num, block_ptr->mst_ports_used, \
		block_ptr->slv_ports_num, block_ptr->slv_ports_used);
}

void hasha_link_blocks (hasha_block_t *mst_blk_ptr, size_t mst_port_idx, hasha_block_t *slv_blk_ptr, size_t slv_port_idx) {
	
	printf ("Running hasha_link_blocks... ");
	
	assert (mst_blk_ptr != NULL);
	assert (slv_blk_ptr != NULL);

	if (mst_port_idx < mst_blk_ptr->mst_ports_num) {
		if (mst_blk_ptr->mst_port_ptr[mst_port_idx].addr.block_ptr != NULL) {
			printf ("\n\tWARNING: overriding previous connection of a master port %zu!\n", mst_port_idx);
		}
		else {
			mst_blk_ptr->mst_ports_used++;
		}
		mst_blk_ptr->mst_port_ptr[mst_port_idx].addr.block_ptr = slv_blk_ptr;
		mst_blk_ptr->mst_port_ptr[mst_port_idx].addr.port_idx  = slv_port_idx;
	}
	else {
		printf ("\n\tERROR: not enough spare master ports in %s (id = %zu)!\n", mst_blk_ptr->name, mst_blk_ptr->id);
		return;
	}

	if (slv_port_idx < slv_blk_ptr->slv_ports_num) {
		if (slv_blk_ptr->slv_port_ptr[slv_port_idx].addr.block_ptr != NULL) {
			printf ("\n\tWARNING: overriding previous connection of a slave port %zu!\n", slv_port_idx);
		}
		else {
			slv_blk_ptr->slv_ports_used++;
		}
		slv_blk_ptr->slv_port_ptr[slv_port_idx].addr.block_ptr = mst_blk_ptr;
		slv_blk_ptr->slv_port_ptr[slv_port_idx].addr.port_idx  = mst_port_idx;
	}
	else {
		printf ("\n\tERROR: not enough spare slave ports in %s (id = %zu)!\n", slv_blk_ptr->name, slv_blk_ptr->id);
		return;
	}
		
	printf ("OKAY! link: {%s (id = %zu)}.master_port[%zu] ---> {%s (id = %zu)}.slave_port[%zu]\n",
		mst_blk_ptr->name, mst_blk_ptr->id, mst_port_idx,
		slv_blk_ptr->name, slv_blk_ptr->id, slv_port_idx);
	/*printf ("\t\t%s (id = %zu): total master ports = %zu (%zu used), total slave ports = %zu (%zu used)\n", \
		mst_blk_ptr->name, mst_blk_ptr->id, mst_blk_ptr->mst_ports_num, mst_blk_ptr->mst_ports_used, mst_blk_ptr->slv_ports_num, mst_blk_ptr->slv_ports_used);
	printf ("\t\t%s (id = %zu): total master ports = %zu (%zu used), total slave ports = %zu (%zu used)\n", \
		slv_blk_ptr->name, slv_blk_ptr->id, slv_blk_ptr->mst_ports_num, slv_blk_ptr->mst_ports_used, slv_blk_ptr->slv_ports_num, slv_blk_ptr->slv_ports_used);*/
}


void hasha_tx_req_to_slv (hasha_block_t *mst_blk_ptr, size_t mst_port_idx, bool tx_bit) {
	
	assert (mst_blk_ptr != NULL);
	
	hasha_block_t *slv_blk_ptr  = mst_blk_ptr->mst_port_ptr[mst_port_idx].addr.block_ptr;
	size_t         slv_port_idx = mst_blk_ptr->mst_port_ptr[mst_port_idx].addr.port_idx;
	
	// slv_blk_ptr may be NULL, in this case do nothing but return
	if (slv_blk_ptr == NULL) {
		return;
	}	
	
	pthread_mutex_lock     (&slv_blk_ptr->mutex);
	slv_blk_ptr->slv_port_ptr[slv_port_idx].req = tx_bit;
	pthread_cond_broadcast (&slv_blk_ptr->event);
	pthread_mutex_unlock   (&slv_blk_ptr->mutex);
}

void hasha_tx_ack_to_mst (hasha_block_t *slv_blk_ptr, size_t slv_port_idx, bool tx_bit) {
	
	assert (slv_blk_ptr != NULL);
		
	hasha_block_t *mst_blk_ptr  = slv_blk_ptr->slv_port_ptr[slv_port_idx].addr.block_ptr;
	size_t         mst_port_idx = slv_blk_ptr->slv_port_ptr[slv_port_idx].addr.port_idx;
	
	// mst_blk_ptr may be NULL, in this case do nothing but return
	if (mst_blk_ptr == NULL) {
		return;
	}
	
	pthread_mutex_lock     (&mst_blk_ptr->mutex);
	mst_blk_ptr->mst_port_ptr[mst_port_idx].ack = tx_bit;
	pthread_cond_broadcast (&mst_blk_ptr->event);
	pthread_mutex_unlock   (&mst_blk_ptr->mutex);
}

void hasha_wait_req_from_mst (hasha_block_t *slv_blk_ptr, size_t slv_port_idx, bool rx_bit) {
	
	assert (slv_blk_ptr != NULL);
	
	// remote block_ptr may be NULL, in this case do nothing but return
	if (slv_blk_ptr->slv_port_ptr[slv_port_idx].addr.block_ptr == NULL) {
		return;
	}
		
	pthread_mutex_lock (&slv_blk_ptr->mutex);
	while (slv_blk_ptr->slv_port_ptr[slv_port_idx].req != rx_bit) {
		pthread_cond_wait (&slv_blk_ptr->event, &slv_blk_ptr->mutex);
	}
	pthread_mutex_unlock  (&slv_blk_ptr->mutex);
}

void hasha_wait_ack_from_slv (hasha_block_t *mst_blk_ptr, size_t mst_port_idx, bool rx_bit) {
	
	assert (mst_blk_ptr != NULL);

	// remote block_ptr may be NULL, in this case do nothing but return
	if (mst_blk_ptr->mst_port_ptr[mst_port_idx].addr.block_ptr == NULL) {
		return;
	}
	
	pthread_mutex_lock (&mst_blk_ptr->mutex);
	while (mst_blk_ptr->mst_port_ptr[mst_port_idx].ack != rx_bit) {
		pthread_cond_wait (&mst_blk_ptr->event, &mst_blk_ptr->mutex);
	}
	pthread_mutex_unlock  (&mst_blk_ptr->mutex);
}

void hasha_notify_slv (hasha_block_t *mst_blk_ptr, size_t mst_port_idx) {
	hasha_tx_req_to_slv (mst_blk_ptr, mst_port_idx, true);
	hasha_wait_ack_from_slv (mst_blk_ptr, mst_port_idx, true);
	hasha_tx_req_to_slv (mst_blk_ptr, mst_port_idx, false);
	hasha_wait_ack_from_slv (mst_blk_ptr, mst_port_idx, false);
}

void hasha_wait_for_mst (hasha_block_t *slv_blk_ptr, size_t slv_port_idx) {
	hasha_wait_req_from_mst (slv_blk_ptr, slv_port_idx, true);
	hasha_tx_ack_to_mst (slv_blk_ptr, slv_port_idx, true);
	hasha_wait_req_from_mst (slv_blk_ptr, slv_port_idx, false);
	hasha_tx_ack_to_mst (slv_blk_ptr, slv_port_idx, false);
}
