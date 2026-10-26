# Software Development Lifecycle

## AWR2944 mmWave Radar Safety Firmware

### Document Information
| Field | Value |
|---|---|
| Document ID | SDLC-AWR2944-001 |
| Version | 1.0 |
| Standard | ISO 26262-6 (Software Level) |
| Classification | ASIL-D |

---

## 1. Overview

This document describes the software development lifecycle (SDLC) applied to the AWR2944 radar safety firmware, in compliance with ISO 26262-6 and IEC 61508-3. All software development activities follow a V-Model approach with full traceability from requirements through design, implementation, and verification.

## 2. V-Model Process

```
  Requirements         <--->        System Qualification Tests
      |                                      ^
      v                                      |
  Architecture Design  <--->   Integration Tests
      |                                      ^
      v                                      |
  Detailed Design      <--->   Unit Tests
      |                                      ^
      v                                      |
  Implementation  ---------> Code Review
```

## 3. Phase 1: Software Safety Requirements

### 3.1 Activities
- Derive software safety requirements from Technical Safety Concept (TSC)
- Assign ASIL to each software safety requirement
- Establish traceability matrix (requirement ID -> TSC ID -> safety goal)

### 3.2 Work Products
| Work Product | Format | Tool |
|---|---|---|
| SW Safety Requirements Spec | DOORS module | IBM DOORS |
| Traceability Matrix | DOORS links | IBM DOORS |
| Requirements Review Report | PDF | Peer review |

### 3.3 Methods (per ISO 26262-6 Table 2)
- Semi-formal notation: **Recommended (applied)**
- Formal notation: Not applied (rationale documented)
- Back-to-back comparison with TSC: **Applied**

## 4. Phase 2: Architecture Design

### 4.1 Activities
- Design modular safety architecture (see safety_manual.md Section 2)
- Define interfaces between safety and non-safety modules
- Allocate safety mechanisms to software components
- Perform DFA on software architecture

### 4.2 Design Principles (per ISO 26262-6 Table 3)
| Principle | Status |
|---|---|
| Hierarchical structure | Applied (HAL -> Drivers -> Services -> App) |
| Restricted size of software components | Applied (max 200 LOC per function) |
| Restricted size of interfaces | Applied (max 6 parameters per function) |
| Strong cohesion within components | Applied |
| Loose coupling between components | Applied |
| Static scheduling | Applied (cooperative round-robin) |
| Restricted use of interrupts | Applied (safety-critical ISRs only) |
| No dynamic memory allocation | Applied (all static allocation) |

### 4.3 Freedom from Interference
Mechanisms ensuring freedom from interference between ASIL-D and non-ASIL modules:
- MPU-based memory protection
- Stack guard patterns
- Static memory allocation (no heap)
- Separate address spaces for safety and QM code

## 5. Phase 3: Detailed Design & Implementation

### 5.1 Coding Standard
All code shall comply with **MISRA-C:2012** (mandatory rules + required rules). Advisory rules are followed unless a documented deviation exists.

### 5.2 Coding Guidelines (per ISO 26262-6 Table 8)
| Guideline | Status |
|---|---|
| Enforcement of low complexity | Applied (cyclomatic complexity <= 15) |
| Use of language subsets (MISRA) | Applied |
| Enforcement of strong typing | Applied (platform_types.h) |
| No dynamic objects or variables | Applied |
| No hidden data flow / control flow | Applied |
| No unconditional jumps | Applied |
| No recursion | Applied |

### 5.3 Tools
| Tool | Purpose | Qualification |
|---|---|---|
| TI ARM CGT v20.x | Compiler | Tool qualification per ISO 26262-8 Clause 11 |
| PC-lint Plus | Static analysis (MISRA) | Validated against MISRA test suite |
| gcov/lcov | Code coverage | Cross-checked with manual inspection |
| Python 3.x | Build scripts | Non-safety (build-time only) |

## 6. Phase 4: Verification

### 6.1 Unit Testing (per ISO 26262-6 Table 10)
| Method | Status |
|---|---|
| Requirements-based testing | Applied (each requirement has >= 1 test) |
| Interface testing | Applied |
| Fault injection testing | Applied (ECC error injection, WDT timeout) |
| Resource usage testing | Applied (stack watermark, CPU load) |
| Back-to-back testing | Applied (model vs. target comparison) |

### 6.2 Structural Coverage (per ISO 26262-6 Table 12)
| Level | Target | Achieved |
|---|---|---|
| Statement coverage | 100% | 100% |
| Branch coverage | 100% | 100% |
| MC/DC coverage | >= 90% | 92% |

### 6.3 Integration Testing
- Top-down integration following the architecture hierarchy
- Full data path test (ADC -> DMA -> processing -> output)
- Full safety chain test (fault injection -> detection -> safe state)

### 6.4 Qualification Testing
- ASIL-D requirement traceability verification
- Worst-case execution time (WCET) analysis
- Safety mechanism effectiveness testing

## 7. Configuration Management

- Version control: Git (with signed commits)
- Branching model: GitFlow (main, develop, feature/*, release/*)
- Code review: Mandatory peer review before merge to develop
- Release approval: Safety manager sign-off required

## 8. Change Management

All changes to safety-critical code require:
1. Impact analysis on safety requirements
2. Regression test plan
3. Code review by independent reviewer
4. Updated traceability matrix
5. Safety manager approval

## 9. Document Traceability

```
Safety Goals (SG)
  |
  v
Technical Safety Concept (TSC)
  |
  v
SW Safety Requirements (SRS)
  |
  +---> Architecture Design (SAD)
  |         |
  |         v
  |     Detailed Design (SDD)
  |         |
  |         v
  |     Source Code
  |
  +---> Unit Test Cases
  +---> Integration Test Cases
  +---> Qualification Test Cases
```

## 10. Revision History

| Version | Date | Author | Changes |
|---|---|---|---|
| 1.0 | 2024-01-15 | Safety Team | Initial release |
