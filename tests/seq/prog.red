org s
s seq.a $ab1, $ba1
seq.a $ab1, $ac1
jmp $fail, $0

seq.b $ab1, $ac1
seq.b $cb1, $ab2
jmp $fail, $0

seq.ab $cb1, $ba1
seq.ab $ac1, $ba1
jmp $fail, $0

seq.ba $ab1, $ac1
seq.ba $cb1, $ba1
jmp $fail, $0

seq.f $ab1, $ba1
seq.f $ab1, $ab2
jmp $fail, $0

seq.x $ab1, $ab1
seq.x $ab2, $ba1
jmp $fail, $0

seq.i $ab1, $ab2
seq.i $ab1, $ab3
jmp $fail, $0

jmp $0, $0
fail mov $0, $1
ab1 dat $3, $-1
ab2 nop #3, >-1
ab3 dat $3, $-1
ba1 dat $-1, $3
ac1 dat $3, $2
cb1 dat $-2, $-1
