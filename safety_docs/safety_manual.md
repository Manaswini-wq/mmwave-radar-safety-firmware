# Safety Manual

## AWR2944 mmWave Radar Safety Firmware

### Document Information
| Field | Value |
|---|---|
| Document ID | SM-AWR2944-001 |
| Version | 1.0 |
| Classification | ISO 26262 ASIL-D / IEC 61508 SIL-3 |
| Prepared for | TUV Certification Audit |

---

## 1. Introduction

This Safety Manual provides the information necessary for the safe integration and operation of the AWR2944 mmWave radar safety firmware. It is intended for system integrators, safety engineers, and certification auditors.

### 1.1 Assumed Safety Integrity Level

The firmware is designed, developed, and verified for:
- **ISO 26262 ASIL-D** (highest automotive safety integrity level)
- **IEC 61508 SIL-3** (safety integrity level 3)

### 1.2 Safety Goals

| SG-ID | Safety Goal | ASIL |
|---|---|---|
| SG-001 | Prevent unintended radar transmission | ASIL-D |
| SG-002 | Ensure safe state within FTTI on fault detection | ASIL-D |
| SG-003 | Maintain data integrity of radar measurements | ASIL-B |

## 2. Safety Architecture

### 2.1 Hardware Safety Mechanisms

| Mechanism | Coverage | Diagnostic Interval | Reference |
|---|---|---|---|
| Lockstep CPU (R5F) | CPU core faults | Continuous (cycle-by-cycle) | ISO 26262-5 Table D.3 |
| ECC (SEC/DED) | Memory faults (TCMA, TCMB, L2) | Continuous | ISO 26262-5 Table D.6 |
| Windowed Watchdog | Program flow timing | 10 ms | ISO 26262-5 Table D.5 |
| ESM Module | Fault escalation | Continuous | Device-specific |
| Voltage Monitor | Supply voltage | 20 ms | IEC 61508 Table A.14 |
| Temperature Monitor | Die temperature | 200 ms | IEC 61508 Table A.14 |

### 2.2 Software Safety Mechanisms

| Mechanism | Coverage | Diagnostic Interval |
|---|---|---|
| CPU Register Test | Stuck-at faults in registers | 10 ms |
| RAM March Test | Memory cell faults | 50 ms (background) |
| Stack Overflow Detection | Stack guard pattern check | 10 ms |
| Control Flow Monitoring | Illegal execution paths | 10 ms |
| E2E CRC-32 | Data corruption in transit | Every data transfer |
| ADC Loopback Test | ADC signal path faults | 100 ms |
| PLL Lock Monitor | Frequency synthesis faults | 10 ms |

## 3. Safety Metrics

| Metric | Target | Achieved | Requirement |
|---|---|---|---|
| SPFM | >= 99% | 99.2% | ISO 26262-5 Table 3 |
| LFM | >= 97% | 97.8% | ISO 26262-5 Table 3 |
| SFF | >= 99% | 99.1% | IEC 61508 Table 2 |

## 4. Safe State Definition

The safe state is defined as:
1. All TX channels disabled (no RF emission)
2. Chirp sequencer stopped
3. External safe-state pin asserted (active low)
4. Host processor notified via SPI
5. Watchdog continues to be serviced
6. System remains in safe state until power cycle

### 4.1 Safe State Transition Time

Maximum safe state transition time: **50 ms** (includes TX disable, chirp stop, and pin assertion).

### 4.2 Fault Tolerant Time Interval (FTTI)

The FTTI for all safety functions is **100 ms**, providing a 50 ms margin above the safe state transition time.

## 5. Integration Requirements

### 5.1 Hardware Requirements

1. Lockstep mode must be enabled via eFuse (cannot be enabled in software)
2. External watchdog IC recommended as backup to internal WDT
3. Safe-state GPIO pin must be connected to system-level safety logic
4. JTAG debug port must be disabled in production builds

### 5.2 Software Requirements

1. No modification of safety-critical code sections without re-verification
2. Host processor must monitor safe-state pin and SPI fault notifications
3. System must be power-cycled after safe-state entry (no automatic recovery)
4. Calibration must be run after every power-on

### 5.3 Environmental Requirements

| Parameter | Requirement |
|---|---|
| Operating Temperature | -40 to +125 degC |
| Supply Voltage (VDD) | 0.95V to 1.05V (1.0V nominal) |
| Supply Voltage (VDDA) | 1.71V to 1.89V (1.8V nominal) |

## 6. Verification Evidence

The following verification evidence is available for audit:

1. Unit test reports (tests/unit/) -- 100% requirement coverage
2. Integration test reports (tests/integration/) -- full data path and safety chain
3. Qualification test reports (tests/qualification/) -- ASIL-D requirement traceability
4. Static analysis report (MISRA-C:2012 compliance)
5. Code coverage report (MC/DC coverage >= 90%)
6. FMEA, FTA, DFA analysis documents

## 7. Known Limitations

1. ROM patches are loaded at startup; mid-operation patching is not supported
2. Calibration is performed once at startup; runtime re-calibration requires host command
3. Maximum continuous operation time before mandatory re-BIST: 24 hours
4. Flash memory wear-out: 100,000 erase cycles (fault log may fill up)

## 8. Revision History

| Version | Date | Author | Changes |
|---|---|---|---|
| 1.0 | 2024-01-15 | Safety Team | Initial release |
