# Failure Mode and Effects Analysis (FMEA)

## Document Information
| Field | Value |
|---|---|
| Document ID | FMEA-AWR2944-001 |
| Version | 1.0 |
| Date | 2024-01-15 |
| Classification | ISO 26262 ASIL-D |
| Status | Released for Review |

## 1. Scope

This FMEA covers the AWR2944 mmWave radar safety firmware, analyzing potential failure modes in the software components and their effects on system safety. The analysis follows ISO 26262-5, Annex E methodology.

## 2. System Description

The firmware controls the AWR2944 radar sensor used in automotive ADAS applications. Safety-critical functions include:
- Radar TX power control (prevents excessive radiation)
- Chirp sequencer control (ensures correct radar operation)
- Safe-state management (disables TX on fault detection)
- Data integrity (CRC protection on radar data path)

## 3. FMEA Table

| ID | Component | Failure Mode | Effect | Severity | Occurrence | Detection | RPN | Safety Mechanism | Residual Risk |
|---|---|---|---|---|---|---|---|---|---|
| FM-001 | CPU Core | Stuck-at fault in registers | Incorrect computation, wrong TX power | 10 | 2 | 1 | 20 | Lockstep CPU comparison, CPU register test | Acceptable |
| FM-002 | CPU Core | Program counter corruption | Execution of unintended code | 10 | 2 | 1 | 20 | Lockstep, control flow monitoring | Acceptable |
| FM-003 | TCMA Memory | Single-bit error (SEC) | Data corruption, correctable | 5 | 3 | 1 | 15 | ECC (Hamming), SEC interrupt | Acceptable |
| FM-004 | TCMA Memory | Double-bit error (DED) | Data corruption, uncorrectable | 10 | 1 | 1 | 10 | ECC detection, safe-state entry | Acceptable |
| FM-005 | Watchdog Timer | Timer stuck / not running | Loss of timing supervision | 9 | 1 | 2 | 18 | Windowed WDT self-test, ESM monitoring | Acceptable |
| FM-006 | PLL | Loss of lock | Incorrect chirp frequency | 8 | 3 | 1 | 24 | PLL lock monitor, ESM error | Acceptable |
| FM-007 | ADC | Stuck-at output | Incorrect radar data | 7 | 2 | 2 | 28 | ADC loopback self-test, data range check | Acceptable |
| FM-008 | TX Power Amp | Excessive output power | Regulatory violation, safety hazard | 10 | 1 | 1 | 10 | TX power monitor, safe-state TX disable | Acceptable |
| FM-009 | DMA Engine | Address corruption | Data written to wrong memory | 9 | 1 | 2 | 18 | DMA MPU protection, E2E CRC | Acceptable |
| FM-010 | Stack | Stack overflow | Memory corruption, undefined behavior | 9 | 2 | 1 | 18 | Stack guard patterns, periodic check | Acceptable |
| FM-011 | Voltage Supply | Under/over voltage | Component malfunction | 8 | 2 | 1 | 16 | On-die voltage monitor, ESM alert | Acceptable |
| FM-012 | Temperature | Over-temperature | Device damage, parameter drift | 7 | 2 | 1 | 14 | Temperature monitor, throttle/safe-state | Acceptable |
| FM-013 | CRC Module | CRC miscalculation | Undetected data corruption | 9 | 1 | 2 | 18 | Dual CRC computation, known-answer test | Acceptable |
| FM-014 | Chirp Engine | Chirp parameter corruption | Wrong radar waveform | 8 | 1 | 2 | 16 | Parameter readback verification | Acceptable |
| FM-015 | ESM Module | ESM not responding | Faults not escalated | 10 | 1 | 2 | 20 | ESM self-test, external watchdog | Acceptable |

| FM-016 | Flash Memory | Bit flip in stored constants | Calibration or config data corrupted | 8 | 2 | 1 | 16 | Flash ECC, CRC over config blocks, compare with golden copy | Acceptable |
| FM-017 | SPI Interface | Data frame corruption | Host receives wrong radar data or commands | 7 | 3 | 1 | 21 | E2E CRC-32 on every SPI frame, ACK/NAK protocol | Acceptable |
| FM-018 | SPI Interface | Clock line stuck | Communication loss with host | 6 | 2 | 2 | 24 | SPI timeout monitor, safe-state on prolonged silence | Acceptable |
| FM-019 | GPIO Safe-State Pin | Pin stuck high (inactive) | Safe state not signalled to host | 10 | 1 | 2 | 20 | Pin readback after assertion, redundant SPI fault notification | Acceptable |
| FM-020 | Interrupt Controller | Spurious interrupt | Unintended ISR execution, timing jitter | 7 | 2 | 2 | 28 | ISR entry validation, control flow monitor, stack guard | Acceptable |
| FM-021 | Interrupt Controller | Missed interrupt | Delayed or missed fault handling | 9 | 1 | 2 | 18 | Polling backup for critical faults, WDT safety net | Acceptable |
| FM-022 | Lockstep Comparator | Comparator failure (stuck match) | CPU faults not detected | 10 | 1 | 1 | 10 | Periodic lockstep self-test with forced mismatch, ESM check | Acceptable |
| FM-023 | Control Flow Monitor | Checkpoint counter corruption | Illegal path not detected | 9 | 1 | 2 | 18 | Dual counter with diverse encoding, WDT as backup | Acceptable |
| FM-024 | DAC (TX Power) | Output drift | Gradual TX power deviation | 7 | 2 | 2 | 28 | Periodic TX power calibration check, ADC power readback | Acceptable |
| FM-025 | DAC (TX Power) | Output rail-to-rail | Maximum TX power emitted | 10 | 1 | 1 | 10 | Hardware TX power limiter, safe-state on out-of-range | Acceptable |
| FM-026 | RX Front-End | Gain stuck at max | Signal saturation, wrong measurements | 6 | 2 | 2 | 24 | RX gain calibration readback, signal quality check | Acceptable |
| FM-027 | LO Leakage Cal | Calibration value corruption | Phase error in radar measurements | 5 | 2 | 2 | 20 | CRC on calibration table, reasonableness check | Acceptable |
| FM-028 | ROM Patch Module | Patch not applied | Known silicon errata active | 8 | 1 | 1 | 8 | Patch verification (readback), version register check | Acceptable |
| FM-029 | ROM Patch Module | Wrong patch applied | Unpredictable device behaviour | 9 | 1 | 1 | 9 | Patch CRC verification, patch ID whitelist | Acceptable |
| FM-030 | DMA MPU | MPU misconfigured | DMA can overwrite protected memory | 9 | 1 | 2 | 18 | MPU config readback at startup and periodic, E2E CRC on buffers | Acceptable |

