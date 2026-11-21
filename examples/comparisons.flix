var score:int = 85

.start:
    cmp score >= 90
        outln "Grade A"

    el cmp score >= 80
        outln "Grade B"

    el cmp score >= 70
        outln "Grade C"

    el
        outln "Grade F"

    halt
