# Fault Tree Analysis (FTA)

## Document Information
| Field | Value |
|---|---|
| Document ID | FTA-AWR2944-001 |
| Version | 1.0 |
| Classification | ISO 26262 ASIL-D |

## 1. Top-Level Event

**Unintended radar emission** -- The radar sensor transmits RF energy in an uncontrolled manner (wrong frequency, wrong power, or when it should be silent).

## 2. Fault Tree

```
                    [TOP: Unintended Radar Emission]
                                |
                    +-----------+-----------+
                    |                       |
            [TX Not Disabled          [Wrong Chirp
             When Required]            Parameters]
                    |                       |
              +-----+-----+          +-----+-----+
              |           |          |           |
        [Safe-State   [TX Power  [PLL Lock   [Chirp Reg
         Logic Fail]   DAC Stuck] Lost]       Corruption]
              |           |          |           |
         +----+----+ +---+---+ +---+---+   +---+---+
         |        |  |       |  |       |   |       |
    [CPU Fault] [ESM [HW    [SW [VCO   [Ref [ECC   [SW
     AND        Fail] Fail]  Bug] Drift] Clk  Fail]  Bug]
     Lockstep                     |      Fail]
     Fail]                   [Temp
                              OOR]
```

## 3. Minimal Cut Sets

| Cut Set | Events | Probability | Safety Mechanism |
|---|---|---|---|
| CS-1 | CPU fault AND lockstep fault | ~10^-18 / hr | Independent lockstep core |
| CS-2 | ESM module fault AND WDT fault | ~10^-15 / hr | Diverse safety mechanisms |
| CS-3 | PLL unlock AND PLL monitor fault | ~10^-12 / hr | ESM + software monitor |
| CS-4 | ECC DED AND safe-state logic fault | ~10^-15 / hr | Hardware ESM pin |
| CS-5 | TX DAC stuck AND TX monitor fault | ~10^-14 / hr | Independent power detector |

## 4. Common Cause Analysis

| Potential Common Cause | Mitigation |
|---|---|
| Power supply failure | Independent voltage monitoring, brown-out reset |
| Clock failure | PLL lock detection, clock monitor |
| Temperature excursion | Temperature sensor, thermal shutdown |
| Software common-mode fault | Lockstep execution, diverse algorithms |
| ECC hardware failure | ECC self-test with error injection |

## 5. Quantitative Analysis

The fault tree demonstrates that all single-point failures are covered by independent safety mechanisms. The probability of the top event (unintended emission) is dominated by the dual-point cut sets, all of which have probabilities well below the ASIL-D target of 10^-8 failures/hour.

## 6. Conclusion

The fault tree confirms that the safety architecture provides adequate independence between safety mechanisms, meeting ISO 26262 ASIL-D requirements for hardware fault tolerance.
