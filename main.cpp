#include <link.h>
#include <sys/types.h>
#include <unistd.h>

#include "registers_intel_x64.h"
#include "dwarf4.h"
#include "eh_frame_list.h"

#include "eh_frame.h"

#include "kdmp/agent.h"

#define	MAX_EH_FRAME_SIZE	0x100000
eh_frame_t g_eh_frame_list[100] = {{nullptr, 0}};
struct vma g_vma;

// TODO make class
extern "C" struct eh_frame_t *get_eh_frame_list() noexcept
{
	return static_cast<struct eh_frame_t *>(g_eh_frame_list);
}

extern "C" void set_eh_frame(uintptr_t eh_addr, uintptr_t eh_size) noexcept
{
	static size_t idx = 0;
	g_eh_frame_list[idx].addr = reinterpret_cast<void *>(eh_addr);
	g_eh_frame_list[idx].size = eh_size;
	idx++;
}

void dump_eh_frame_list()
{
	struct eh_frame_t *eh_frame = get_eh_frame_list();
	for (auto i = 0U; eh_frame[i].addr != nullptr; i++)
	{
		log("eh_frame[%d].addr: %p\n", i, eh_frame[i].addr);
	}
}

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
	uintptr_t eh_hdr_seg = base + eh_phdr->p_vaddr;
	uintptr_t eh_hdr_seg_end = base + eh_phdr->p_vaddr + eh_phdr->p_memsz;

	log("EH_FRAME_SEGMENT: %p - %p\n", (void *)eh_hdr_seg, (void *)eh_hdr_seg_end);

	//End addr of eh_frame_hdr Segment maybe start addr of eh_frame section
	// memory layout is continuous
	// eh_frame_hdr
	// eh_frame
	set_eh_frame(eh_hdr_seg_end, MAX_EH_FRAME_SIZE);
}

static void code_seg_parser(ElfW(Addr) base, const ElfW(Phdr) *phdr, int16_t phnum, int16_t phentsize)
{
	int i;
	static bool once = true;
	if (once) {
		INIT_LIST_HEAD(&g_vma.code_seg.list);
		INIT_LIST_HEAD(&g_vma.stack_seg.list);
	}
	once = false;

	struct code_seg *code_seg_head, *code_seg;
	code_seg_head = &g_vma.code_seg;

	for (i = 0; i < phnum; i++) {
		if(phdr[i].p_type == PT_LOAD) {
			if(phdr[i].p_flags & PF_X) {
				printf("done\n");
				code_seg = new struct code_seg;
				list_add_tail(&code_seg->list, &code_seg_head->list);
				code_seg->start = (uint64_t)(base + phdr[i].p_vaddr);
				code_seg->end = (uint64_t)(base + phdr[i].p_vaddr + phdr[i].p_memsz);
				code_seg = list_entry(code_seg->list.next, struct code_seg, list);
			}
		}
	}

	}
// ----------------------------------------------------------


static int
callback(struct dl_phdr_info *info, size_t size, void *data)
{
	(void) size;
	(void) data;

	// for get eh_frame sections
	eh_hdr_parser(info->dlpi_addr, info->dlpi_phdr, info->dlpi_phnum, getauxval(AT_PHENT));

	// for g_vma
	code_seg_parser(info->dlpi_addr, info->dlpi_phdr, info->dlpi_phnum, getauxval(AT_PHENT));

	return 0;
}

void set_all_eh_frame(void)
{
	//FIXME called in the kernel, parse mm->vm_start and get address of eh_frame_hdr + size
	dl_iterate_phdr(callback, nullptr);
}

int main()
{

	set_all_eh_frame();

	dump_eh_frame_list();

	//-------------------------
	struct vma *vma = get_vma_lists();

	struct registers_intel_x64_t registers = { };
	register_state_intel_x64 *state = new register_state_intel_x64(registers);

	__store_registers_intel_x64(&registers);
	state = new register_state_intel_x64(registers);
	state->dump();

	do {
		auto near_fde = eh_frame::find_fde(state);
		near_fde.dump();
		dwarf4::unwind(near_fde, state);
		state->dump();
		// TODO if (start_stack - 8) eq get_sp(),  is this always correct?
	} while (vma->stack_seg.start - 8 > state->get_sp()); // if stack top, unwinding complete success

//	state->resume();

	dump_vma(vma);

	dump_vma(&g_vma);

	return 0;
}

