/*
 * Copyright (c) 2012, 2014, 2020 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2004-2006 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __CPU_O3_MEM_DEP_UNIT_HH__
#define __CPU_O3_MEM_DEP_UNIT_HH__

#include <list>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "base/statistics.hh"
#include "cpu/inst_seq.hh"
#include "cpu/o3/dyn_inst_ptr.hh"
#include "cpu/o3/limits.hh"
#include "cpu/o3/store_set.hh"
#include "debug/MemDepUnit.hh"

namespace gem5
{

struct SNHash
{
    size_t
    operator()(const InstSeqNum &seq_num) const
    {
        unsigned a = (unsigned)seq_num;
        unsigned hash = (((a >> 14) ^ ((a >> 2) & 0xffff))) & 0x7FFFFFFF;
        return hash;
    }
};

struct BaseO3CPUParams;

namespace o3
{

class CPU;
class InstructionQueue;

class MemDepUnit
{
  protected:
    std::string _name;

  public:
    MemDepUnit();
    MemDepUnit(const BaseO3CPUParams &params);
    ~MemDepUnit();

    std::string name() const { return _name; }

    void init(const BaseO3CPUParams &params, ThreadID tid, CPU *cpu);
    bool isDrained() const;
    void drainSanityCheck() const;
    void takeOverFrom();
    void setIQ(InstructionQueue *iq_ptr);

    void insert(const DynInstPtr &inst);
    void insertNonSpec(const DynInstPtr &inst);
    void insertBarrier(const DynInstPtr &barr_inst);
    void regsReady(const DynInstPtr &inst);
    void nonSpecInstReady(const DynInstPtr &inst);
    void reschedule(const DynInstPtr &inst);
    void replay();
    void completeInst(const DynInstPtr &inst);
    void squash(const InstSeqNum &squashed_num, ThreadID tid);
    void violation(const DynInstPtr &store_inst,
                   const DynInstPtr &violating_load);
    void issue(const DynInstPtr &inst);
    void dumpLists();

  private:
    void completed(const DynInstPtr &inst);
    void wakeDependents(const DynInstPtr &inst);

    typedef typename std::list<DynInstPtr>::iterator ListIt;

    class MemDepEntry;

    typedef std::shared_ptr<MemDepEntry> MemDepEntryPtr;

    class MemDepEntry
    {
      public:
        MemDepEntry(const DynInstPtr &new_inst);
        ~MemDepEntry();

        std::string name() const { return "memdepentry"; }

        DynInstPtr inst;
        ListIt listIt;
        std::vector<MemDepEntryPtr> dependInsts;
        bool regsReady = false;
        int memDeps = 0;
        bool completed = false;
        bool squashed = false;

#ifdef GEM5_DEBUG
        static int memdep_count;
        static int memdep_insert;
        static int memdep_erase;
#endif
    };

    MemDepEntryPtr *findInHash(const DynInstConstPtr& inst);
    void moveToReady(MemDepEntryPtr &ready_inst_entry);

    typedef std::unordered_map<InstSeqNum, MemDepEntryPtr, SNHash> MemDepHash;
    typedef typename MemDepHash::iterator MemDepHashIt;

    MemDepHash memDepHash;
    std::list<DynInstPtr> instList[MaxThreads];
    std::list<DynInstPtr> instsToReplay;
    StoreSet depPred;
    std::unordered_set<InstSeqNum> loadBarrierSNs;
    std::unordered_set<InstSeqNum> storeBarrierSNs;

    bool hasLoadBarrier() const { return !loadBarrierSNs.empty(); }
    bool hasStoreBarrier() const { return !storeBarrierSNs.empty(); }

    void insertBarrierSN(const DynInstPtr &barr_inst);

    // Fast-path cache for STT conservative dependence:
    // latest older store/atomic/barrier/HTM-like producer per thread.
    InstSeqNum latestStoreLikeSeqNum[MaxThreads];

    InstructionQueue *iqPtr;
    bool sttEnabled;
    bool implicitChannelEnabled;
    int id;

    struct MemDepUnitStats : public statistics::Group
    {
        MemDepUnitStats(statistics::Group *parent);
        statistics::Scalar insertedLoads;
        statistics::Scalar insertedStores;
        statistics::Scalar conflictingLoads;
        statistics::Scalar conflictingStores;
    } stats;
};

} // namespace o3
} // namespace gem5

#endif // __CPU_O3_MEM_DEP_UNIT_HH__