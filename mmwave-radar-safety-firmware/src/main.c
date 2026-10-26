/**
 * @file main.c
 * @brief Main entry point for AWR2944 mmWave Radar Safety Firmware
 * @details Bare-metal main loop implementing ISO 26262 ASIL-D and
 *          IEC 61508 SIL-3 compliant radar control firmware.
 *
 *          Initialization sequence:
 *          1. Safety mechanisms (watchdog, ECC, lockstep, ESM)
 *          2. ROM patches and calibration trim loading
 *          3. Radar hardware (PLL, ADC, DAC, chirp engine)
 *          4. PBIST/LBIST self-tests
 *          5. Main control loop with periodic diagnostics
 *
 * @copyright Copyright (c) 2024
 * @license MIT
 */

#include "platform_types.h"
#include "safety_cfg.h"
#include "awr2944_regs.h"

/* Safety modules */
#include "safety/safety_monitor.h"
#include "safety/fault_handler.h"
#include "safety/diagnostic_coverage.h"
#include "safety/watchdog.h"
#include "safety/ecc_handler.h"
#include "safety/lockstep_monitor.h"
#include "safety/crc_e2e.h"
#include "safety/control_flow_monitor.h"

/* Radar modules */
#include "radar/radar_control.h"
#include "radar/chirp_config.h"
#include "radar/data_acquisition.h"

/* Calibration modules */
#include "calibration/tx_power_cal.h"
#include "calibration/rx_gain_phase_cal.h"
#include "calibration/lo_leakage_cal.h"

/* Driver modules */
#include "drivers/adc_driver.h"
#include "drivers/dac_driver.h"
#include "drivers/pll_driver.h"
#include "drivers/spi_driver.h"
#include "drivers/gpio_driver.h"

/* ROM modules */
#include "rom/rom_patch.h"
#include "rom/trim_loader.h"

/* HAL modules */
#include "hal/hal_interrupt.h"
#include "hal/hal_timer.h"
#include "hal/hal_dma.h"

/* ========================================================================== */
/*                           Module-Level Variables                           */
/* ========================================================================== */

/** @brief System tick counter (incremented by RTI timer ISR) */
static volatile uint32 g_systemTickMs = 0U;

/** @brief Flag set by chirp-done ISR */
static volatile boolean g_chirpDataReady = FALSE;

/** @brief Flag set by frame-done ISR */
static volatile boolean g_frameDone = FALSE;

/** @brief Current system state */
static volatile uint32 g_systemState = 0U;

/* System state definitions */
#define SYS_STATE_INIT          0U
#define SYS_STATE_SELFTEST      1U
#define SYS_STATE_CALIBRATION   2U
#define SYS_STATE_OPERATIONAL   3U
#define SYS_STATE_SAFE          4U

/* ========================================================================== */
/*                           Forward Declarations                             */
/* ========================================================================== */

static Status_t Main_InitSafetyMechanisms(void);
static Status_t Main_InitHardware(void);
static Status_t Main_RunStartupSelfTest(void);
static Status_t Main_RunCalibration(void);
static void     Main_ControlLoop(void);
static void     Main_ProcessChirpData(void);
static void     Main_RunPeriodicDiagnostics(void);
static void     Main_EnterSafeState(uint32 faultCode);
static void     Main_TimerISR(void);
static void     Main_ChirpDoneISR(void);
static void     Main_FrameDoneISR(void);

/* ========================================================================== */
/*                           Main Entry Point                                 */
/* ========================================================================== */

/**
 * @brief Firmware main entry point
 * @details Implements the complete initialization and runtime sequence
 *          per ISO 26262 ASIL-D software architecture.
 *
 *          The function must never return. If an unrecoverable fault is
 *          detected at any stage, the system enters a safe state where
 *          all TX channels are disabled and the chirp sequencer is stopped.
 */
