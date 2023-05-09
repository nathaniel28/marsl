org s
s djn.a $2, $a
jmp $fail, >0
djn.a $fail, $a
djn.b $2, $a
jmp $fail, >0
djn.b $fail, $a
djn.f $2, $b
jmp $fail, >0
djn.f $2, $c
jmp $fail, >0
djn.f $2, $d
jmp $fail, >0
djn.f $fail, $d
djn.f $2, $e
jmp $fail, >0
jmp $0, $0
fail mov $0, $1
a dat $2, $2
b dat $2, $1
c dat $1, $2
d dat $2, $2
e dat $0, $0
