.start:
    outln "Start"
    jmp .Second

.First:
    outln "First"
    halt 0

.Second:
    outln "Second"
    jmp .First
