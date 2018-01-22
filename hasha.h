#include <pthread.h>
#include <stdbool.h>



typedef struct hw_block_t hw_block_t;


typedef struct hw_hasha_mst_t {
	bool        req; 
	bool        ack;
	hw_block_t *slv_ptr;
	size_t      slv_lane_num;
} hw_hasha_mst_t;

typedef struct hw_hasha_slv_t {
	bool        req; 
	bool        ack;
	hw_block_t *mst_ptr;
	size_t      mst_lane_num;
} hw_hasha_slv_t;

struct hw_block_t {
	char             *name;
	size_t            id;
	size_t            numof_hasha_mst;
	size_t            numof_hasha_slv;
	size_t            hasha_mst_in_use;
	size_t            hasha_slv_in_use;
	hw_hasha_mst_t   *hasha_mst_ptr;
	hw_hasha_slv_t   *hasha_slv_ptr;
	
	pthread_cond_t    event;
	pthread_mutex_t   mutex;
};

void init_hw_block (hw_block_t *block_ptr, char *name, size_t id, size_t numof_hasha_mst, size_t numof_hasha_slv);

void connect_blocks (hw_block_t *mst_ptr, size_t mst_lane_num, hw_block_t *slv_ptr, size_t slv_lane_num);

void hasha_notify_slv   (hw_block_t *this_ptr, size_t mst_lane_num);
void hasha_wait_for_mst (hw_block_t *this_ptr, size_t slv_lane_num);
