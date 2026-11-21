var passcode:int = 123
var lives:int = 3
var name:str = "John Doe"

.start:
    cmp passcode == 123
    && name == "John Doe"
        outln "Welcome, John Doe!"

    cmp lives > 0 || passcode == 0
        outln "Player can continue."

    cmp !lives == 0
        outln "Player still has lives."

    cmp lives > 0 ^^ passcode == 0
        outln "XOR condition passed."

    halt
