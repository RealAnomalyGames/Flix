.start:
    var score:int = 0
    jmp .Gameplay

.Gameplay:
    mov score, 250
    jmp .GameOver

.GameOver:
    outln "Final Score:"
    outln score
    halt 0
