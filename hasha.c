#include <assert.h>
#include "hasha.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
 

void hasha_run (hasha_block_t *reset_ctrl_blk) {
	
	while (1) {

		// wait for any request
		hasha_req_t r = hasha_wait_for_event (this_ptr);
		
		if (r.valid && (p.port_idx == HASHA_RESET_CTRL_REQ)) {
			printf ("\tGlobal reset request detected...\n", this_ptr->id);
			return NULL;
		}
	}
}

hasha_block_t hasha_new_block (char *name, size_t id, size_t ports_num, void *top_proc (void *p)) {
	
	printf ("Running hasha_init_block... ");
	
	hasha_block_t blk;
	blk.name = name;
	blk.id   = id;
	
	blk.ports_num  = ports_num;
	blk.ports_used = 0;
	
	if (ports_num > 0) {
		
		blk.port_arr = calloc (ports_num, sizeof (hasha_port_t));
		
		for (size_t i = 0; i < ports_num; i++) {
			blk.port_arr[i].type                       = UNUSED;
			blk.port_arr[i].this_port_addr.block_ptr   = block_ptr;
			blk.port_arr[i].this_port_addr.port_idx    = i;
			blk.port_arr[i].return_port_addr.block_ptr = NULL;
			blk.port_arr[i].return_port_addr.port_idx  = 0;
			blk.port_arr[i].pending                    = false;
			blk.port_arr[i].resp_type                  = ACK;
			blk.port_arr[i].handler                    = NULL;
		}
	}
	else {
		blk.port_arr = NULL;
	}
	
	pthread_condattr_t ca;
	pthread_condattr_init (&ca);
	pthread_cond_init (&blk.event, &ca);
	
	pthread_mutexattr_t ma;
	pthread_mutexattr_init (&ma);
	pthread_mutex_init (&blk.mutex, &ma);

	if (top_proc != NULL) {
		pthread_create (&blk.thread, NULL, top_proc, block_ptr);
		pthread_detach (blk.thread);
	}

	printf ("OKAY! block name: %s %zu, has %zu ports (%zu used)\n", \
		blk.name, blk.id, blk.ports_num, blk.ports_used);
}
/*
void hasha_link (hasha_block_t *blk_ptr, size_t port_idx, hasha_block_t *return_blk_ptr, size_t return_port_idx, hasha_port_type_t port_type, hasha_resp_type_t resp_type) {
	
	assert (blk_ptr != NULL);

	char *port_type_msg = (port_type == MASTER) ? " master" : " slave";
	
	if (port_idx < blk_ptr->ports_num) {
		if (blk_ptr->port_arr[port_idx].return_port_addr.block_ptr != NULL) {
			printf ("\n\tWARNING: overriding previous connection of a%s port %zu!\n", port_type_msg, port_idx);
		}
		else {
			blk_ptr->ports_used++;
		}
		blk_ptr->port_arr[port_idx].return_port_addr.block_ptr = return_blk_ptr;
		blk_ptr->port_arr[port_idx].return_port_addr.port_idx  = return_port_idx;
		
		blk_ptr->port_arr[port_idx].type      = port_type;
		blk_ptr->port_arr[port_idx].resp_type = resp_type;
	}
	else {
		printf ("\n\tERROR: not enough spare%s ports in %s (id = %zu)!\n", port_type_msg, blk_ptr->name, blk_ptr->id);
		return;
	}
}

void hasha_link_blocks (hasha_block_t *mst_blk_ptr, size_t mst_port_idx, hasha_block_t *slv_blk_ptr, size_t slv_port_idx, hasha_resp_type_t resp_type) {
	
	printf ("Running hasha_link_blocks... ");
	
	assert (mst_blk_ptr != NULL);
	assert (slv_blk_ptr != NULL);

	hasha_link (mst_blk_ptr, mst_port_idx, slv_blk_ptr, slv_port_idx, MASTER, resp_type);
	hasha_link (slv_blk_ptr, slv_port_idx, mst_blk_ptr, mst_port_idx, SLAVE,  resp_type);
	
	
	char *loopback_msg  = (mst_blk_ptr == slv_blk_ptr) ? "loopback link" : "         link";
	char *resp_type_msg = (resp_type == ACK) ? "-[ACK]-" : "[NOACK]";
	printf ("OKAY! %s: {MASTER %s %zu}.port[%zu] --%s--> {SLAVE %s %zu}.port[%zu]\n",
		loopback_msg,  mst_blk_ptr->name, mst_blk_ptr->id, mst_port_idx,
		resp_type_msg, slv_blk_ptr->name, slv_blk_ptr->id, slv_port_idx);
}
*/
void hasha_link2 (hasha_port2_t *mst_port_ptr, hasha_port2_t *slv_port_ptr, hasha_port_type_t port_type, hasha_resp_type_t resp_type) {
	
	assert (blk_ptr != NULL);

	char *port_type_msg = (port_type == MASTER) ? " master" : " slave";
	
	if (port_idx < blk_ptr->ports_num) {
		if (blk_ptr->port_arr[port_idx].return_port_addr.block_ptr != NULL) {
			printf ("\n\tWARNING: overriding previous connection of a%s port %zu!\n", port_type_msg, port_idx);
		}
		else {
			blk_ptr->ports_used++;
		}
		blk_ptr->port_arr[port_idx].return_port_addr.block_ptr = return_blk_ptr;
		blk_ptr->port_arr[port_idx].return_port_addr.port_idx  = return_port_idx;
		
		blk_ptr->port_arr[port_idx].type      = port_type;
		blk_ptr->port_arr[port_idx].resp_type = resp_type;
	}
	else {
		printf ("\n\tERROR: not enough spare%s ports in %s (id = %zu)!\n", port_type_msg, blk_ptr->name, blk_ptr->id);
		return;
	}
}

