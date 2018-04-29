.section .text
.global compileFunctionDynamicallyASM

@ On entry to this function the state pointer is in a0, the stack pointer in a1, the top of stack in a2
@ as per usual. We want to call the address associated with a2. 
compileFunctionDynamicallyASM:
    @ We only use r3 (TempRegister) and r4 (TempRegister2)
    @ As we are making non-tail function calls we have to push LR
    push {r3, r4, lr}
    mov r5, r0
    @ Allocate space on the stack for the DynamicFunctionResult, which is 3 words
    sub sp, #3 * 4
    @ When calling a function that returns a struct the first argument is the address to write the result at
    @ We therefore have to shift the original three arguments along one register
    mov r3, r2
    mov r2, r1
    mov r1, r0
    @ And then write the address of the stack pointer for the result
    mov r0, sp
    @ Load the address of the function to call
    @ 12 is the offset --- in words --- of m_compileOrInterpretFunction
    @ r1 contains the state pointer
    ldr r4, [r1, #12 * 4]
    @ Call m_compileOrInterpretFunction (JIT::compileFunctionDynamically)
    blx r4
    @ Restores the state pointer
    pop {r0}
    @ The second value in DynamicFunctionResult is the function that we need to jump to
    pop {r3}
    @ Verify that we don't have a nullptr for the function
    mov r4, #0
    cmp r3, r4
    beq lError
    @ We popped it into a temporary, so now move to ip
    mov ip, r3
    @ This is the offset that we need to add to all the return addresses
    pop {r3}
    @ We can skip the loop if the offset is zero
    cmp r3, r4
    beq lEnd
    @ MAIN LOOP
    @ Invariant: r1 contains the end index (m_escapeStackAddress)
    @            r4 contains the current index
    @            r2 contains the current value
    @            r3 contains the offset that we are adding to the return addresses
    ldr r1, [r0, #6 * 4]
    @ r1 w
    @ r4 is the stack index that we need to modify
    @ #2 because we ignore r3, r4 pushed onto the stack and we may need to update our own lr!
    add r4, sp, #2 * 4
lCheck:
    @ Terminate the loop once we've checked all the return addresses we need to
    cmp r1, r4
    beq lEnd
    @ Load the value
    ldr r2, [r4, #0 * 4]
    @ Adjust the return address by the new offset
    add r2, r2, r3
    @ Store the result back to the stack
    str r2, [r4, #0 * 4]
    @ Increment the return address that we're looking at
    add r4, r4, #4
    b lCheck
lEnd:
    @ Restore registers
    pop {r3, r4}
    @ Restore the lr
    pop {r2}
    mov lr, r2
    @ Restore the stack pointer
    ldr r1, [r0, #0 * 4]
    @ Restore the top of stack
    ldr r2, [r1, #0 * 4]
    @ Call the newly compiled function via tail call
    bx ip
lError:
    @ 5 == Environment::VMStatus::CompilerError
    mov r1, #5
    @ 13 == offsetof(Environment::VM, m_status) / sizeof(uint32_t)
    str r1, [r0, #13 * 4]
    @ Load the escape stack address
    ldr r1, [r0, #6 * 4]
    @ Regular halt code
    mov sp, r1
    pop {r4, r5, r6, r7, pc}