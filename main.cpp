#include <link.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include "registers_intel_x64.h"
#include "dwarf4.h"
#include "eh_frame_list.h"

#include "eh_frame.h"

#define	MAX_EH_FRAME_SIZE	0x100000
uintptr_t g_offs = 0;
uintptr_t g_size = MAX_EH_FRAME_SIZE;

// ----------------------------------------------------------
#include <sys/auxv.h>
static const ElfW(Phdr) *get_phdr(const ElfW(Phdr) *phdr, uint16_t phnum, uint16_t phentsize) {

	for (int i = 0; i < phnum; i++) {
		if (phdr->p_type == PT_GNU_EH_FRAME) {
			return phdr;
		}

		phdr = (ElfW(Phdr) *)((char *)phdr + phentsize);
	}

	return NULL;
}

static void eh_hdr_parser(ElfW(Addr) base, const ElfW(Phdr) *phdr, int16_t phnum, int16_t phentsize)
{
	const ElfW(Phdr) *eh_phdr = get_phdr(phdr, phnum, phentsize);
	if (!eh_phdr) return;
	uintptr_t eh_seg = base + eh_phdr->p_vaddr;
	uintptr_t eh_seg_end = base + eh_phdr->p_vaddr + eh_phdr->p_memsz;

	printf("EH_FRAME_SEGMENT: %p - %p\n", (void *)eh_seg, (void *)eh_seg_end);

	//End addr of eh_frame_hdr Segment maybe start addr of eh_frame section
	// memory layout is continuous
	// eh_frame_hdr
	// eh_frame
	g_offs = eh_seg_end;
}
// ----------------------------------------------------------


static int
callback(struct dl_phdr_info *info, size_t size, void *data)
{
	(void) size;
	(void) data;
	static auto once = false;

	if (once) return 0;
	once = true;

	// for get eh_frame sections
	eh_hdr_parser(info->dlpi_addr, info->dlpi_phdr, info->dlpi_phnum, getauxval(AT_PHENT));

	return 0;
}

eh_frame_t g_eh_frame_list[100] = {{nullptr, 0}};

extern "C" struct eh_frame_t *get_eh_frame_list() noexcept
{
	g_eh_frame_list[0].addr = reinterpret_cast<void *>(g_offs);
	g_eh_frame_list[0].size = g_size;

	return static_cast<struct eh_frame_t *>(g_eh_frame_list);
}

int main()
{
	dl_iterate_phdr(callback, nullptr);

	struct eh_frame_t *eh_frame = get_eh_frame_list();
	debug("eh_frame.addr: %p\n", eh_frame->addr);
	debug("eh_frame.size: 0x%lx\n", eh_frame->size);


	std::vector<fd_entry> g_fde;
	for (auto fde = fd_entry(*eh_frame); fde; ++fde) {
		if(fde.is_cie()) {
			auto cie = ci_entry(*eh_frame, fde.entry_start());
//			cie.dump();
			continue;
		}

		g_fde.push_back(fde);
//		fde.dump();
	}

	struct registers_intel_x64_t registers = { };
	register_state_intel_x64 *state = new register_state_intel_x64(registers);

	for (auto itr = g_fde.begin(); itr != g_fde.end(); ++itr) {
		// objdump or dwarfdump in this function
		dwarf4::decode_cfi(*itr, state);
	}

	return 0;
}

