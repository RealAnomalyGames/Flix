var score:int = 85
var result:str = ""

.start:
    cmp score >= 90
        mov result, "Excellent"
    el cmp score >= 80
        mov result, "Great"
    el cmp score >= 70
        mov result, "Good"
    el
        mov result, "Needs improvement"

    outln result
    halt
