# Implementation Notes: STT in gem5 O3

## Overview

This repository contains a focused implementation study of Speculative Taint Tracking (STT) on top of the gem5 O3 CPU model. The implementation focuses on taint propagation, speculative control handling, and suppression of unsafe speculative stores.

Most changes are concentrated in `DynInst` state, commit logic, `LSQ` / `LSQUnit`, and squash-related bookkeeping.

## Core Mechanisms

- **DynInst taint metadata**  
  Taint flags for argument, control, data, and address dependencies are attached directly to `DynInst`. Later pipeline stages use these flags to propagate taint, trigger squashes, and block memory-side effects.

- **Commit-stage squashes**  
  The commit stage is treated as the architectural boundary for speculation. When a tainted control instruction reaches commit, the implementation clears the fetch window and squashes younger instructions fetched under that tainted control context.

- **LSQ shadow stores**  
  Unsafe speculative stores are converted into zero-size shadow entries in the store queue. From the pipeline point of view, these stores execute and retire normally, but the modified writeback path consumes the zero-size entries without issuing a real memory request.

## Bookkeeping Fixes

Introducing zero-size shadow stores into the LSQ required several bookkeeping fixes to preserve forward progress.

- **HEAD_FAULT deadlock**  
  The original commit logic still treated zero-size shadow stores as pending older stores. This caused repeated `HEAD_FAULT` and `FAULT_WAIT` loops at commit.  
  **Fix:** outstanding-store checks were updated to ignore zero-size shadow entries.

- **`storesToWB` leaks on squash**  
  If a younger store had already been marked writable and was later squashed, the global `storesToWB` counter was not repaired. This left the processor waiting on non-existent store work.  
  **Fix:** squash handling was updated to decrement `storesToWB` for squashed entries that had been counted but not completed.

- **`storeInFlight` stalls from baseline O3 behavior**  
  Some commit stalls were not caused by the STT changes, but by gem5 O3’s baseline memory-ordering behavior. In these cases, `hasStoresToWB()` remained true because an older real store was still in flight.  
  This case had to be distinguished from shadow-store bookkeeping bugs during debugging.