#include "kdmp/kdmp.h"

// COUNTERMEASURE
// return address points to executable vma?
// stack is pivoted?
// FCI
// FSI

// FLOW
// Kernelのtask_structのmmのmmapの内，X segmentsのものを取得するこれをVMAと定義する
// 現在のRIP(RIPとはリターンアドレスのこと？)を取得
// 対応するVMAを取得しチェックする
// VMAからEH_FRAMEを取得する
// Unwindingと，チェックをする



struct vma *get_vma(uint64_t rip)
{
	do {
		if (vma->code_seg.start < rip && rip < vma->code_seg.end)
			return vma;

		vma = vma->next;
	} while (vma != NULL);

	return NULL;
}

struct eh_frame get_eh_frame(struct *vma)
{
	int i;
	for (i = 0; i < MAX_NUM_MODULE; i++) {
		if (vma->code_seg.start < eh_frame_list[i] && eh_frame_list[i] < vma->code_seg.end) {
			return eh_frame_list[i];
		}
	}

	// BUG, not found eh_frame
	// この関数が呼ばれる前に，VMAの検証をしているのでここには到達しない
	return 0;
}

void do_check(void)
{
	while (true) {
		vma = get_vma(state->get_ip());

		if (vma != NULL) {
			//from kernel, not executable or new library is loaded
			vma = get_new_x_vma()
			if (vma == NULL) {
				//failed, maybe not executalbe
				//FIXME Flexible Insepect for start_stack
				return false;
			}
		}

		//TODO pointer?
		struct eh_frame eh_frame = get_eh_frame(vma);

		do {
			uint64_t rip = state->get_ip();
			uint64_t rsp = state->get_sp();

			if (rsp < STACK_START || rsp < STACK_END) { // stack pivot
				return false;
			}

			if (vma->code_seg.start < rsp && rsp < vma->code_seg.end) {
				unwind_depth += 1;
			}
			else {
				break; //go to next vma e.g.) binary eh_frame ==> library eh_frame
			}

			auto fde = eh_frame::find_fde(state);
			dwarf4::unwind(fde, state);
			fde.dump();
			state->dump();

			if (vma->stack_seg.start-8 =< state->get_sp()) {
				return true;
			}

		//FIXME
		} while (!fde);

	log("where\n");
	return true;
	}

}


