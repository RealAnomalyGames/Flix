.start:
    outln "Starting Flix..."
    jmp .MainMenu

.MainMenu:
    outln "Main Menu"
    jmp .Gameplay

.Gameplay:
    outln "Gameplay Started"
    halt 0
