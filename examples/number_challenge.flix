var number:int = 10
var target:int = 20
var bonus:int = 5
var lives:int = 3
var player:str = "Player"

.start:
    outln "========================"
    outln "     NUMBER CHALLENGE"
    outln "========================"

    outln player

    add number = bonus
    out "Current number: "
    outln number

    mul number = 2
    out "After multiplier: "
    outln number

    cmp number >= target
        outln "Challenge complete!"

        inc lives
        out "Bonus life awarded. Lives: "
        outln lives

    el
        outln "Challenge incomplete."

    cmp lives > 0 && number >= target
        outln "You win!"

    halt
