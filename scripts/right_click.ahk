Sleep, 1000
counter := 0
#SingleInstance force

Gui, Add, Edit, vLog w400 h400,
Gui, Show, w400 h400
stop := false

Loop
{
    Send {RButton}
    Sleep, 100
    Send {Esc}
    Sleep, 100
    counter := counter + 1
    GuiControl,, Log, %counter%

    if (stop)
    {
        break
    }
}

F10::
    stop := true
