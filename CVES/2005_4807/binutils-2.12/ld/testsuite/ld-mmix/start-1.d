#source: start2.s
#ld: -m elf64mmix
#objdump: -td

.*:     file format elf64-mmix

SYMBOL TABLE:
0+ l    d  \.text	0+ 
2000000000000000 l    d  \.data	0+ 
2000000000000000 l    d  \.sbss	0+ 
2000000000000000 l    d  \.bss	0+ 
0+ l    d  \*ABS\*	0+ 
0+ l    d  \*ABS\*	0+ 
0+ l    d  \*ABS\*	0+ 
0+4 g       \.text	0+ _start
2000000000000000 g     O \*ABS\*	0+ __bss_start
2000000000000000 g     O \*ABS\*	0+ _edata
2000000000000000 g     O \*ABS\*	0+ _end
0+4 g     O \.text	0+ _start\.

Disassembly of section \.text:

0+ <_start-0x4>:
   0:	fd000001 	swym 0,0,1

0+4 <_start>:
   4:	fd000002 	swym 0,0,2
