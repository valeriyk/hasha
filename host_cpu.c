
static void host_cpu_mst_por_proc (void) {
	
	size_t frame_idx = 0;
	size_t buf_idx = 0;
	size_t shader_idx = 0;
	const size_t frame_num = 5;
	const size_t buf_num = 1;
	const size_t shader_num = 2;
	
	for (size_t i = 0; i < 5; i++) { // for each frame
	
		printf ("FRAME %zu\n", i);
		for (size_t j = 0; j < 1; j++) { // for each buffer (framebuf, shadowbuf, etc)
	
			printf ("BUFFER %zu\n", j);	
			for (size_t k = 0; k < 2; k++) { // for each shader (vshader, pshader, ..)
	
				printf ("SHADER %zu\n", k);
				
				hasha_handshake_req (this_ptr->port_arr[HASHA_HOST_TO_USHADER]);
				
				// wait till ushaders tell me that they are done:
				hasha_wait_for_req  (this_ptr->port_arr[HASHA_HOST_FROM_USHADER]);
				
				// work done here
				printf ("\tHost CPU is doing something meaningful...\n");
				// end of work
			}
			
			d.uint32 = 0xdeadbeef;
			hasha_send_req (this_ptr, HASHA_HOST_TO_VIDEOCTRL_MST, d, 0);
		}	
	}
}

void *host_cpu_top (void *p) {

	hasha_block_t *this_ptr = (hasha_block_t *) p;
	printf ("host mst started, waiting...\n");
	this_ptr->port_arr[].handler = host_cpu_event_handler_fptr;
	hasha_run (this_ptr);
	printf ("all done\n");
	
	return NULL;
}
