# Dependent Failure Analysis (DFA)

## Document Information
| Field | Value |
|---|---|
| Document ID | DFA-AWR2944-001 |
| Version | 1.0 |
| Classification | ISO 26262 ASIL-D |

## 1. Purpose

This document analyzes dependent failures (common cause, cascading, and common mode failures) that could defeat the safety mechanisms in the AWR2944 radar firmware per ISO 26262-9, Clause 7.

## 2. Initiating Causes

| ID | Initiating Cause | Affected Components | Analysis |
|---|---|---|---|
| IC-01 | Power supply voltage drop | CPU, memories, analog, PLL | Voltage monitor triggers safe state before components malfunction |
| IC-02 | Clock source failure | CPU, peripherals, timers | PLL lock detect + independent WDT (RC oscillator) |
| IC-03 | Extreme temperature | All components | Temperature monitor + thermal margins + derating |
| IC-04 | ESD/EMI event | All digital/analog | ESD protection, PCB shielding, software CRC |
| IC-05 | Software systematic fault | All software modules | Lockstep (detects data-dependent faults), diverse algorithms |
| IC-06 | Radiation-induced upset | Memories, CPU registers | ECC + lockstep + periodic BIST |

## 3. Cascading Failure Analysis

| Cascade Path | Likelihood | Mitigation |
|---|---|---|
| ECC DED -> Data abort -> ISR corruption | Low | Data abort enters safe state directly |
| PLL unlock -> Wrong ADC clock -> Data corruption | Medium | PLL monitor disables ADC before data used |
| WDT timeout -> Reset -> Corrupt flash | Very Low | Flash write requires explicit sequence |
| DMA error -> Buffer overflow -> Code corruption | Low | DMA MPU prevents writes to code region |

## 4. Common Mode Failure Analysis

| Safety Function Pair | Independence Mechanism | CMF Resistance |
|---|---|---|
| Lockstep CPU vs. ECC | Different hardware blocks, different fault models | High |
| Watchdog vs. ESM | Different clock sources (RC vs. crystal) | High |
| Software CRC vs. Hardware ECC | Diverse algorithms (CRC polynomial vs. Hamming) | High |
| Temperature monitor vs. Voltage monitor | Different analog sensors | High |

## 5. Conclusion

The safety architecture demonstrates sufficient independence between safety mechanisms. No single initiating cause can defeat multiple independent safety mechanisms simultaneously, meeting the DFA requirements of ISO 26262 ASIL-D.
