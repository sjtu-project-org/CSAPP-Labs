
tm.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <.text>:
   0:	48 b8 33 32 30 34 36 	movabs $0x3130333634303233,%rax
   7:	33 30 31 
   a:	48 89 04 24          	mov    %rax,(%rsp)
   e:	48 31 c0             	xor    %rax,%rax
  11:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
  16:	48 8d 3c 24          	lea    (%rsp),%rdi
  1a:	68 ba 18 40 00       	pushq  $0x4018ba
  1f:	c3                   	retq   
