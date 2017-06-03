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
#include "eh_frame_hdr.h"

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

	struct eh_frame_hdr *eh_hdr = reinterpret_cast<eh_frame_hdr *>(eh_seg);

	uint64_t eh_frame_ptr_enc = 0x0 | eh_hdr->eh_frame_ptr_enc;
	char *enc = (char *)eh_hdr->enc;
	printf("eh_frame_ptr_enc: %d\n", eh_hdr->eh_frame_ptr_enc);
	printf("enc 0x%lx\n", *enc);

	uint64_t *result = (uint64_t *)decode_pointer(&enc, eh_frame_ptr_enc);
	printf("result: 0x%lx\n", result);
}

// ----------------------------------------------------------



uintptr_t g_offs = 0;
uintptr_t g_size = 0;

static int
callback(struct dl_phdr_info *info, size_t size, void *data)
{
    (void) size;
    (void) data;
    static auto once = false;

    if (once) return 0;
    once = true;

    // GNU_EH_FRAME eh_frame_hdr
    eh_hdr_parser(info->dlpi_addr, info->dlpi_phdr, info->dlpi_phnum, getauxval(AT_PHENT));

    for (int i = 0; i < info->dlpi_phnum; i++)
    {
        if (info->dlpi_phdr[i].p_type == PT_LOAD)
        {
            g_offs = info->dlpi_addr;
            break;
        }
    }

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

	std::stringstream eh_frame_offs_ss;
	std::stringstream eh_frame_size_ss;

	eh_frame_offs_ss << "readelf -SW /proc/" << getpid() << "/exe | grep \".eh_frame\" | grep -v \".eh_frame_hdr\" | awk '{print $4}' > offs.txt";
	eh_frame_size_ss << "readelf -SW /proc/" << getpid() << "/exe | grep \".eh_frame\" | grep -v \".eh_frame_hdr\" | awk '{print $6}' > size.txt";

	system(eh_frame_offs_ss.str().c_str());
	system(eh_frame_size_ss.str().c_str());

	auto &&offs_file = std::ifstream("offs.txt");
	auto &&size_file = std::ifstream("size.txt");

	auto &&offs_str = std::string((std::istreambuf_iterator<char>(offs_file)), std::istreambuf_iterator<char>());
	auto &&size_str = std::string((std::istreambuf_iterator<char>(size_file)), std::istreambuf_iterator<char>());

	/* file value */
//	std::cout << offs_str;
//	std::cout << size_str;

	std::remove("offs.txt");
	std::remove("size.txt");

	g_offs += std::stoull(offs_str, nullptr, 16);
	g_size += std::stoull(size_str, nullptr, 16);

//	std::cout << std::hex << g_offs << std::endl;
//	std::cout << std::hex << g_size << std::endl;

//	g_size = 0x10000000;

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

//	register_state *state = new register_state();
	struct registers_intel_x64_t registers = { };
	register_state_intel_x64 *state = new register_state_intel_x64(registers);

	for (auto itr = g_fde.begin(); itr != g_fde.end(); ++itr) {
		// objdump or dwarfdump in this function
		dwarf4::decode_cfi(*itr, state);
	}

	return 0;
}