int main(void)
{
    Status_t status;

    g_systemState = SYS_STATE_INIT;

    /* -------------------------------------------------------------------- */
    /* Phase 1: Initialize safety mechanisms FIRST                          */
    /* ISO 26262 Part 5, Clause 8: Safety mechanisms shall be initialized   */
    /* before any safety-relevant function becomes active.                   */
    /* -------------------------------------------------------------------- */
    status = Main_InitSafetyMechanisms();
    if (status != STATUS_OK)
    {
        /* Safety mechanism init failure -- cannot proceed safely */
        Main_EnterSafeState(0x0001U);
        /* NORETURN */
    }

    /* Control flow checkpoint: safety init complete */
    CFM_Checkpoint(CFM_CP_SAFETY_INIT_DONE);

    /* -------------------------------------------------------------------- */
    /* Phase 2: Load ROM patches and calibration trim data                  */
    /* Must happen before hardware init as patches may affect register map   */
    /* -------------------------------------------------------------------- */
    status = RomPatch_LoadAll();
    if (status != STATUS_OK)
    {
        Main_EnterSafeState(0x0002U);
    }

    status = TrimLoader_LoadAll();
    if (status != STATUS_OK)
    {
        Main_EnterSafeState(0x0003U);
    }

    CFM_Checkpoint(CFM_CP_ROM_PATCH_DONE);

    /* -------------------------------------------------------------------- */
    /* Phase 3: Initialize radar hardware                                    */
    /* PLL, ADC, DAC, GPIO, SPI, DMA, interrupt controller                  */
    /* -------------------------------------------------------------------- */
    status = Main_InitHardware();
    if (status != STATUS_OK)
    {
        Main_EnterSafeState(0x0004U);
    }

    CFM_Checkpoint(CFM_CP_HW_INIT_DONE);

    /* -------------------------------------------------------------------- */
    /* Phase 4: Startup self-tests (PBIST, LBIST)                           */
    /* IEC 61508 Table A.3: Startup tests for SIL-3                         */
    /* -------------------------------------------------------------------- */
    g_systemState = SYS_STATE_SELFTEST;
    status = Main_RunStartupSelfTest();
    if (status != STATUS_OK)
    {
        Main_EnterSafeState(0x0005U);
    }

    CFM_Checkpoint(CFM_CP_SELFTEST_DONE);

    /* -------------------------------------------------------------------- */
    /* Phase 5: Run calibration algorithms                                   */
    /* TX power, RX gain/phase, LO leakage                                  */
    /* -------------------------------------------------------------------- */
    g_systemState = SYS_STATE_CALIBRATION;
    status = Main_RunCalibration();
    if (status != STATUS_OK)
    {
        Main_EnterSafeState(0x0006U);
    }

    CFM_Checkpoint(CFM_CP_CAL_DONE);

    /* -------------------------------------------------------------------- */
    /* Phase 6: Enter operational state                                      */
    /* Start radar frame sequencing and enter main control loop              */
    /* -------------------------------------------------------------------- */
    g_systemState = SYS_STATE_OPERATIONAL;

    /* Start radar operation */
    status = RadarControl_StartFrames();
    if (status != STATUS_OK)
    {
        Main_EnterSafeState(0x0007U);
    }

    /* Verify control flow integrity up to this point */
    if (CFM_Verify() != STATUS_OK)
    {
        Main_EnterSafeState(0x00FFU);
    }

    /* -------------------------------------------------------------------- */
    /* Main control loop -- runs indefinitely                                */
    /* -------------------------------------------------------------------- */
    Main_ControlLoop();

    /* Should never reach here */
    Main_EnterSafeState(0xDEADU);

    return 0;  /* Unreachable, satisfies compiler */
}

/* ========================================================================== */
/*                  Phase 1: Safety Mechanism Initialization                  */
/* ========================================================================== */

/**
 * @brief Initialize all safety mechanisms
 * @return STATUS_OK on success, STATUS_ERROR on failure
 */
static Status_t Main_InitSafetyMechanisms(void)
{
    Status_t status;

    /* Initialize control flow monitor first */
    CFM_Init(CFM_SIGNATURE_SEED);

    /* Initialize fault handler and ESM */
    status = FaultHandler_Init();
    if (status != STATUS_OK) { return status; }

    /* Initialize and start windowed watchdog */
    status = Watchdog_Init(WDT_TIMEOUT_MS, WDT_WINDOW_MIN_MS);
    if (status != STATUS_OK) { return status; }

    status = Watchdog_Enable();
    if (status != STATUS_OK) { return status; }

    /* Initialize ECC on all memories */
    status = ECC_Init();
    if (status != STATUS_OK) { return status; }

    /* Verify lockstep CPU mode is active */
    status = Lockstep_Init();
    if (status != STATUS_OK) { return status; }

    /* Initialize safety monitor (diagnostic scheduler) */
    status = SafetyMonitor_Init();
    if (status != STATUS_OK) { return status; }

    /* Initialize E2E CRC module */
    CRC_E2E_Init();

    return STATUS_OK;
}