void hasha_link_ports (hasha_port2_t *mst_port_ptr, hasha_port2_t *slv_port_ptr, hasha_resp_type_t resp_type) {
	
	printf ("Running hasha_link_blocks... ");
	
	assert (mst_port_ptr != NULL);
	assert (slv_port_ptr != NULL);

	hasha_link2 (mst_port_ptr->this_port_addr, slv_port_ptr->this_port_addr, MASTER, resp_type);
	hasha_link2 (slv_port_ptr->this_port_addr, mst_port_ptr->this_port_addr, SLAVE,  resp_type);
	
	char *loopback_msg  = (mst_blk_ptr == slv_blk_ptr) ? "loopback link" : "         link";
	char *resp_type_msg = (resp_type == ACK) ? "-[ACK]-" : "[NOACK]";
	printf ("OKAY! %s: {MASTER %s %zu}.port[%zu] --%s--> {SLAVE %s %zu}.port[%zu]\n",
		loopback_msg,  mst_blk_ptr->name, mst_blk_ptr->id, mst_port_idx,
		resp_type_msg, slv_blk_ptr->name, slv_blk_ptr->id, slv_port_idx);
}


// FROM MASTER TO SLAVE:
// hasha_handshake_req (hasha_port_t *mst_port_ptr)
// hasha_write_req     (hasha_port_t *mst_port_ptr, hasha_data_t data)
// hasha_read_req      (hasha_port_t *mst_port_ptr)
// FROM SLAVE TO MASTER:
// hasha_handshake_ack (hasha_port_t *slv_port_ptr)
// hasha_write_ack     (hasha_port_t *slv_port_ptr)
// hasha_read_ack      (hasha_port_t *slv_port_ptr, hasha_data_t data)
// WAIT:
// hasha_wait_for_req  (hasha_port_t *slv_port_ptr)
// hasha_wait_for_ack  (hasha_port_t *mst_port_ptr)
// hasha_wait_for_any_req   (hasha_block_t *blk_ptr)
// hasha_wait_for_any_ack   (hasha_block_t *blk_ptr)
// hasha_wait_for_any_event (hasha_block_t *blk_ptr)

