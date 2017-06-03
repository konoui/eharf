//	FIXME does not work

.text

.global __store_registers_intel_x64
__store_registers_intel_x64:
    movq %rax, 0x00(%rdi)
    movq %rbx, 0x08(%rdi)
    movq %rcx, 0x10(%rdi)
    movq %rdx, 0x18(%rdi)
    movq %rdi, 0x20(%rdi)
    movq %rsi, 0x28(%rdi)
    movq %rbp, 0x30(%rdi)
    add $0x08, %rsp
    movq %rsp, 0x38(%rdi)
    sub $0x08, %rsp
    movq %r8, 0x40(%rdi)
    movq %r9, 0x48(%rdi)
    movq %r10, 0x50(%rdi)
    movq %r11, 0x58(%rdi)
    movq %r12, 0x60(%rdi)
    movq %r13, 0x68(%rdi)
    movq %r14, 0x70(%rdi)
    movq %r15, 0x78(%rdi)

    movq (%rsp), %rax
    movq %rax, 0x80(%rdi)

    ret

.global __load_registers_intel_x64
__load_registers_intel_x64:
    movq 0x08(%rdi), %rdx
    movq 0x10(%rdi), %rcx
    movq 0x18(%rdi), %rbx
    movq 0x28(%rdi), %rsi
    movq 0x30(%rdi), %rbp
    movq 0x40(%rdi), %r8
    movq 0x48(%rdi), %r9
    movq 0x50(%rdi), %r10
    movq 0x58(%rdi), %r11
    movq 0x60(%rdi), %r12
    movq 0x68(%rdi), %r13
    movq 0x70(%rdi), %r14
    movq 0x78(%rdi), %r15

    movq 0x38(%rdi), %rsp

    movq 0x80(%rdi), %rax
    pushq %rax

    movq 0x00(%rdi), %rax
    movq 0x20(%rdi), %rdi

    ret