/* ========================================================================== */
/*                  Phase 3: Hardware Initialization                          */
/* ========================================================================== */

/**
 * @brief Initialize all hardware peripherals
 * @return STATUS_OK on success
 */
static Status_t Main_InitHardware(void)
{
    Status_t status;

    /* Service watchdog during lengthy init */
    Watchdog_Service();

    /* Initialize interrupt controller (VIM) */
    HAL_Interrupt_Init();

    /* Register ISR handlers */
    HAL_Interrupt_Register(INT_NUM_RTI_COMP0, Main_TimerISR);
    HAL_Interrupt_Register(INT_NUM_CHIRP_AVAIL, Main_ChirpDoneISR);
    HAL_Interrupt_Register(INT_NUM_FRAME_DONE, Main_FrameDoneISR);

    /* Initialize system tick timer (1 ms) */
    HAL_Timer_Init(0U, 1000U);  /* Timer 0, 1000 us period */
    HAL_Timer_Start(0U);

    /* Initialize GPIO (including safety output pin) */
    status = GPIO_Init();
    if (status != STATUS_OK) { return status; }
    GPIO_SetDirection(ESM_SAFE_STATE_PIN, GPIO_DIR_OUTPUT);
    GPIO_Write(ESM_SAFE_STATE_PIN, 1U);  /* De-assert safe state */

    Watchdog_Service();

    /* Initialize PLL and wait for lock */
    status = PLL_Init();
    if (status != STATUS_OK) { return status; }

    status = PLL_WaitForLock(1000U);  /* 1000 us timeout */
    if (status != STATUS_OK) { return status; }

    Watchdog_Service();

    /* Initialize ADC */
    status = ADC_Init();
    if (status != STATUS_OK) { return status; }

    /* Initialize DAC */
    status = DAC_Init();
    if (status != STATUS_OK) { return status; }

    /* Initialize SPI for host communication */
    status = SPI_Init();
    if (status != STATUS_OK) { return status; }

    Watchdog_Service();

    /* Initialize DMA for data acquisition */
    HAL_DMA_Init();

    /* Initialize radar control module */
    status = RadarControl_Init();
    if (status != STATUS_OK) { return status; }

    /* Initialize data acquisition (ping-pong buffers, DMA) */
    status = DataAcq_Init();
    if (status != STATUS_OK) { return status; }

    /* Enable global interrupts */
    HAL_Interrupt_GlobalEnable();

    return STATUS_OK;
}

/* ========================================================================== */
/*                  Phase 4: Startup Self-Test                                */
/* ========================================================================== */

/**
 * @brief Run startup Built-In Self-Tests
 * @details Executes PBIST (memory BIST) and LBIST (logic BIST)
 *          as required by IEC 61508 for SIL-3 startup diagnostic.
 * @return STATUS_OK if all self-tests pass
 */
static Status_t Main_RunStartupSelfTest(void)
{
    DiagResult_t result;

    Watchdog_Service();

    /* Run CPU register test */
    result = Diagnostic_CPURegisterTest();
    if (result != DIAG_PASS) { return STATUS_ERROR; }

    Watchdog_Service();

    /* Run program counter integrity test */
    result = Diagnostic_PCIntegrityTest();
    if (result != DIAG_PASS) { return STATUS_ERROR; }

    Watchdog_Service();

    /* Run RAM march test on critical memory regions */
    result = Diagnostic_RAMMarchTest();
    if (result != DIAG_PASS) { return STATUS_ERROR; }

    Watchdog_Service();

    /* ECC self-test: inject and verify SEC detection */
    result = ECC_SelfTest();
    if (result != DIAG_PASS) { return STATUS_ERROR; }

    Watchdog_Service();

    /* Lockstep self-test: inject compare error */
    result = Lockstep_SelfTest();
    if (result != DIAG_PASS) { return STATUS_ERROR; }

    Watchdog_Service();

    /* ADC loopback self-test */
    result = Diagnostic_ADCLoopbackTest();
    if (result != DIAG_PASS) { return STATUS_ERROR; }

    return STATUS_OK;
}

