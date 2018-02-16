#include <assert.h> 
#include "hasha.h"
#include "main.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>




static void *host_cpu_top (void *p) {

	hasha_block_t *this_ptr = (hasha_block_t *) p;
	
	printf ("host started, waiting...\n");
	
	this_ptr->port_arr[].handler = host_cpu_event_handler_fptr;
	
	hasha_run (this_ptr);
	
	size_t frame_idx = 0;
	size_t buf_idx = 0;
	size_t shader_idx = 0;
	const size_t frame_num = 5;
	const size_t buf_num = 1;
	const size_t shader_num = 2;
	
	//~ for (size_t i = 0; i < 5; i++) { // for each frame
		//~ printf ("FRAME %zu\n", i);
		//~ for (size_t j = 0; j < 1; j++) { // for each buffer (framebuf, shadowbuf, etc)
			//~ printf ("BUFFER %zu\n", j);	
			//~ for (size_t k = 0; k < 2; k++) { // for each shader (vshader, pshader, ..)
				//~ printf ("SHADER %zu\n", k);
				
				//~ hasha_handshake_req (this_ptr->port_arr[HASHA_HOST_TO_USHADER]);
				
				//~ // wait till ushaders tell me that they are done:
				//~ hasha_wait_for_req  (this_ptr->port_arr[HASHA_HOST_FROM_USHADER]);
				
				//~ // work done here
				//~ printf ("\tHost CPU is doing something meaningful...\n");
				//~ // end of work
			//~ }
			
			d.uint32 = 0xdeadbeef;
			hasha_send_req (this_ptr, HASHA_HOST_TO_VIDEOCTRL_MST, d, 0);
		//~ }
	//~ }
	
	
	while (1) {

		hasha_req_t r = hasha_wait_for_any_event (this_ptr);
	
		if (r.valid) {
						
			switch (p.port_idx) {
				
				case (HASHA_USHADER_POWERUPRESET):		
					hasha_handshake_req (this_ptr->port_arr[HASHA_HOST_TO_USHADER_MST]);
					shader_idx++;
					break;
					
				case (HASHA_HOST_TO_USHADER_MST):
					if (shader_idx < shader_num) {
						hasha_handshake_req (this_ptr->port_arr[HASHA_HOST_TO_USHADER_MST]);
						shader_idx++;
					}
					else if (buf_idx < buf_num) {
						shader_idx = 0;
						hasha_handshake_req (this_ptr->port_arr[HASHA_HOST_TO_USHADER_MST]);
						buf_idx++;
					}
					else if (frame_idx < frame_num) {
						shader_idx = 0;
						buf_idx = 0;
						hasha_handshake_req (this_ptr->port_arr[HASHA_HOST_TO_VIDEOCTRL_MST]);
						frame_idx++;
					}
					else {
						goto done;
					}					
					break;
					
				default:
					break;
			}
		}
	}
	
done:

	printf ("all done\n");
	
	return NULL;
}

static void *ushader_top (void *p) {

	hasha_block_t *this_ptr = (hasha_block_t *) p;
	
	printf ("ushader %zu started, waiting...\n", this_ptr->id);
	
	//uint32_t ctrl_regs[32];
	
	while (1) {

		hasha_req_t r = hasha_wait_for_any_event (this_ptr);
		
		if (r.valid) {
						
			switch (p.port_idx) {
				case (HASHA_USHADER_FROM_PREV):					
					hasha_handshake_ack (this_ptr->port_arr[p.port_idx]);
					hasha_handshake_req (this_ptr->port_arr[HASHA_USHADER_TO_NEXT]);
					//hasha_wait_for_ack  (this_ptr->port_arr[HASHA_USHADER_TO_NEXT]);

					break;
					
				case (HASHA_USHADER_FROM_NEXT):
					hasha_handshake_ack (this_ptr->port_arr[p.port_idx]);
					// If request came from downstream slave, forward the request
					//  to the upstream ushader
					hasha_handshake_req (this_ptr->port_arr[HASHA_USHADER_TO_PREV]);
					break;
					
				case (HASHA_USHADER_TO_NEXT):
					// Do the other work here
					printf ("\tushader %zu executing...\n", this_ptr->id);
					// end of work
					break;
					
				case (HASHA_USHADER_TO_PREV):
					break;
					
				default:
					printf ("\tERROR: Uknown request to Unified Shader %zu\n", this_ptr->id);
					return NULL;
			}
		}
	}
	
	printf ("\tushader %zu stopped\n", this_ptr->id);
	
	return NULL;
}

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


