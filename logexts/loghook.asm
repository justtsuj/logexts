.model flat, STDCALL
.code

LogProcessHook PROTO r_ebp:DWORD, ret_addr:DWORD, index:DWORD
; void __stdcall LogHook()
_LogHook      proc 
                push    esp
                push    eax
                push    edx
                push    ecx
                sub     esp, 0Ch
                push    ebp
                mov     ebp, esp
                mov     eax, [ebp+24h]
                push    dword ptr [ebp+20h]
                mov     [ebp+4], eax
                push    eax
                push    ebp
                call    LogProcessHook
                mov     ecx, [ebp+24h]
                mov     edx, [ebp+1Ch]
                mov     [edx], ecx
                pop     ebp
                add     esp, 0Ch
                pop     ecx
                pop     edx
                add     esp, 4
                pop     esp
                retn
_LogHook      endp

; __int64 __stdcall LogHookCallFunction(_DWORD *r_ebp, __int64 (__stdcall *real_func)(), int *num_of_param)
_LogHookCallFunction PROC r_ebp:DWORD, real_func:DWORD, p_num_of_param:DWORD


                ;push    ebp
                ;mov     ebp, esp
                push    ebx
                push    esi
                push    edi
                mov     ebx, esp
                mov     ecx, p_num_of_param
                mov     ecx, [ecx]
                add     ecx, 10h
                mov     esi, r_ebp
                and     esp, 0FFFFFF00h
                mov     edi, esp
                add     esi, 28h
                sub     edi, 100h
                push    ebx
                sub     esp, 0FCh
                cld
                rep movsd
                mov     eax, r_ebp
                mov     edx, [eax+14h]
                mov     ecx, [eax+10h]
                mov     eax, [eax+18h]
                call    real_func
                push    eax
                mov     eax, r_ebp
                mov     [eax+14h], edx
                mov     [eax+10h], ecx
                pop     eax
                mov     ecx, esp
                and     ecx, 0FFh
                and     esp, 0FFFFFF00h
                add     esp, 0FCh
                pop     esp
                mov     edi, esp
                add     edi, 0Ch
                cmp     ebp, edi
                jnz     short loc_5002FA94
loc_5002FA6E:
                mov     edi, r_ebp
                add     [edi+1Ch], ecx
                shr     ecx, 1
                jb      short loc_5002FA8C
                shr     ecx, 1
                jb      short loc_5002FA91
loc_5002FA7C:
                mov     esi, p_num_of_param
                mov     [esi], ecx
                add     dword ptr [edi+1Ch], 4
                pop     edi
                pop     esi
                pop     ebx
                pop     ebp
                retn    0Ch
loc_5002FA8C:
                int     3
                shr     ecx, 1
                jmp     short loc_5002FA7C
loc_5002FA91:
                int     3
                jmp     short loc_5002FA7C
loc_5002FA94:
                int     3
                mov     ebp, edi
                jmp     short loc_5002FA6E
_LogHookCallFunction endp
end