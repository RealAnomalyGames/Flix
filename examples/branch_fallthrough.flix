var number:int = 2
var result:str = ""

.start:
    cmp number == 1
        mov result, "One"
    el cmp number == 2
        mov result, "Two"
    el cmp number == 3
        mov result, "Three"
    el
        mov result, "Other"

    outln result
    halt
