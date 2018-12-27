extern irq_handler

global isr
isr:
    pusha
    push    esp
    call    irq_handler
    add     esp, 4
    popa
    add     esp, 4
    iret

%macro declare_isr 1
global isr_%1
isr_%1:
    push    %1
    jmp     isr
%endmacro

%assign i 0
%rep 48
    declare_isr i
    %assign i i+1
%endrep
