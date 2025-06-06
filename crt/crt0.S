    .text
    .align 2
    .globl _start

    # Внешние символы (линкер-скрипт должен их объявить)
    .extern _sdata         # начало small data в ROM
    .extern _sdata_end     # конец small data в RAM
    .extern _data_rom      # начало обычных данных в ROM
    .extern _data          # начало обычных данных в RAM
    .extern _data_end      # конец обычных данных в RAM
    .extern _sbss          # начало small BSS
    .extern _sbss_end      # конец small BSS
    .extern _bss           # начало обычного BSS
    .extern _bss_end       # конец обычного BSS
    .extern STACK_TOP      # вершина стека (скрипт линкера задаёт)

_start:
    # 1) Установка указателя стека (r1)
    lis   r1, STACK_TOP@ha
    addi  r1, r1, STACK_TOP@l

    # 2) Сохранение LR и резерв фрейма
    stwu  r1, -16(r1)
    mflr  r0
    stw   r0, 12(r1)

    # 3) Инициализация FPU/VMX
    mfmsr r0
    ori   r0, r0, 0x2000      # включаем бит FP (MSR[FP]=1)
    ori   r0, r0, 0x4000      # включаем бит SPE (MSR[SPE]=1)
    mtmsr r0

    # 4) Копируем small data (.sdata) с учётом overlap и выравнивания
    lis   r3, _sdata@ha
    addi  r3, r3, _sdata@l        # r3 = &_sdata (ROM)
    lis   r4, _sdata@ha
    addi  r4, r4, _sdata@l        # r4 = &_sdata (RAM)
    lis   r5, _sdata_end@ha
    addi  r5, r5, _sdata_end@l    # r5 = &_sdata_end (RAM)
    sub   r6, r5, r4              # r6 = size of .sdata
    cmpwi r6, 0
    ble   init_data               # если 0, пропускаем

    # Если ROM < RAM и участки пересекаются → backward copy
    cmpw  r3, r4
    bge   copy_sdata_forward
    add   r3, r3, r6
    add   r4, r4, r6

copy_sdata_backward_words:
    srwi  r7, r6, 2
    cmpwi r7, 0
    beq   copy_sdata_backward_tail
    subi  r3, r3, 4
    subi  r4, r4, 4
    lwz   r0, 0(r3)
    stw   r0, 0(r4)
    subi  r6, r6, 4
    b     copy_sdata_backward_words

copy_sdata_backward_tail:
    andi. r8, r6, 3
    beq   init_data
copy_sdata_backward_byte:
    subi  r3, r3, 1
    subi  r4, r4, 1
    lbz   r0, 0(r3)
    stb   r0, 0(r4)
    subi  r8, r8, 1
    cmpwi r8, 0
    bne   copy_sdata_backward_byte
    b     init_data

copy_sdata_forward:
    srwi  r7, r6, 2
copy_sdata_words:
    cmpwi r7, 0
    beq   copy_sdata_tail
    lwz   r0, 0(r3)
    stw   r0, 0(r4)
    addi  r3, r3, 4
    addi  r4, r4, 4
    addi  r7, r7, -1
    b     copy_sdata_words

copy_sdata_tail:
    andi. r8, r6, 3
    beq   init_data
copy_sdata_byte:
    lbz   r0, 0(r3)
    stb   r0, 0(r4)
    addi  r3, r3, 1
    addi  r4, r4, 1
    addi  r8, r8, -1
    cmpwi r8, 0
    bne   copy_sdata_byte

init_data:
    # 5) Копируем обычные данные (.data), overlap + выравнивание
    lis   r3, _data_rom@ha
    addi  r3, r3, _data_rom@l
    lis   r4, _data@ha
    addi  r4, r4, _data@l
    lis   r5, _data_end@ha
    addi  r5, r5, _data_end@l
    sub   r6, r5, r4
    cmpwi r6, 0
    ble   init_sbss

    cmpw  r3, r4
    bge   copy_data_forward
    add   r3, r3, r6
    add   r4, r4, r6

copy_data_backward_words:
    srwi  r7, r6, 2
    cmpwi r7, 0
    beq   copy_data_backward_tail
    subi  r3, r3, 4
    subi  r4, r4, 4
    lwz   r0, 0(r3)
    stw   r0, 0(r4)
    subi  r6, r6, 4
    b     copy_data_backward_words

copy_data_backward_tail:
    andi. r8, r6, 3
    beq   init_sbss
copy_data_backward_byte:
    subi  r3, r3, 1
    subi  r4, r4, 1
    lbz   r0, 0(r3)
    stb   r0, 0(r4)
    subi  r8, r8, 1
    cmpwi r8, 0
    bne   copy_data_backward_byte
    b     init_sbss

copy_data_forward:
    srwi  r7, r6, 2
copy_data_words:
    cmpwi r7, 0
    beq   copy_data_tail
    lwz   r0, 0(r3)
    stw   r0, 0(r4)
    addi  r3, r3, 4
    addi  r4, r4, 4
    addi  r7, r7, -1
    b     copy_data_words

copy_data_tail:
    andi. r8, r6, 3
    beq   init_sbss
copy_data_byte:
    lbz   r0, 0(r3)
    stb   r0, 0(r4)
    addi  r3, r3, 1
    addi  r4, r4, 1
    addi  r8, r8, -1
    cmpwi r8, 0
    bne   copy_data_byte

init_sbss:
    # 6) Обнуляем small BSS (.sbss) с выравниванием
    li    r0, 0
    lis   r4, _sbss@ha
    addi  r4, r4, _sbss@l
    lis   r5, _sbss_end@ha
    addi  r5, r5, _sbss_end@l

    andi. r6, r4, 3
    beq   sbss_zero_loop
    subi  r7, r4, r6
    addi  r4, r7, 4

sbss_zero_loop:
    cmpw  r4, r5
    beq   init_bss
    stw   r0, 0(r4)
    addi  r4, r4, 4
    b     sbss_zero_loop

init_bss:
    # 7) Обнуляем обычный BSS (.bss) с выравниванием
    li    r0, 0
    lis   r4, _bss@ha
    addi  r4, r4, _bss@l
    lis   r5, _bss_end@ha
    addi  r5, r5, _bss_end@l

    andi. r6, r4, 3
    beq   bss_zero_loop
    subi  r7, r4, r6
    addi  r4, r7, 4

bss_zero_loop:
    cmpw  r4, r5
    beq   done_init
    stw   r0, 0(r4)
    addi  r4, r4, 4
    b     bss_zero_loop

done_init:
    # 8) Восстанавливаем LR и выходим
    lwz   r0, 12(r1)
    mtlr  r0
    addi  r1, r1, 16
    blr

    .size _start, .-_start
