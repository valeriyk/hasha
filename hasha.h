#pragma once

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>


typedef enum {MASTER, SLAVE, UNUSED} ptlm_port_type_t;

typedef enum {ACK, NOACK} ptlm_resp_type_t;

typedef struct ptlm_block_t ptlm_block_t;

typedef struct ptlm_port_addr_t {
	hasha_block_t *block_ptr;
	size_t         port_idx;
} ptlm_port_addr_t;
					
typedef union ptlm_data_t {
	float     sfloat;
	double    dfloat;
	bool      boolean;
	uint8_t   uint8;
	 int8_t    int8;
	uint16_t  uint16;
	 int16_t   int16;
	uint32_t  uint32;
	 int32_t   int32;
	uint64_t  uint64;
	 int64_t   int64;
	void      *void_ptr;
} ptlm_data_t;

typedef struct ptlm_port_t {
	
	ptlm_port_type_t  type;      // Master port or slave port
	
	bool               pending;   // Master port: pending ack from slave
	                              // Slave  port: pending req from master
	
	ptlm_resp_type_t  resp_type; // Whether the slave port will acknowledge
	                              // the transaction; failure to do so may cause
	                              // missing transactions (as in the case of
	                              // edge-sensitive interrupts)
	
	ptlm_port_addr_t  this_port_addr;   // {block:port} address of this very port
	
	ptlm_port_addr_t  opp_port_addr; // {block:port} address of the opposite port
	                                 // (the counterpart),
	                                     // specifies the destination of a master port
	                                     // or the source of a slave port.
	
	ptlm_data_t       data;      // Driven by master, consumed by slave
	
	size_t             data_addr; // Optional address of data (e.g.
	                              // register address or memory address).
	                              // Driven by master, consumed by slave
	                              
	void *handler; // function pointer
	
} ptlm_port_t;

/*
typedef struct hasha_signal_port_t {
	
	hasha_port_type_t  type;      // Master port or slave port
	
	bool               pending;   // Master port: pending ack from slave
	                              // Slave  port: pending req from master
	
	hasha_resp_type_t  resp_type; // Whether the slave port will acknowledge
	                              // the transaction; failure to do so may cause
	                              // missing transactions (as in the case of
	                              // edge-sensitive interrupts)
	
	hasha_port_addr_t  this_port_addr;   // {block:port} address of this very port
	
	hasha_port_addr_t  return_port_addr; // {block:port} address of the counterpart,
	                                     // specifies the destination of a master port
	                                     // or the source of a slave port.
} hasha_signal_port_t;


typedef struct hasha_port2_t {
	
	hasha_port_type_t  type;      // Master port or slave port
	bool               bus;       // Am I a bus or a signal?
	hasha_resp_type_t  resp_type; // Whether the slave port will acknowledge
	                              // the transaction; failure to do so may cause
	                              // missing transactions (as in the case of
	                              // edge-sensitive interrupts)
	
	bool               pending;   // Master port: pending ack from slave
	                              // Slave  port: pending req from master
	
	hasha_port_addr_t  this_port_addr;   // {block:port} address of this very port
	
	hasha_port_addr_t  return_port_addr; // {block:port} address of the counterpart,
	                                     // specifies the destination of a master port
	                                     // or the source of a slave port.
	
	hasha_data_t       din;      // Driven by master, consumed by slave
	hasha_data_t       dout;     // Driven by slave, consumed by master
	
	size_t             addr;     // Address of data (e.g. register/memory address).
	                             // Driven by master, consumed by slave
} hasha_port2_t;
*/

//~ typedef struct hasha_payload_t {
	//~ hasha_data_t   data;
	//~ size_t         data_addr;
	//~ size_t         port_idx;
	//~ bool           valid;
//~ } hasha_payload_t;


struct ptlm_proc_t {
	size_t            ports_num;
	size_t            ports_used;
	hasha_port_t     *port_arr;
	
	pthread_cond_t    event;
	pthread_mutex_t   mutex;
	
	pthread_t         thread;
}

struct ptlm_block_t {
	char             *name;
	size_t            id;
	ptlm_proc_t mst;
	ptlm_proc_t slv;
}

/*
struct hasha_block_t {
	char             *name;
	size_t            id;
	size_t            ports_num;
	size_t            ports_used;
	hasha_port_t     *port_arr;
	
	pthread_cond_t    event;
	pthread_mutex_t   mutex;
	
	pthread_t         thread;
};
*/
hasha_block_t hasha_new_block     (char *name, size_t id, size_t ports_num);

//void hasha_link_blocks (hasha_block_t *mst_ptr, size_t mst_lane_idx, hasha_block_t *slv_ptr, size_t slv_lane_idx, hasha_resp_type_t resp_type);
void hasha_link_ports (hasha_port2_t *mst_port_ptr, hasha_port2_t *slv_port_ptr, hasha_resp_type_t resp_type) {
void hasha_run        (hasha_block_t *reset_ctrl_blk);

int             hasha_send_req     (hasha_block_t *mst_blk_ptr, size_t mst_port_idx, hasha_data_t data, size_t data_addr);
hasha_payload_t hasha_wait_for_req (hasha_block_t *slv_blk_ptr);