## 4. Severity / Occurrence / Detection Scales

### 4.1 Severity (S) Scale
| Rating | Description | Criteria |
|---|---|---|
| 10 | Hazardous | Failure can lead to safety hazard without warning (uncontrolled TX emission) |
| 9 | Serious | Failure can lead to safety hazard with warning (fault escalation may be delayed) |
| 8 | High | Failure causes loss of primary function (radar stops operating) |
| 7 | Moderate | Failure causes degraded function (radar accuracy reduced) |
| 6 | Low | Failure causes minor functional impact (data quality slightly degraded) |
| 5 | Minor | Failure causes inconvenience (correctable error logged) |
| 1-4 | Negligible | No functional impact or cosmetic only |

### 4.2 Occurrence (O) Scale
| Rating | Description | Estimated Rate |
|---|---|---|
| 1 | Extremely unlikely | < 1 per 10^8 hours |
| 2 | Remote | 1 per 10^7 hours |
| 3 | Low | 1 per 10^6 hours |
| 4 | Moderate | 1 per 10^5 hours |
| 5+ | High | > 1 per 10^4 hours |

### 4.3 Detection (D) Scale
| Rating | Description | Detection Mechanism |
|---|---|---|
| 1 | Almost certain | Hardware automatic detection (lockstep, ECC, ESM) |
| 2 | High | Software periodic diagnostic within 10 ms |
| 3 | Moderate | Software periodic diagnostic within 100 ms |
| 4+ | Low | Detection depends on external monitoring or manual review |

## 5. RPN Thresholds

| RPN Range | Action Required |
|---|---|
| 1-50 | Acceptable -- monitor during operation; no design change required |
| 51-100 | Risk reduction recommended -- add diagnostic or design improvement |
| 101-200 | Risk reduction required -- mandatory design change before release |
| 201-1000 | Unacceptable -- redesign required; production release blocked |

## 6. Summary Statistics

| Metric | Value |
|---|---|
| Total failure modes analysed | 30 |
| Maximum RPN | 28 (FM-007 ADC stuck, FM-020 spurious interrupt, FM-024 DAC drift) |
| Minimum RPN | 8 (FM-028 ROM patch not applied) |
| Average RPN | 17.9 |
| Failure modes with S >= 9 | 12 (all have D <= 2) |
| Failure modes with RPN > 50 | 0 |

## 7. Recommended Monitoring Actions

Even though all RPNs are below the action threshold, the following monitoring actions are recommended for production:

1. **Lockstep self-test** -- Run forced mismatch test at every power-on (FM-022)
2. **ECC error rate logging** -- Track SEC correction frequency; increasing rate may indicate degradation (FM-003)
3. **TX power readback** -- Log TX power ADC readings over product lifetime for trend analysis (FM-008, FM-024, FM-025)
4. **PLL lock statistics** -- Track PLL re-lock events; root-cause any increase (FM-006)
5. **SPI error rate** -- Monitor CRC failure rate on SPI bus as environmental indicator (FM-017, FM-018)

## 8. Conclusions

All 30 identified failure modes have RPNs below 50 due to the comprehensive defence-in-depth safety architecture:

- **Hardware mechanisms** (lockstep CPU, ECC, ESM, voltage/temperature monitors) provide continuous, automatic detection with detection rating D=1.
- **Software mechanisms** (control flow monitor, CRC-32 E2E, watchdog, periodic BIST) provide periodic detection within the 100 ms FTTI budget with detection rating D=2.
- **Redundancy** ensures no single safety mechanism failure leaves a hazardous fault undetected (see DFA_analysis.md).

The highest severity failure modes (S=10: FM-001, FM-002, FM-004, FM-008, FM-015, FM-019, FM-022, FM-025) all have at least two independent detection mechanisms, ensuring robust fault coverage consistent with ASIL-D / SIL-3 requirements.

No residual risks are classified as unacceptable. This FMEA will be reviewed and updated whenever the safety architecture or firmware design changes.

## 9. Revision History

| Version | Date | Author | Changes |
|---|---|---|---|
| 1.0 | 2024-01-15 | Safety Team | Initial release (15 failure modes) |
| 1.1 | 2024-02-01 | Safety Team | Extended to 30 failure modes; added scales, statistics, monitoring actions |
