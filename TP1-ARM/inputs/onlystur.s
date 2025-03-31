.text
mov X1, 0x10000000
mov X10, 0x1234
stur X10, [X1, 0x0]
sturb W10, [X1, 0x6]
HLT 0 