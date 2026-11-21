var score:int = 85
var bonus:int = 15
var passed:bool = true
var result:str = ""

.start:
    cmp score >= 80
        && bonus >= 10
        && passed == true
        mov result, "All conditions passed"
    el
        mov result, "Conditions failed"

    outln result
    halt
