#source: greg-1.s
#source: gregbza1.s
#source: start.s
#source: a.s
#as: -x
#ld: -m elf64mmix
#objdump: -dt

# Like greg-3, but a different expanding insn.

.*:     file format elf64-mmix

SYMBOL TABLE:
0+ l    d  .text	0+ 
2000000000000000 l    d  .data	0+ 
2000000000000000 l    d  .sbss	0+ 
2000000000000000 l    d  .bss	0+ 
0+7f0 l    d  \.MMIX\.reg_contents	0+ 
0+ l    d  \*ABS\*	0+ 
0+ l    d  \*ABS\*	0+ 
0+ l    d  \*ABS\*	0+ 
0+18 g       \.text	0+ _start
0+fe g       \*REG\*	0+ areg
2000000000000000 g     O \*ABS\*	0+ __bss_start
2000000000000000 g     O \*ABS\*	0+ _edata
2000000000000000 g     O \*ABS\*	0+ _end
0+18 g     O \.text	0+ _start\.
0+1c g       \.text	0+ a

Disassembly of section \.text:

0+ <_start-0x18>:
   0:	5afe0006 	pbnz \$254,18 <_start>
   4:	e3ff001c 	setl \$255,0x1c
   8:	e6ff0000 	incml \$255,0x0
   c:	e5ff0000 	incmh \$255,0x0
  10:	e4ff0000 	inch \$255,0x0
  14:	9fffff00 	go \$255,\$255,0

0+18 <_start>:
  18:	e3fd0001 	setl \$253,0x1

0+1c <a>:
  1c:	e3fd0004 	setl \$253,0x4
