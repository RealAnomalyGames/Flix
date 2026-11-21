var score:int = 85
var bonus:int = 15
var result:str = ""

.start:
    cmp score >= 80
        cmp bonus >= 20
            mov result, "Excellent"
        el cmp bonus >= 10
            mov result, "Great"
        el
            mov result, "Passed"
    el
        mov result, "Failed"

    outln result
    halt
