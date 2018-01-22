#pragma once

#include <pthread.h>
#include <stdbool.h>



typedef struct hasha_block_t hasha_block_t;

typedef struct hasha_addr_t {
	hasha_block_t *block_ptr;
	size_t         port_idx;
} hasha_addr_t;

typedef struct hasha_port_t {
	bool         req;
	bool         ack;
	hasha_addr_t addr;
} hasha_port_t;

struct hasha_block_t {
	char             *name;
	size_t            id;
	size_t            mst_ports_num;
	size_t            slv_ports_num;
	size_t            mst_ports_used;
	size_t            slv_ports_used;
	hasha_port_t     *mst_port_ptr;
	hasha_port_t     *slv_port_ptr;
	
	pthread_cond_t    event;
	pthread_mutex_t   mutex;
};

void hasha_init_block     (hasha_block_t *block_ptr, char *name, size_t id, size_t mst_ports_num, size_t slv_ports_num);
void hasha_link_blocks    (hasha_block_t *mst_ptr, size_t mst_lane_idx,
                           hasha_block_t *slv_ptr, size_t slv_lane_idx);
void hasha_notify_slv     (hasha_block_t *mst_ptr, size_t mst_port_idx);
void hasha_wait_for_mst   (hasha_block_t *slv_ptr, size_t slv_port_idx);