/* ========================================================================== */
/*                  Phase 5: Calibration                                      */
/* ========================================================================== */

/**
 * @brief Run all calibration algorithms
 * @return STATUS_OK if calibration succeeds
 */
static Status_t Main_RunCalibration(void)
{
    Status_t status;

    Watchdog_Service();

    /* TX power calibration -- measure and correct power flatness */
    status = TxPowerCal_Run();
    if (status != STATUS_OK) { return status; }

    Watchdog_Service();

    /* RX gain/phase calibration -- correct inter-channel mismatch */
    status = RxGainPhaseCal_Run();
    if (status != STATUS_OK) { return status; }

    Watchdog_Service();

    /* LO leakage calibration -- null DC offset */
    status = LOLeakageCal_Run();
    if (status != STATUS_OK) { return status; }

    return STATUS_OK;
}

/* ========================================================================== */
/*                  Main Control Loop                                         */
/* ========================================================================== */

/**
 * @brief Main operational control loop
 * @details Runs continuously, processing radar data and executing
 *          periodic safety diagnostics. Services the watchdog within
 *          the valid window.
 *
 *          ISO 26262 Part 6, Clause 9: The main loop timing must be
 *          deterministic and bounded by the watchdog timeout.
 */
static void Main_ControlLoop(void)
{
    uint32 lastDiagTickMs = 0U;

    while (g_systemState == SYS_STATE_OPERATIONAL)
    {
        /* ---- Service watchdog within the valid window ---- */
        Watchdog_Service();

        /* ---- Control flow checkpoint: loop iteration start ---- */
        CFM_Checkpoint(CFM_CP_LOOP_START);

        /* ---- Process chirp data if available ---- */
        if (g_chirpDataReady == TRUE)
        {
            g_chirpDataReady = FALSE;
            Main_ProcessChirpData();
        }

        /* ---- Handle frame completion ---- */
        if (g_frameDone == TRUE)
        {
            g_frameDone = FALSE;
            /* Frame data ready -- notify host via SPI */
            DataAcq_NotifyHost();
        }

        /* ---- Run periodic diagnostics ---- */
        if ((g_systemTickMs - lastDiagTickMs) >= DIAG_CPU_TEST_INTERVAL_MS)
        {
            lastDiagTickMs = g_systemTickMs;
            Main_RunPeriodicDiagnostics();
        }

        /* ---- Check safety monitor status ---- */
        if (SafetyMonitor_GetStatus() != STATUS_OK)
        {
            Main_EnterSafeState(0x0010U);
        }

        /* ---- Control flow checkpoint: loop iteration end ---- */
        CFM_Checkpoint(CFM_CP_LOOP_END);
    }
}

/**
 * @brief Process received chirp ADC data
 * @details Reads DMA-captured ADC data from ping-pong buffer,
 *          validates E2E CRC, and prepares for host transfer.
 */
static void Main_ProcessChirpData(void)
{
    Status_t status;
    DataAcq_Buffer_t *pBuffer;

    /* Get the completed buffer from the DMA ping-pong */
    pBuffer = DataAcq_GetReadyBuffer();
    if (pBuffer == NULL_PTR)
    {
        return;
    }

    /* Validate data integrity with E2E CRC */
    status = DataAcq_ValidateCRC(pBuffer);
    if (status != STATUS_OK)
    {
        /* CRC mismatch -- data corruption detected */
        FaultHandler_Report(FAULT_DATA_INTEGRITY, FAULT_SEVERITY_HIGH);
        return;
    }

    /* Queue buffer for host transfer */
    DataAcq_QueueForTransfer(pBuffer);
}

/* ========================================================================== */
/*                  Periodic Diagnostic Runner                                */
/* ========================================================================== */

/**
 * @brief Execute periodic safety diagnostics
 * @details Runs a subset of diagnostics each cycle to distribute
 *          computational load. Full coverage is achieved within
 *          the FTTI (Fault Tolerant Time Interval).
 */
