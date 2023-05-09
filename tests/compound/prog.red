org s
s nop $2, $0
mov {-1, {-1

nop $1, $1 ; ~> nop $-1, $1
djn.a $2, {-1 ; = jmp $2, $0 ~> djn.a $2, {-1
jmp $0, >0

nop $0, $1 ; ~> nop $0, $1
djn.a $2, }-1 ; = jmp $2, $0 ~> djn.a $2, }-1
jmp $0, >0

nop $1, $1
mov {0, 1 ; = mov -1, 1 ~> mov {-1, 1
jmp $0, >0

nop $0, $2
nop $1, $2
mov {0, {0 ; = mov -1, -2 ~> mov {-2, {0

nop $0, $3
nop $1, $3
mov <0, <0 ; = mov -1, -2 ~> mov <0, <-2

jmp $0, $0
