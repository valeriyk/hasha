#include <assert.h> 
#include "hasha.h"
#include "main.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>





/*
static void *video_ctrl_top (void *p) {

	enum {UPSTREAM_LINK = 0};
	
	printf ("videoctrl started, waiting...\n");
	
	hasha_block_t *this_ptr = (hasha_block_t *) p;
	
	while (1) {
	
		//hasha_wait_for_mst (this_ptr, HASHA_VIDEOCTRL_SLV);
		hasha_payload_t p = hasha_wait_for_req (this_ptr);

		if (p.valid && (p.port_idx == HASHA_VIDEOCTRL_SLV)) {
			// work done here
			printf ("Video Controller is drawing a frame from %x...\n", p.data.uint32);
			// end of work
		}
		else {
			printf ("ERROR: Video Controller received unknown request!\n");
		}
	}
	
	printf ("videoctrl stopped\n");
	
	return NULL;
}
*/

	
int main (int argc, char** argv) {
      
    printf ("Constructing the system...\n");
    
	hasha_block_t host_cpu   = hasha_new_block ("Host CPU",         DUMMY_ID, HOST_MAX_HASHA_PORTS, host_cpu_top);
	//hasha_block_t video_ctrl = hasha_new_block ("Video Controller", DUMMY_ID, VIDEOCTRL_MAX_HASHA_PORTS, video_ctrl_top);
	hasha_block_t bus_fabric = hasha_new_block ("Bus Fabric",       DUMMY_ID, GPU_MAX_USHADERS + 1, bus_fabric_top); // 1 slave + N masters
	hasha_block_t reset_ctrl = hasha_new_block ("Reset Controller", DUMMY_ID, 1, NULL); // connected to the host cpu only
	hasha_block_t ushader [GPU_MAX_USHADERS];
	for (size_t i = 0; i < GPU_MAX_USHADERS; i++) {
		ushader[i] = hasha_new_block ("Unified Shader", i, USHADER_MAX_HASHA_PORTS, ushader_top);
	}
		
	//~ hasha_link_blocks (&host_cpu,   HASHA_HOST_TO_USHADER_MST,  &ushader[0], HASHA_USHADER_UPSTREAM_SLV, ACK);
	//~ hasha_link_blocks (&ushader[0], HASHA_USHADER_UPSTREAM_MST, &host_cpu,   HASHA_HOST_TO_USHADER_SLV, ACK);
	//~ for (size_t i = 1; i < GPU_MAX_USHADERS; i++) {
		//~ hasha_link_blocks (&ushader[i-1], HASHA_USHADER_DOWNSTREAM_MST, &ushader[i]ll,   HASHA_USHADER_UPSTREAM_SLV, ACK);
		//~ hasha_link_blocks (&ushader[i],   HASHA_USHADER_UPSTREAM_MST,   &ushader[i-1], HASHA_USHADER_DOWNSTREAM_SLV, ACK);
	//~ }
	//~ hasha_link_blocks (&ushader[GPU_MAX_USHADERS-1], HASHA_USHADER_DOWNSTREAM_MST, &ushader[GPU_MAX_USHADERS-1], HASHA_USHADER_DOWNSTREAM_SLV, NOACK); // loopback, must not have ACK
	//~ hasha_link_blocks (&host_cpu, HASHA_HOST_TO_VIDEOCTRL_MST, &videoctrl, HASHA_VIDEOCTRL_SLV, ACK);
	
	hasha_link_ports (&host_cpu.port_arr[HASHA_HOST_TO_USHADER_MST], &ushader[0].port_arr[HASHA_USHADER_FROM_PREV], ACK);
	hasha_link_ports (&ushader[0].port_arr[HASHA_USHADER_TO_PREV], &host_cpu.port_arr[HASHA_HOST_TO_USHADER_SLV], ACK);
	for (size_t i = 1; i < GPU_MAX_USHADERS; i++) {
		hasha_link_ports (&ushader[i-1].port_arr[HASHA_USHADER_TO_NEXT], &ushader[i].port_arr[HASHA_USHADER_FROM_PREV], ACK);
		hasha_link_ports (&ushader[i].port_arr[HASHA_USHADER_TO_PREV],   &ushader[i-1].port_arr[HASHA_USHADER_FROM_NEXT], ACK);
	}
	hasha_link_ports (&ushader[GPU_MAX_USHADERS-1].port_arr[HASHA_USHADER_DOWNSTREAM_MST], &ushader[GPU_MAX_USHADERS-1].port_arr[HASHA_USHADER_DOWNSTREAM_SLV], NOACK); // loopback, must not have ACK
	/*hasha_link_ports (&host_cpu.port_arr[HASHA_HOST_TO_VIDEOCTRL_MST], &video_ctrl.port_arr[HASHA_VIDEOCTRL_SLV], ACK);
	
	hasha_link_ports (&host_cpu.port_arr[HASHA_HOST_TO_BUSMUX], &bus_fabric.port_arr[HASHA_BUSMUX_SLV], ACK);
	for (size_t i = 0; i < GPU_MAX_USHADERS; i++) {
		hasha_link_ports (&bus_fabric.port_arr[HASHA_BUSMUX_MST + i], &ushader[i].port_arr[HASHA_USHADER_UPSTREAM_BUS_SLV], ACK);
	}
	*/
	hasha_link_ports (&host_cpu.port_arr[HASHA_HOST_TO_RESET_CTRL], &reset_ctrl.port_arr[HASHA_RESET_CTRL_FROM_HOST], NOACK);
	
	printf ("System constructed!\n");
	
	hasha_run (&reset_ctrl);
	
	// DONE
    return 0;
}


