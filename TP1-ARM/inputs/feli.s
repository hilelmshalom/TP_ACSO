.text
movz X10, #15
movz X11, #10
cmp X10, X11
bgt salto
add X1, X1, #1
salto:
adds X2, X2, #10
HLT 0 