int hasha_send_req (hasha_block_t *mst_blk_ptr, size_t mst_port_idx, hasha_data_t data, size_t data_addr) {
	assert (mst_blk_ptr != NULL);
	assert (mst_blk_ptr->port_arr != NULL);
	assert (mst_blk_ptr->port_arr[mst_port_idx].type == MASTER);
	assert (mst_blk_ptr->port_arr[mst_port_idx].this_port_addr.block_ptr != NULL);
	assert (mst_blk_ptr->port_arr[mst_port_idx].this_port_addr.block_ptr == mst_blk_ptr);
	assert (mst_blk_ptr->port_arr[mst_port_idx].this_port_addr.port_idx  == mst_port_idx);
	
	// I am the master, my return port is the slave port, whose address I need to send the request
	hasha_block_t *slv_blk_ptr  = mst_blk_ptr->port_arr[mst_port_idx].return_port_addr.block_ptr;
	size_t         slv_port_idx = mst_blk_ptr->port_arr[mst_port_idx].return_port_addr.port_idx;
	
	// A pointer to the destination block may be NULL, e.g. if we are the last in the chain,
	//  in this case do nothing but silently return
	if (slv_blk_ptr == NULL) {
		return EXIT_SUCCESS;
	}	
	
	// The slave will need to know where to send an acknowledge (i.e. my address, since I am the master)
	hasha_port_addr_t return_port_addr = mst_blk_ptr->port_arr[mst_port_idx].this_port_addr;
	
	pthread_mutex_lock     (&slv_blk_ptr->mutex);
	slv_blk_ptr->port_arr[slv_port_idx].pending          = true;
	slv_blk_ptr->port_arr[slv_port_idx].data             = data;
	slv_blk_ptr->port_arr[slv_port_idx].data_addr        = data_addr;
	slv_blk_ptr->port_arr[slv_port_idx].return_port_addr = return_port_addr;
	
	pthread_cond_broadcast (&slv_blk_ptr->event);
	pthread_mutex_unlock   (&slv_blk_ptr->mutex);
	
	return EXIT_SUCCESS;
}

int hasha_send_data          (hasha_block_t *mst_blk_ptr, size_t mst_port_idx, hasha_data_t data, size_t data_addr) {
	assert (mst_blk_ptr != NULL);
	assert (mst_blk_ptr->port_arr != NULL);
	assert (mst_blk_ptr->port_arr[mst_port_idx].type == MASTER);
	assert (mst_blk_ptr->port_arr[mst_port_idx].this_port_addr.block_ptr != NULL);
	assert (mst_blk_ptr->port_arr[mst_port_idx].this_port_addr.block_ptr == mst_blk_ptr);
	assert (mst_blk_ptr->port_arr[mst_port_idx].this_port_addr.port_idx  == mst_port_idx);
	
	// I am the master, my return port is the slave port, whose address I need to send the request
	hasha_block_t *slv_blk_ptr  = mst_blk_ptr->port_arr[mst_port_idx].return_port_addr.block_ptr;
	size_t         slv_port_idx = mst_blk_ptr->port_arr[mst_port_idx].return_port_addr.port_idx;
	
	// A pointer to the destination block may be NULL, e.g. if we are the last in the chain,
	//  in this case do nothing but silently return
	if (slv_blk_ptr == NULL) {
		return EXIT_SUCCESS;
	}	
	
	// The slave will need to know where to send an acknowledge (i.e. my address, since I am the master)
	hasha_port_addr_t return_port_addr = mst_blk_ptr->port_arr[mst_port_idx].this_port_addr;
	
	pthread_mutex_lock     (&slv_blk_ptr->mutex);
	slv_blk_ptr->port_arr[slv_port_idx].pending          = true;
	slv_blk_ptr->port_arr[slv_port_idx].data             = data;
	slv_blk_ptr->port_arr[slv_port_idx].data_addr        = data_addr;
	slv_blk_ptr->port_arr[slv_port_idx].return_port_addr = return_port_addr;
	
	pthread_cond_broadcast (&slv_blk_ptr->event);
	pthread_mutex_unlock   (&slv_blk_ptr->mutex);
	
	return EXIT_SUCCESS;
}

