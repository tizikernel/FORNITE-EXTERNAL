.CODE

VMProtectBegin_ASM64 PROC
    push rax
    push rbx
    push rcx
    push rdx
    
    ; Magic VMP marker signature, safe for CPU to execute past if unpacked
    mov eax, 'VMPr'
    mov ebx, 'otec'
    mov ecx, 'tBeg'
    mov edx, 'in'
    
    pop rdx
    pop rcx
    pop rbx
    pop rax
    ret
VMProtectBegin_ASM64 ENDP

VMProtectEnd_ASM64 PROC
    push rax
    push rbx
    push rcx
    push rdx
    
    ; Magic VMP marker signature, safe for CPU to execute past if unpacked
    mov eax, 'VMPr'
    mov ebx, 'otec'
    mov ecx, 'tEnd'
    mov edx, ' '
    
    pop rdx
    pop rcx
    pop rbx
    pop rax
    ret
VMProtectEnd_ASM64 ENDP

END
