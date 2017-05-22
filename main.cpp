#include <link.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "eh_frame_list.h"

#include "eh_frame.h"

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

	struct eh_frame_t *eh_frame = get_eh_frame_list();
	debug("eh_frame.addr: %p\n", eh_frame->addr);
	debug("eh_frame.size: 0x%lx\n", eh_frame->size);

	// ci_entry
	char *next_entry;
	bool fde = true;
	ci_entry *init_ci = new ci_entry(*eh_frame);
	next_entry = init_ci->entry_end();
	fde = (init_ci->is_cie() == false);
	init_ci->dump();
	delete(init_ci);

	while (true) {
		while (fde) {
			fd_entry *fd = new fd_entry(*eh_frame, next_entry);
			fde = (fd->is_cie() == false);
			next_entry = fd->entry_end();
			if (next_entry == NULL) break;
			fd->dump();
			delete(fd);
		}

		if (next_entry == NULL) break;
		ci_entry *ci = new ci_entry(*eh_frame, next_entry);
		next_entry = ci->entry_end();
		fde = (init_ci->is_cie() == false);
		ci->dump();
		delete(ci);
	}

	return 0;
}
