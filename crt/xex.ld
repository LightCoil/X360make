/********************************************************************
 * Оптимизированный linker script для сборки XEX‑образа
 * - Выравнивание всех сегментов по 0x1000 (странице).
 * - Разделение на сегменты с правильными правами (RX / RW).
 * - NOLOAD для .bss и .stack (линкер не заливает нули).
 * - Маркеры начала/конца для инициализации в рантайме.
 * - ENTRY объявляется через опцию -e или по умолчанию _start.
 ********************************************************************/

/* Если нужно переопределять точку входа из командной строки,
   можно при сборке передавать: -e MyEntryPoint */
ENTRY(_start)

/* Определяем Program Headers, чтобы управлять правами доступа */
PHDRS
{
    textseg  PT_LOAD FLAGS = P_FLAGS(R,X);
    datasets PT_LOAD FLAGS = P_FLAGS(R,W);
}

/* Разделяем все секции по выравниванию 0x1000 */
SECTIONS
{
    /* Текстовый сегмент (код + константы).
     * Если при сборке задан -Ttext=ADDR, .text начнётся с ADDR. */
    .text ALIGN(0x1000) : AT(ADDR(.text))
        :textseg
    {
        KEEP(*(.init))         /* Инициализационный код, если есть */
        *(.text .text.*)       /* Весь код */
        *(.rodata .rodata.*)   /* Все константы */
        _etext = .;            /* Маркер конца .text */
    }

    /* Данные с инициализированными переменными */
    .data ALIGN(0x1000) : AT(ADDR(.data))
        :datasets
    {
        __data_start__ = .;
        KEEP(*(.data .data.*))
        _edata = .;
    }

    /* Неинициализированные данные (.bss) → NOLOAD */
    .bss ALIGN(0x1000) (NOLOAD)
        :datasets
    {
        __bss_start__ = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end__ = .;
    }

    /* Секция стека (NOLOAD) */
    .stack ALIGN(0x1000) (NOLOAD)
        :datasets
    {
        __stack_start__ = .;
        . = ALIGN(8);
        . += 16K;           /* 16 КБ стек */
        __stack_end__ = .;
    }

    /* Отладочные метки */
    .comment : { *(.comment) }
    .note.gnu.build-id : { *(.note.gnu.build-id) }
}

/* Маркеры для рантайма */
PROVIDE(_end = __bss_end__);
PROVIDE(__stack_size = __stack_end__ - __stack_start__);

/* Для сборки:
 *
 * 1. Компиляция объектов (.o):
 *      gcc -c foo.c -fdata-sections -ffunction-sections
 *
 * 2. Линковка в ELF:
 *      ld -T xex.ld -Ttext=0x00001000 -o output.elf *.o -Map=output.map
 *
 * 3. Конвертация в бинарник (raw) без нулей для .bss и .stack:
 *      objcopy \
 *        --only-section=.text \
 *        --only-section=.rodata \
 *        --only-section=.data \
 *        --set-section-flags .bss=alloc,load,readonly,data \
 *        --set-section-flags .stack=alloc,load,readonly,data \
 *        -O binary output.elf output.raw
 *
 * После этого output.raw можно упаковать в XEX‑контейнер внешним инструментом.
 */
