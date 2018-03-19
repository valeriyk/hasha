static void req_from_prev (void) {
	hasha_handshake_ack (this_ptr->port_arr[p.port_idx]);
	hasha_handshake_req (this_ptr->port_arr[HASHA_USHADER_TO_NEXT]);
	//hasha_wait_for_ack  (this_ptr->port_arr[HASHA_USHADER_TO_NEXT]);
	
	// Do the other work here
	printf ("\tushader %zu executing...\n", this_ptr->id);
	// end of work


	return;
}

static void req_from_next (void) {
	hasha_handshake_ack (this_ptr->port_arr[p.port_idx]);
	// If request came from downstream slave, forward the request
	//  to the upstream ushader
	hasha_handshake_req (this_ptr->port_arr[HASHA_USHADER_TO_PREV]);
	return;
}




void *ushader_top (void *p) {

	hasha_block_t *this_ptr = (hasha_block_t *) p;
	
	printf ("ushader %zu started, waiting...\n", this_ptr->id);
	
	this_ptr->port_arr[].proc = req_from_prev_proc;
	this_ptr->port_arr[].proc = req_from_next_proc;
	
	hasha_run (this_ptr);
	//uint32_t ctrl_regs[32];
	
	printf ("\tushader %zu stopped\n", this_ptr->id);
	
	return NULL;
}
