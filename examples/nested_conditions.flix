var score:int = 95
var lives:int = 3

.start:
    cmp score >= 50
        outln "You passed."

        cmp score >= 90
            outln "Excellent score!"

        el
            outln "Keep practicing."

    el
        outln "You failed."

    cmp lives > 0
        outln "You are still playing."

    halt