int hasha_send_req_and_wait (hasha_block_t *mst_blk_ptr, size_t mst_port_idx, hasha_data_t data, size_t data_addr) {
	
	hasha_send_req (mst_blk_ptr, mst_port_idx, data, data_addr);
	
	bool ack_needed = (mst_blk_ptr->port_arr[mst_port_idx].resp_type == ACK);
	
	if (ack_needed) {

		if (mst_blk_ptr == slv_blk_ptr) {
			printf ("CRITICAL WARNING: Loopback detected for a link with ACK response enabled. Deadlock is possible! Use NOACK.\n");
		}
		
		pthread_mutex_lock (&mst_blk_ptr->mutex);

		while (mst_blk_ptr->port_arr[mst_port_idx].pending != true) {
			pthread_cond_wait (&mst_blk_ptr->event, &mst_blk_ptr->mutex);
		}		
		mst_blk_ptr->port_arr[mst_port_idx].pending = false;
		
		pthread_mutex_unlock  (&mst_blk_ptr->mutex);
	}
	return EXIT_SUCCESS;
}

int hasha_send_ack (hasha_port_t *slv_port_ptr, hasha_data_t data) {
	
	assert (slv_port_ptr != NULL);
	assert (slv_port_ptr->type == SLAVE);
	//~ assert (mst_blk_ptr->port_arr[mst_port_idx].this_port_addr.block_ptr != NULL);
	//~ assert (mst_blk_ptr->port_arr[mst_port_idx].this_port_addr.block_ptr == mst_blk_ptr);
	//~ assert (mst_blk_ptr->port_arr[mst_port_idx].this_port_addr.port_idx  == mst_port_idx);
	
	// I am the slave, my return port is the master port, whose address I need to send the acknowledge
	hasha_block_t *mst_blk_ptr  = slv_port_ptr->return_port_addr.block_ptr;
	size_t         mst_port_idx = slv_port_ptr->return_port_addr.port_idx;
	
	// A pointer to the master block may be NULL, e.g. if we are the first in the chain,
	//  in this case do nothing but silently return
	if (mst_blk_ptr == NULL) {
		return EXIT_SUCCESS;
	}
	
	pthread_mutex_lock     (&mst_blk_ptr->mutex);
	mst_blk_ptr->port_arr[mst_port_idx].pending         = true;
	mst_blk_ptr->port_arr[mst_port_idx].dout            = data;
	//mst_blk_ptr->port_arr[mst_port_idx].data_addr        = data_addr;
	//mst_blk_ptr->port_arr[mst_port_idx].return_port_addr = return_port_addr;
	
	pthread_cond_broadcast (&mst_blk_ptr->event);
	pthread_mutex_unlock   (&mst_blk_ptr->mutex);
	
	//~ if ((mst_blk_ptr == slv_blk_ptr) && (mst_blk_ptr->port_arr[mst_port_idx].resp_type == ACK)) {
		//~ printf ("CRITICAL WARNING: Loopback detected for a link with ACK response enabled. Deadlock is possible! Use NOACK.\n");
	//~ }
	
	//~ bool ack_needed = (mst_blk_ptr->port_arr[mst_port_idx].resp_type == ACK);
	
	//~ if (ack_needed) {

		//~ pthread_mutex_lock (&mst_blk_ptr->mutex);

		//~ while (mst_blk_ptr->port_arr[mst_port_idx].pending != true) {
			//~ pthread_cond_wait (&mst_blk_ptr->event, &mst_blk_ptr->mutex);
		//~ }
		
		//~ mst_blk_ptr->port_arr[mst_port_idx].pending = false;
		
		//~ pthread_mutex_unlock  (&mst_blk_ptr->mutex);
	//~ }
	return EXIT_SUCCESS;
}

// WAIT:

enum {HANDSHAKE, READ, WRITE, ANY, NONE} hasha_wait_type_t;
enum {SPECIFIC, ANY, NONE} hasha_wait_port_t;

