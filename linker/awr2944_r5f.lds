/**
 * @file awr2944_r5f.lds
 * @brief GNU LD Linker Script for AWR2944 Cortex-R5F
 * @details Memory layout for bare-metal safety-critical firmware.
 *          Includes safety-specific sections for BIST data and
 *          safety-critical code placement in TCM for deterministic timing.
 *
 * Target: TI AWR2944 (Cortex-R5F, lockstep mode)
 * Toolchain: TI ARM CGT / GNU ARM
 */

ENTRY(_resetvectors)

MEMORY
{
    /* Tightly Coupled Memories -- deterministic, zero-wait-state */
    TCMA      (RWX) : ORIGIN = 0x00000000, LENGTH = 64K    /* R5F ATCM (code + vectors) */
    TCMB      (RW)  : ORIGIN = 0x00080000, LENGTH = 64K    /* R5F BTCM (data + stack)   */

    /* L2 SRAM -- main program and data memory */
    L2RAM     (RWX) : ORIGIN = 0x10200000, LENGTH = 384K   /* L2 SRAM                   */

    /* L3 SRAM -- shared memory for DSP/host communication */
    L3RAM     (RW)  : ORIGIN = 0x88000000, LENGTH = 2M     /* Shared L3 SRAM            */

    /* External Flash (QSPI) */
    FLASH     (RX)  : ORIGIN = 0x20000000, LENGTH = 4M     /* QSPI NOR Flash            */

    /* Mailbox memory for host interface */
    MBOX      (RW)  : ORIGIN = 0x50800000, LENGTH = 4K     /* Mailbox SRAM              */
}

SECTIONS
{
    /* -------------------------------------------------------------------- */
    /*  Vector Table -- placed at start of TCMA for fastest exception entry */
    /* -------------------------------------------------------------------- */
    .vectors : ALIGN(64)
    {
        _vectors_start = .;
        KEEP(*(.vectors))
        KEEP(*(.vectors.*))
        _vectors_end = .;
    } > TCMA

    /* -------------------------------------------------------------------- */
    /*  Safety-Critical Code -- placed in TCM for deterministic execution   */
    /*  per ISO 26262 ASIL-D timing requirements                            */
    /* -------------------------------------------------------------------- */
    .safety_critical : ALIGN(32)
    {
        _safety_text_start = .;
        *(.safety_critical)
        *(.safety_critical.*)
        *safety_monitor.o(.text .text.*)
        *fault_handler.o(.text .text.*)
        *watchdog.o(.text .text.*)
        *ecc_handler.o(.text .text.*)
        *lockstep_monitor.o(.text .text.*)
        _safety_text_end = .;
    } > TCMA

    /* -------------------------------------------------------------------- */
    /*  Main Program Text                                                   */
    /* -------------------------------------------------------------------- */
    .text : ALIGN(32)
    {
        _text_start = .;
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)
        _text_end = .;
    } > L2RAM

    /* -------------------------------------------------------------------- */
    /*  Read-Only Data                                                      */
    /* -------------------------------------------------------------------- */
    .rodata : ALIGN(8)
    {
        _rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)

        /* Calibration lookup tables */
        *(.cal_tables)

        /* CRC lookup tables */
        *(.crc_tables)
        _rodata_end = .;
    } > L2RAM

    /* ARM exception tables */
    .ARM.extab : ALIGN(4)
    {
        *(.ARM.extab*)
    } > L2RAM

    .ARM.exidx : ALIGN(4)
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > L2RAM

    /* -------------------------------------------------------------------- */
    /*  Initialized Data -- loaded from Flash/L2, copied to TCMB at startup */
    /* -------------------------------------------------------------------- */
    .data : ALIGN(8)
    {
        _data_start = .;
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        _data_end = .;
    } > TCMB AT > L2RAM

    _data_load_start = LOADADDR(.data);

    /* -------------------------------------------------------------------- */
    /*  BIST Data Section -- used for RAM march test patterns               */
    /* -------------------------------------------------------------------- */
    .bist_data (NOLOAD) : ALIGN(32)
    {
        _bist_data_start = .;
        *(.bist_data)
        *(.bist_data.*)
        _bist_data_end = .;
    } > TCMB

    /* -------------------------------------------------------------------- */
    /*  BSS (Zero-initialized Data)                                         */
    /* -------------------------------------------------------------------- */
    .bss (NOLOAD) : ALIGN(8)
    {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        _bss_end = .;
    } > TCMB

    /* -------------------------------------------------------------------- */
    /*  Heap                                                                */
    /* -------------------------------------------------------------------- */
    .heap (NOLOAD) : ALIGN(8)
    {
        _heap_start = .;
        . += 8K;
        _heap_end = .;
    } > TCMB

    /* -------------------------------------------------------------------- */
    /*  Stack -- placed at end of TCMB, grows downward                      */
    /*  Separate stacks for each ARM mode (ISO 26262 stack isolation)       */
    /* -------------------------------------------------------------------- */
    .stack (NOLOAD) : ALIGN(8)
    {
        _svc_stack_start = .;
        . += 4K;
        _svc_stack_end = .;

        _irq_stack_start = .;
        . += 2K;
        _irq_stack_end = .;

        _fiq_stack_start = .;
        . += 1K;
        _fiq_stack_end = .;

        _abort_stack_start = .;
        . += 1K;
        _abort_stack_end = .;

        _undef_stack_start = .;
        . += 512;
        _undef_stack_end = .;

        _sys_stack_start = .;
        . += 4K;
        _sys_stack_end = .;
    } > TCMB

    /* -------------------------------------------------------------------- */
    /*  DMA Ping-Pong Buffers -- placed in L3 for DSS/MSS shared access     */
    /* -------------------------------------------------------------------- */
    .dma_buffers (NOLOAD) : ALIGN(128)
    {
        _dma_buf_start = .;
        *(.dma_buffers)
        *(.dma_buffers.*)
        _dma_buf_end = .;
    } > L3RAM

    /* -------------------------------------------------------------------- */
    /*  Host Communication Mailbox                                          */
    /* -------------------------------------------------------------------- */
    .mailbox (NOLOAD) : ALIGN(32)
    {
        *(.mailbox)
    } > MBOX

    /* -------------------------------------------------------------------- */
    /*  Debug and ELF metadata (not loaded to target)                       */
    /* -------------------------------------------------------------------- */
    .comment        0 : { *(.comment) }
    .debug_info     0 : { *(.debug_info) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_ranges   0 : { *(.debug_ranges) }
    .debug_aranges  0 : { *(.debug_aranges) }

    /* Stack overflow sentinel */
    PROVIDE(_stack_guard_pattern = 0xDEADCAFE);
    PROVIDE(_main_stack_top = _sys_stack_end);
}
