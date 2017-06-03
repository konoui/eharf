#ifndef EH_FRAME_HDR_H
#define EH_FRAME_HDR_H

//detail is follow https://refspecs.linuxfoundation.org/LSB_1.3.0/gLSB/gLSB/ehframehdr.html

struct eh_frame_hdr {
	unsigned char version;
	unsigned char eh_frame_ptr_enc;
	unsigned char fde_count_enc;
	unsigned char table_enc;

	/*
	 * The rest of the header is variable-length and consists of the
	 * following members:
	 *
	 *	encoded_t eh_frame_ptr;
	 *	encoded_t fde_count;
	 */

	/* A single encoded pointer should not be more than 8 bytes. */
	uint64_t enc[2];

	/*
	 * struct {
	 *    encoded_t start_ip;
	 *    encoded_t fde_addr;
	 * } binary_search_table[fde_count];
	 */
	char data[0];
} __packed;

#endif
