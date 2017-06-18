#ifndef UNWIND_CHECKER_H
#define UNWIND_CHECKER_H

#include "kdmp/kdmp.h"
#include "eh_frame.h"
#include "registers_intel_x64.h"
#include "dwarf4.h"
#include "eh_frame_list.h"

// COUNTERMEASURE
// return address points to executable vma?
// stack is pivoted?
// FCI
// FSI

// FLOW
// pgdを共通するプロセスにおける，構造体をvmaと定義する
// vma構造体には，プログラムやライブラリの実行可能領域を表す構造体を持つ．これをcode_segと定義する．code_segは，双方向リンクリストとなっている．
// 共通のpgdを持ちながら，異なるスタックを利用する可能性がある(マルチスレッド).そのため，vma構造体はスタックを双方リンクリストとして持つ．
// clone(2)をフックし，vma構造体に追加する必要がある．

struct code_seg *get_code_seg_from_vma(struct vma *vma, uint64_t rip)
{
	struct code_seg *code_seg;
	struct list_head *pos;

	list_for_each(pos, &vma->code_seg.list) {
		code_seg = list_entry(pos, struct code_seg, list);
		if (code_seg->start < rip && rip < code_seg->end) {
			return code_seg;
		}
	}	

	return NULL;
}

struct eh_frame_t *get_eh_frame(struct code_seg *code_seg)
{
	auto eh_frame_list = get_eh_frame_list();

	for (int i = 0; i < MAX_NUM_MODULES; i++) {
		if (code_seg->start < (uint64_t)eh_frame_list[i].addr && 
		    (uint64_t)eh_frame_list[i].addr < code_seg->end) {
			return &eh_frame_list[i];
		}
	}

	// BUG, not found eh_frame
	// この関数が呼ばれる前に，VMAの検証をしているのでここには到達しない
	return nullptr;
}

// FIXME state is need? betther than getint into this function
bool do_check(struct vma *vma, register_state *state)
{
	struct code_seg *code_seg;
	uint64_t unwind_depth = 0;
	fd_entry fde;

	while (true) {
		code_seg = get_code_seg_from_vma(vma, state->get_ip());

		if (code_seg == NULL) {
			//from kernel, not executable or new library is loaded
			//vma = get_new_vma_from_kernel();
			code_seg = get_code_seg_from_vma(vma, state->get_ip());
			if (code_seg == NULL) {
				//failed, maybe not executalbe
				//FIXME Flexible Insepect for start_stack
				log("no executable area, stack corruption\n");
				return false;
			}
		}

		struct eh_frame_t *eh_frame = get_eh_frame(code_seg);
		uint64_t start_stack = vma->stack_seg.start;
		uint64_t end_stack   = vma->stack_seg.end;

		do {
			uint64_t rip = state->get_ip();
			uint64_t rsp = state->get_sp();

			if (rsp < end_stack || start_stack < rsp) { // stack pivot
				log("stack pivot detect ");
				log("rsp: 0x%lx start: 0x%lx end: 0x%lx\n", 
				     rsp, start_stack, end_stack);
				return false;
			}

			// NOTE check return address is a executable?
			if (code_seg->start < rip && rip < code_seg->end) {
				unwind_depth += 1;
			}
			else {
				log("go to next\n");
				break; //go to next code_seg e.g.) binary eh_frame ==> library eh_frame
			}

			fde = eh_frame::find_fde(state, *eh_frame);
			dwarf4::unwind(fde, state);
//			fde.dump();
			state->dump();

			if (start_stack-8 <= state->get_sp()) {
				printf("reach botton of the stack, success\n");
				return true;
			}

		} while (fde);
	}

	log("where\n");
	return true;
}

#endif