// hasha_payload_t hasha_wait_req (hasha_port_t *port_ptr)
// hasha_payload_t hasha_wait_ack (hasha_port_t *port_ptr)

// hasha_transaction_t hasha_wait_any_req   (hasha_block_t *blk_ptr)
// hasha_transaction_t hasha_wait_any_ack   (hasha_block_t *blk_ptr)
// hasha_transaction_t hasha_wait_any_event (hasha_block_t *blk_ptr)

hasha_transaction_t hasha_wait_for_any_event (hasha_block_t *blk_ptr) {
	
	assert (blk_ptr != NULL);
	assert (blk_ptr->port_arr != NULL);
		
	hasha_payload_t payload;
	payload.valid         = false;
	payload.data_addr     = 0;
	payload.port_idx      = 0;
	payload.data.void_ptr = NULL;
		
	pthread_mutex_lock (&slv_blk_ptr->mutex);

	// repeat forever for each port of the block,
	// until a port with a pending transaction is found:
	while (1) {
		for (size_t i = 0; i < blk_ptr->ports_num; i++) {
			if (blk_ptr->port_arr[i].pending) {
					
				slv_blk_ptr->port_arr[i].pending = false;
				
				payload.valid     = true;
				payload.data      = slv_blk_ptr->port_arr[i].data;
				payload.data_addr = slv_blk_ptr->port_arr[i].data_addr;
				payload.port_idx  = i;
				
				bool ack_needed = (slv_blk_ptr->port_arr[i].resp_type == ACK);
				
				pthread_mutex_unlock  (&slv_blk_ptr->mutex);
	
				return payload; // non-empty payload here (valid == true)
			}
		}
		pthread_cond_wait (&slv_blk_ptr->event, &slv_blk_ptr->mutex);
	}
}

hasha_payload_t hasha_wait_for_req (hasha_block_t *slv_blk_ptr) {
	
	assert (slv_blk_ptr != NULL);
	assert (slv_blk_ptr->port_arr != NULL);
		
	hasha_payload_t payload;
	payload.valid         = false;
	payload.data_addr     = 0;
	payload.port_idx      = 0;
	payload.data.void_ptr = NULL;
		
	pthread_mutex_lock (&slv_blk_ptr->mutex);

	// repeat forever for each slave port of the slave block:
	while (1) {
		for (size_t i = 0; i < slv_blk_ptr->ports_num; i++) {
			if (slv_blk_ptr->port_arr[i].type == SLAVE) {
				
				// I am the slave and need to know the address of a master,
				//  because this is where I need to send an acknowledge
				hasha_port_addr_t  return_port_addr = slv_blk_ptr->port_arr[i].return_port_addr;
				hasha_block_t     *mst_blk_ptr      = return_port_addr.block_ptr;
				size_t             mst_port_idx     = return_port_addr.port_idx;

				// if there is a pending request to this port:
				// - clear the "pending" flag
				// - extract payload
				// - determine if acknowledge is needed
				// - - if it is needed, set the "pending" bit of the corresponding master port
				// - return the payload
				if (slv_blk_ptr->port_arr[i].pending) {
					
					slv_blk_ptr->port_arr[i].pending = false;
					
					payload.valid     = true;
					payload.data      = slv_blk_ptr->port_arr[i].data;
					payload.data_addr = slv_blk_ptr->port_arr[i].data_addr;
					payload.port_idx  = i;
					
					bool ack_needed = (slv_blk_ptr->port_arr[i].resp_type == ACK);
					
					pthread_mutex_unlock  (&slv_blk_ptr->mutex);
		
					if (ack_needed) {
						pthread_mutex_lock       (&mst_blk_ptr->mutex);
						mst_blk_ptr->port_arr [mst_port_idx].pending = true;
						pthread_cond_broadcast   (&mst_blk_ptr->event);
						pthread_mutex_unlock     (&mst_blk_ptr->mutex);
					}	
					return payload; // non-empty payload here (valid == true)
				}
			}
		}
		pthread_cond_wait (&slv_blk_ptr->event, &slv_blk_ptr->mutex);
	}
}
