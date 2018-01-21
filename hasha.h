#include <pthread.h>
#include <stdbool.h>




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

void init_hw_block (hw_block_t *block_ptr, char *name, size_t id, size_t numof_hasha_lanes);

void init_hasha (hw_hasha_lane_t *hasha_ptr);

//void connect_block (hw_block_t *block_ptr)

void connect_hasha (
		hw_hasha_lane_t *hasha_ptr,
		hw_block_t      *src_ptr,
		size_t           src_lane_num,
		hw_block_t      *dest_ptr,
		size_t           dest_lane_num);

void hasha_notify   (hw_block_t *this_ptr, size_t lane_num);
void hasha_wait_for (hw_block_t *this_ptr, size_t lane_num);
