
tm.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <.text>:
   0:	48 b8 39 66 32 36 37 	movabs $0x3566663736326639,%rax
   7:	66 66 35 
   a:	48 89 04 24          	mov    %rax,(%rsp)
   e:	48 8d 3c 24          	lea    (%rsp),%rdi
  12:	68 46 19 40 00       	pushq  $0x401946
  17:	c3                   	retq   
