# Implementation Notes: STT in gem5 O3

## Overview

This repository contains our implementation study of Speculative Taint Tracking (STT) in the gem5 O3 CPU model. The implementation focuses on three core behaviors: taint propagation through dynamic instructions, commit-stage handling of tainted speculative control, and suppression of unsafe speculative store side effects.

We modified dynamic instruction metadata, commit logic, load-store queue behavior, store writeback handling, and squash-time repair of store accounting state.

## Main Modified Areas

The implementation is centered around the following components:

- `DynInst` metadata for taint state
- commit-stage control handling in the ROB/commit path
- LSQ and `LSQUnit` handling for speculative shadow stores
- squash-time repair of outstanding store bookkeeping
- interaction with baseline O3 store ordering state such as `storeInFlight`

## 1. Dynamic Instruction Taint Metadata

Taint is represented directly in the dynamic instruction state rather than in a separate architectural shadow structure.

The implementation distinguishes multiple taint categories:

- argument taint
- control taint
- data taint
- address taint

These flags are attached to each dynamic instruction and are checked by later pipeline stages. This allows taint state to move with the instruction as it passes through fetch, rename, issue, execute, LSQ, and commit.

This separation is used to distinguish different speculative risks inside the pipeline. For example:

- address taint is relevant to cache- and memory-visible accesses
- control taint is relevant to speculative path formation and control cleanup
- data taint is relevant to transmit-style dependencies
- argument taint is used for dependency-based propagation

## 2. Commit-Stage Control Handling

The commit stage is treated as the boundary between speculative and non-speculative execution.

When a tainted control instruction reaches commit, younger instructions that were fetched and scheduled under that speculative control context must not continue execution past that point. Our implementation therefore adds commit-stage handling that:

- detects committed tainted control
- requests a fetch window clear
- squashes younger instructions after commit

This behavior is used to clean up stale speculative descendants after a tainted control boundary is resolved. In this implementation, commit is not only the retirement point for instructions, but also the point where speculative control context is finalized and pipeline cleanup is enforced.

## 3. Shadow Store Mechanism

Unsafe speculative stores are suppressed through a shadow-store path in the store queue.

When a store is identified as a tainted speculative transmitter, it is prevented from taking the normal memory writeback path. Instead, the implementation:

- marks the dynamic instruction as executed and committable from the pipeline point of view
- converts the corresponding store queue entry into a zero-size shadow entry
- avoids constructing and sending a real memory write request

In the modified writeback path, zero-size store entries are consumed internally and completed without issuing a real packet to the cache or memory system.

This keeps the store queue moving while preventing speculative tainted stores from creating memory-visible side effects.

## 4. Zero-Size Shadow Store Forward-Progress Bug

The first major correctness issue came from the interaction between shadow stores and commit-stage outstanding-store checks.

In the baseline O3 model, a faulting instruction at the head of the ROB can only retire after older stores are considered drained. After introducing zero-size shadow stores, we observed a case where:

- a speculative store had already been converted into a zero-size shadow entry
- the entry no longer represented real memory-side work
- but the commit logic still treated it as an outstanding older store

This caused faulting head instructions to remain in repeated `HEAD_FAULT` / `FAULT_WAIT` cycles because commit continued to believe that older store work was still pending.

The fix was to modify the outstanding-store check so that zero-size shadow entries are ignored when they no longer correspond to real memory writeback work.

## 5. Squash and `storesToWB` Repair

A second correctness issue appeared during squash handling.

In the O3 store queue, a younger store may already have been marked writable before it is later invalidated by branch recovery or another squash event. If that entry is removed from the queue without repairing global store bookkeeping, the processor can continue to believe that pending store work still exists.

The relevant case is:

- a younger store contributes to `storesToWB`
- the store is later squashed before completion
- the entry is removed
- but the global counter is not decremented

This leaves stale outstanding-store state behind and can block forward progress.

The squash path was therefore modified so that younger stores are removed even if already marked writable, and `storesToWB` is decremented when necessary for uncompleted squashed entries.

## 6. `storeInFlight` and TSO-Style Waiting

After repairing zero-size shadow-store accounting and squash-time `storesToWB` leaks, we still observed cases where commit remained blocked waiting for older stores.

These cases were not caused by STT bookkeeping bugs. Instead, they came from baseline O3 store-ordering behavior, particularly the `storeInFlight` state used by the model’s store ordering rules.

In these cases:

- `hasStoresToWB()` still returns true
- a faulting head instruction remains in a wait state
- but the wait is caused by a real older store already in flight
- not by a broken shadow-store placeholder

This distinction mattered during debugging because the symptom looked similar to the earlier forward-progress bug, while the cause was different.

## Notes on Current Scope

This implementation is a focused integration study inside gem5 O3. It concentrates on:

- taint metadata attached to dynamic instructions
- commit-stage cleanup for tainted speculative control
- suppression of unsafe speculative stores through zero-size shadow entries
- repair of LSQ bookkeeping required to preserve forward progress