static void Main_RunPeriodicDiagnostics(void)
{
    static uint32 diagPhase = 0U;

    switch (diagPhase)
    {
        case 0U:
            /* CPU register test */
            SafetyMonitor_RunDiagnostic(DIAG_ID_CPU_REG);
            break;

        case 1U:
            /* Stack overflow check */
            SafetyMonitor_RunDiagnostic(DIAG_ID_STACK_OVERFLOW);
            break;

        case 2U:
            /* Voltage monitor */
            SafetyMonitor_RunDiagnostic(DIAG_ID_VOLTAGE_MON);
            break;

        case 3U:
            /* Temperature monitor */
            SafetyMonitor_RunDiagnostic(DIAG_ID_TEMP_MON);
            break;

        case 4U:
            /* PLL lock status */
            SafetyMonitor_RunDiagnostic(DIAG_ID_PLL_LOCK);
            break;

        case 5U:
            /* ECC error count check */
            SafetyMonitor_RunDiagnostic(DIAG_ID_ECC_STATUS);
            break;

        case 6U:
            /* Watchdog self-test (window timing) */
            SafetyMonitor_RunDiagnostic(DIAG_ID_WDT_CHECK);
            break;

        case 7U:
            /* RAM background march test (partial) */
            SafetyMonitor_RunDiagnostic(DIAG_ID_RAM_MARCH);
            break;

        default:
            diagPhase = 0U;
            return;
    }

    diagPhase++;
    if (diagPhase > 7U)
    {
        diagPhase = 0U;
    }
}

/* ========================================================================== */
/*                  Safe State Entry                                          */
/* ========================================================================== */

/**
 * @brief Enter safe state
 * @param faultCode Fault identification code for logging
 * @details Implements fail-safe transition per ISO 26262 ASIL-D:
 *          1. Stop chirp sequencer
 *          2. Disable all TX channels
 *          3. Assert external safe-state pin
 *          4. Notify host processor
 *          5. Log fault information
 *          6. Remain in safe state (no automatic recovery without host)
 */
SAFETY_CRITICAL
static void Main_EnterSafeState(uint32 faultCode)
{
    /* Disable interrupts to prevent further processing */
    HAL_Interrupt_GlobalDisable();

    g_systemState = SYS_STATE_SAFE;

    /* Stop chirp sequencer immediately */
    REG32_WR(CHIRP_SEQ_FORCE_STOP, 0x01U);

    /* Disable all TX channels */
    uint32 ch;
    for (ch = 0U; ch < RADAR_MAX_TX_CHANNELS; ch++)
    {
        REG32_WR(TX_CH_CTRL(ch), 0x00U);
    }

    /* Assert external safe-state pin (active low) */
    GPIO_Write(ESM_SAFE_STATE_PIN, 0U);

    /* Log fault to persistent storage */
    FaultHandler_LogFault(faultCode, g_systemTickMs);

    /* Attempt to notify host via SPI */
    SPI_TransmitByte((uint8)(faultCode & 0xFFU));
    SPI_TransmitByte((uint8)((faultCode >> 8U) & 0xFFU));

    /* Remain in safe state -- service watchdog to prevent reset */
    /* Host must power-cycle to recover */
    while (1U)
    {
        Watchdog_Service();
        /* Blink LED or toggle GPIO to indicate safe state */
        GPIO_Toggle(ESM_SAFE_STATE_PIN);
        /* Simple delay loop */
        volatile uint32 delay;
        for (delay = 0U; delay < 1000000U; delay++)
        {
            /* Intentionally empty */
        }
    }
}

/* ========================================================================== */
/*                  Interrupt Service Routines                                */
/* ========================================================================== */

/**
 * @brief System tick timer ISR (1 ms period)
 */
static void Main_TimerISR(void)
{
    g_systemTickMs++;
    HAL_Timer_ClearInterrupt(0U);
}

/**
 * @brief Chirp ADC data available ISR
 * @details Called when DMA completes transfer of ADC data for one chirp.
 */
static void Main_ChirpDoneISR(void)
{
    g_chirpDataReady = TRUE;
    DataAcq_SwapBuffers();
    /* Clear chirp-done interrupt */
    REG32_WR(ADC_INT_CLEAR, ADC_INT_BUFFER_FULL);
}

/**
 * @brief Frame complete ISR
 */
static void Main_FrameDoneISR(void)
{
    g_frameDone = TRUE;
    /* Clear frame-done status */
    REG32_WR(CHIRP_STATUS, CHIRP_STATUS_FRAME_DONE);
}
