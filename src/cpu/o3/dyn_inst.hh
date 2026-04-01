#ifndef __CPU_O3_DYN_INST_HH__
#define __CPU_O3_DYN_INST_HH__

#include <algorithm>
#include <array>
#include <deque>
#include <list>
#include <string>
#include <vector>

#include "base/refcnt.hh"
#include "base/trace.hh"
#include "cpu/checker/cpu.hh"
#include "cpu/exec_context.hh"
#include "cpu/exetrace.hh"
#include "cpu/inst_res.hh"
#include "cpu/inst_seq.hh"
#include "cpu/o3/cpu.hh"
#include "cpu/o3/dyn_inst_ptr.hh"
#include "cpu/o3/lsq_unit.hh"
#include "cpu/op_class.hh"
#include "cpu/reg_class.hh"
#include "cpu/static_inst.hh"
#include "cpu/translation.hh"
#include "debug/DynInst.hh"
#include "debug/HtmCpu.hh"

namespace gem5
{

class Packet;

namespace o3
{

class IQUnit;
class LSQUnit;
class ThreadState;

class DynInst : public ExecContext, public RefCounted
{
  private:
    DynInst(const StaticInstPtr &staticInst, const StaticInstPtr &macroop,
            InstSeqNum seq_num, CPU *cpu);

  public:
    typedef typename std::list<DynInstPtr>::iterator ListIt;

    struct Arrays
    {
        size_t numSrcs;
        size_t numDests;
        RegId *flatDestIdx;
        PhysRegIdPtr *destIdx;
        PhysRegIdPtr *prevDestIdx;
        PhysRegIdPtr *srcIdx;
        uint8_t *readySrcIdx;
    };

    static void *operator new(size_t count, Arrays &arrays);
    static void operator delete(void* ptr);

    DynInst(const Arrays &arrays, const StaticInstPtr &staticInst,
            const StaticInstPtr &macroop, InstSeqNum seq_num, CPU *cpu);

    DynInst(const Arrays &arrays, const StaticInstPtr &staticInst,
            const StaticInstPtr &macroop, const PCStateBase &pc,
            const PCStateBase &pred_pc, InstSeqNum seq_num, CPU *cpu);

    DynInst(const Arrays &arrays, const StaticInstPtr &_staticInst,
            const StaticInstPtr &_macroop);

    ~DynInst();

    Fault execute();
    Fault initiateAcc();
    Fault completeAcc(PacketPtr pkt);

    InstSeqNum seqNum = 0;
    const StaticInstPtr staticInst;
    CPU *cpu = nullptr;
    BaseCPU *getCpuPtr();
    ThreadState *thread = nullptr;
    Fault fault = NoFault;
    trace::InstRecord *traceData = nullptr;

    bool dataTainted = false;
    bool addrTainted = false;
    bool argsTainted = false;
    bool controlTainted = false;
    bool explicitShadowedLoad = false;
    bool explicitShadowedStore = false;
  protected:
    enum Status
    {
        IqEntry,
        RobEntry,
        LsqEntry,
        Completed,
        ResultReady,
        CanIssue,
        Issued,
        Executed,
        CanCommit,
        AtCommit,
        Committed,
        Squashed,
        SquashedInIQ,
        SquashedInLSQ,
        SquashedInROB,
        PinnedRegsRenamed,
        PinnedRegsWritten,
        PinnedRegsSquashDone,
        RecoverInst,
        BlockingInst,
        ThreadsyncWait,
        SerializeBefore,
        SerializeAfter,
        SerializeHandled,
        NumStatus
    };

    enum Flags
    {
        NotAnInst,
        TranslationStarted,
        TranslationCompleted,
        PossibleLoadViolation,
        HitExternalSnoop,
        EffAddrValid,
        RecordResult,
        Predicate,
        MemAccPredicate,
        PredTaken,
        IsStrictlyOrdered,
        ReqMade,
        MemOpDone,
        HtmFromTransaction,
        NoCapableFU,
        MaxFlags
    };

  private:
    std::bitset<MaxFlags> instFlags;
    std::bitset<NumStatus> status;

  protected:
    std::queue<InstResult> instResult;
    std::unique_ptr<PCStateBase> pc;
    std::vector<RegVal> _destMiscRegVal;
    std::vector<short> _destMiscRegIdx;
    size_t _numSrcs;
    size_t _numDests;
    RegId *_flatDestIdx;
    PhysRegIdPtr *_destIdx;
    PhysRegIdPtr *_prevDestIdx;
    PhysRegIdPtr *_srcIdx;
    uint8_t *_readySrcIdx;

  public:
    size_t numSrcs() const { return _numSrcs; }
    size_t numDests() const { return _numDests; }

    const RegId &flattenedDestIdx(int idx) const { return _flatDestIdx[idx]; }
    void flattenedDestIdx(int idx, const RegId &reg_id) { _flatDestIdx[idx] = reg_id; }

    PhysRegIdPtr renamedDestIdx(int idx) const { return _destIdx[idx]; }
    void renamedDestIdx(int idx, PhysRegIdPtr phys_reg_id) { _destIdx[idx] = phys_reg_id; }

    PhysRegIdPtr prevDestIdx(int idx) const { return _prevDestIdx[idx]; }
    void prevDestIdx(int idx, PhysRegIdPtr phys_reg_id) { _prevDestIdx[idx] = phys_reg_id; }

    PhysRegIdPtr renamedSrcIdx(int idx) const { return _srcIdx[idx]; }
    void renamedSrcIdx(int idx, PhysRegIdPtr phys_reg_id) { _srcIdx[idx] = phys_reg_id; }

    bool readySrcIdx(int idx) const
    {
        uint8_t &byte = _readySrcIdx[idx / 8];
        return bits(byte, idx % 8);
    }

    void readySrcIdx(int idx, bool ready)
    {
        uint8_t &byte = _readySrcIdx[idx / 8];
        replaceBits(byte, idx % 8, ready ? 1 : 0);
    }

    ThreadID threadNumber = 0;
    ListIt instListIt;
    std::unique_ptr<PCStateBase> predPC;
    const StaticInstPtr macroop;
    uint8_t readyRegs = 0;

  public:
    Addr effAddr = 0;
    Addr physEffAddr = 0;
    unsigned memReqFlags = 0;
    unsigned effSize;
    uint8_t *memData = nullptr;
    ssize_t lqIdx = -1;
    typename LSQUnit::LQIterator lqIt;
    ssize_t sqIdx = -1;
    typename LSQUnit::SQIterator sqIt;
    LSQ::LSQRequest *savedRequest;
    RequestPtr reqToVerify;

  public:
    void recordResult(bool f) { instFlags[RecordResult] = f; }
    bool effAddrValid() const { return instFlags[EffAddrValid]; }
    void effAddrValid(bool b) { instFlags[EffAddrValid] = b; }
    bool memOpDone() const { return instFlags[MemOpDone]; }
    void memOpDone(bool f) { instFlags[MemOpDone] = f; }
    bool notAnInst() const { return instFlags[NotAnInst]; }
    void setNotAnInst() { instFlags[NotAnInst] = true; }

    bool isDataTainted() const { return dataTainted; }
    bool isAddrTainted() const { return addrTainted; }
    bool isArgsTainted() const { return argsTainted; }
    bool isControlTainted() const { return controlTainted; }
    bool isExplicitShadowedLoad() const { return explicitShadowedLoad; }
    void setExplicitShadowedLoad(bool v) { explicitShadowedLoad = v; }
    bool isExplicitShadowedStore() const { return explicitShadowedStore; }
    void setExplicitShadowedStore(bool v) { explicitShadowedStore = v; }

    void setDataTainted(bool v) { dataTainted = v; }
    void setAddrTainted(bool v) { addrTainted = v; }
    void setArgsTainted(bool v) { argsTainted = v; }
    void setControlTainted(bool v) { controlTainted = v; }

    void clearTaint()
    {
        dataTainted = false;
        addrTainted = false;
        argsTainted = false;
        controlTainted = false;
    }

    void demapPage(Addr vaddr, uint64_t asn) override;

    Fault initiateMemRead(Addr addr, unsigned size, Request::Flags flags,
            const std::vector<bool> &byte_enable) override;
    Fault initiateMemMgmtCmd(Request::Flags flags) override;
    Fault writeMem(uint8_t *data, unsigned size, Addr addr,
                   Request::Flags flags, uint64_t *res,
                   const std::vector<bool> &byte_enable) override;
    Fault initiateMemAMO(Addr addr, unsigned size, Request::Flags flags,
                         AtomicOpFunctorPtr amo_op) override;

    bool translationStarted() const { return instFlags[TranslationStarted]; }
    void translationStarted(bool f) { instFlags[TranslationStarted] = f; }

    bool translationCompleted() const { return instFlags[TranslationCompleted]; }
    void translationCompleted(bool f) { instFlags[TranslationCompleted] = f; }

    bool possibleLoadViolation() const { return instFlags[PossibleLoadViolation]; }
    void possibleLoadViolation(bool f) { instFlags[PossibleLoadViolation] = f; }

    bool hitExternalSnoop() const { return instFlags[HitExternalSnoop]; }
    void hitExternalSnoop(bool f) { instFlags[HitExternalSnoop] = f; }

    bool isTranslationDelayed() const
    {
        return (translationStarted() && !translationCompleted());
    }

  public:
#ifdef GEM5_DEBUG
    void dumpSNList();
#endif

    void
    renameDestReg(int idx, PhysRegIdPtr renamed_dest,
                  PhysRegIdPtr previous_rename)
    {
        renamedDestIdx(idx, renamed_dest);
        prevDestIdx(idx, previous_rename);
        if (renamed_dest->isPinned())
            setPinnedRegsRenamed();
    }

    void renameSrcReg(int idx, PhysRegIdPtr renamed_src)
    {
        renamedSrcIdx(idx, renamed_src);
    }

    void dump();
    void dump(std::string &outstring);

    int cpuId() const;
    uint32_t socketId() const;
    RequestorID requestorId() const;
    ContextID contextId() const;

    Fault getFault() const { return fault; }
    Fault& getFault() { return fault; }

    bool doneTargCalc() { return false; }
    void setPredTarg(const PCStateBase &pred_pc) { set(predPC, pred_pc); }
    const PCStateBase &readPredTarg() { return *predPC; }

    bool readPredTaken() const { return instFlags[PredTaken]; }
    void setPredTaken(bool predicted_taken) { instFlags[PredTaken] = predicted_taken; }

    bool mispredicted() const
    {
        std::unique_ptr<PCStateBase> next_pc(pc->clone());
        staticInst->advancePC(*next_pc);
        return *next_pc != *predPC;
    }

    bool isNop() const { return staticInst->isNop(); }
    bool isMemRef() const { return staticInst->isMemRef(); }
    bool isLoad() const { return staticInst->isLoad(); }
    bool isStore() const { return staticInst->isStore(); }
    bool isAtomic() const { return staticInst->isAtomic(); }
    bool isStoreConditional() const { return staticInst->isStoreConditional(); }
    bool isInstPrefetch() const { return staticInst->isInstPrefetch(); }
    bool isDataPrefetch() const { return staticInst->isDataPrefetch(); }
    bool isInteger() const { return staticInst->isInteger(); }
    bool isFloating() const { return staticInst->isFloating(); }
    bool isVector() const { return staticInst->isVector(); }
    bool isControl() const { return staticInst->isControl(); }
    bool isCall() const { return staticInst->isCall(); }
    bool isReturn() const { return staticInst->isReturn(); }
    bool isDirectCtrl() const { return staticInst->isDirectCtrl(); }
    bool isIndirectCtrl() const { return staticInst->isIndirectCtrl(); }
    bool isCondCtrl() const { return staticInst->isCondCtrl(); }
    bool isUncondCtrl() const { return staticInst->isUncondCtrl(); }
    bool isSerializing() const { return staticInst->isSerializing(); }
    bool isSerializeBefore() const
    {
        return staticInst->isSerializeBefore() || status[SerializeBefore];
    }
    bool isSerializeAfter() const
    {
        return staticInst->isSerializeAfter() || status[SerializeAfter];
    }
    bool isSquashAfter() const { return staticInst->isSquashAfter(); }
    bool isFullMemBarrier() const { return staticInst->isFullMemBarrier(); }
    bool isReadBarrier() const { return staticInst->isReadBarrier(); }
    bool isWriteBarrier() const { return staticInst->isWriteBarrier(); }
    bool isNonSpeculative() const { return staticInst->isNonSpeculative(); }
    bool isQuiesce() const { return staticInst->isQuiesce(); }
    bool isUnverifiable() const { return staticInst->isUnverifiable(); }
    bool isSyscall() const { return staticInst->isSyscall(); }
    bool isMacroop() const { return staticInst->isMacroop(); }
    bool isMicroop() const { return staticInst->isMicroop(); }
    bool isDelayedCommit() const { return staticInst->isDelayedCommit(); }
    bool isLastMicroop() const { return staticInst->isLastMicroop(); }
    bool isFirstMicroop() const { return staticInst->isFirstMicroop(); }
    bool isHtmStart() const { return staticInst->isHtmStart(); }
    bool isHtmStop() const { return staticInst->isHtmStop(); }
    bool isHtmCancel() const { return staticInst->isHtmCancel(); }
    bool isHtmCmd() const { return staticInst->isHtmCmd(); }

    uint64_t getHtmTransactionUid() const override
    {
        assert(instFlags[HtmFromTransaction]);
        return htmUid;
    }

    uint64_t newHtmTransactionUid() const override
    {
        panic("Not yet implemented\n");
        return 0;
    }

    bool inHtmTransactionalState() const override
    {
        return instFlags[HtmFromTransaction];
    }

    uint64_t getHtmTransactionalDepth() const override
    {
        return inHtmTransactionalState() ? htmDepth : 0;
    }

    void setHtmTransactionalState(uint64_t htm_uid, uint64_t htm_depth)
    {
        instFlags.set(HtmFromTransaction);
        htmUid = htm_uid;
        htmDepth = htm_depth;
    }

    void clearHtmTransactionalState()
    {
        if (inHtmTransactionalState()) {
            DPRINTF(HtmCpu,
                "clearing instuction's transactional state htmUid=%u\n",
                getHtmTransactionUid());
            instFlags.reset(HtmFromTransaction);
            htmUid = -1;
            htmDepth = 0;
        }
    }

    void setSerializeBefore() { status.set(SerializeBefore); }
    void clearSerializeBefore() { status.reset(SerializeBefore); }
    bool isTempSerializeBefore() { return status[SerializeBefore]; }
    void setSerializeAfter() { status.set(SerializeAfter); }
    void clearSerializeAfter() { status.reset(SerializeAfter); }
    bool isTempSerializeAfter() { return status[SerializeAfter]; }
    void setSerializeHandled() { status.set(SerializeHandled); }
    bool isSerializeHandled() { return status[SerializeHandled]; }

    OpClass opClass() const { return staticInst->opClass(); }

    std::unique_ptr<PCStateBase> branchTarget() const
    {
        return staticInst->branchTarget(*pc);
    }

    size_t numSrcRegs() const { return numSrcs(); }
    size_t numDestRegs() const { return numDests(); }

    size_t numDestRegs(RegClassType type) const
    {
        return staticInst->numDestRegs(type);
    }

    const RegId& destRegIdx(int i) const { return staticInst->destRegIdx(i); }
    const RegId& srcRegIdx(int i) const { return staticInst->srcRegIdx(i); }

    uint8_t resultSize() { return instResult.size(); }

    InstResult popResult(InstResult dflt=InstResult())
    {
        if (!instResult.empty()) {
            InstResult t = instResult.front();
            instResult.pop();
            return t;
        }
        return dflt;
    }

    template<typename T>
    void setResult(const RegClass &reg_class, T &&t)
    {
        if (instFlags[RecordResult]) {
            instResult.emplace(reg_class, std::forward<T>(t));
        }
    }

    void markSrcRegReady();
    void markSrcRegReady(RegIndex src_idx);

    void setCompleted() { status.set(Completed); }
    bool isCompleted() const { return status[Completed]; }
    void setResultReady() { status.set(ResultReady); }
    bool isResultReady() const { return status[ResultReady]; }
    void setCanIssue() { status.set(CanIssue); }
    bool readyToIssue() const { return status[CanIssue]; }
    void clearCanIssue() { status.reset(CanIssue); }
    void setIssued() { status.set(Issued); }
    bool isIssued() const { return status[Issued]; }
    void clearIssued() { status.reset(Issued); }
    void setExecuted() { status.set(Executed); }
    bool isExecuted() const { return status[Executed]; }
    void setCanCommit() { status.set(CanCommit); }
    void clearCanCommit() { status.reset(CanCommit); }
    bool readyToCommit() const { return status[CanCommit]; }
    void setAtCommit() { status.set(AtCommit); }
    bool isAtCommit() { return status[AtCommit]; }
    void setCommitted() { status.set(Committed); }
    bool isCommitted() const { return status[Committed]; }
    void setSquashed();
    bool isSquashed() const { return status[Squashed]; }

    void setInIQ(IQUnit *_iq)
{
    iq = _iq;
}

    void clearInIQ();

    bool isInIQ() const { return status[IqEntry]; }
    void setSquashedInIQ() { status.set(SquashedInIQ); status.set(Squashed); }
    bool isSquashedInIQ() const { return status[SquashedInIQ]; }
    IQUnit *iq = nullptr;

    void setInLSQ() { status.set(LsqEntry); }
    void removeInLSQ() { status.reset(LsqEntry); }
    bool isInLSQ() const { return status[LsqEntry]; }
    void setSquashedInLSQ() { status.set(SquashedInLSQ); status.set(Squashed); }
    bool isSquashedInLSQ() const { return status[SquashedInLSQ]; }

    void setInROB() { status.set(RobEntry); }
    void clearInROB() { status.reset(RobEntry); }
    bool isInROB() const { return status[RobEntry]; }
    void setSquashedInROB() { status.set(SquashedInROB); }
    bool isSquashedInROB() const { return status[SquashedInROB]; }

    void setNoCapableFU() { instFlags.set(NoCapableFU); }
    bool noCapableFU() const { return instFlags[NoCapableFU]; }

    bool isPinnedRegsRenamed() const { return status[PinnedRegsRenamed]; }

    void setPinnedRegsRenamed()
    {
        assert(!status[PinnedRegsSquashDone]);
        assert(!status[PinnedRegsWritten]);
        status.set(PinnedRegsRenamed);
    }

    bool isPinnedRegsWritten() const { return status[PinnedRegsWritten]; }

    void setPinnedRegsWritten()
    {
        assert(!status[PinnedRegsSquashDone]);
        assert(status[PinnedRegsRenamed]);
        status.set(PinnedRegsWritten);
    }

    bool isPinnedRegsSquashDone() const
    {
        return status[PinnedRegsSquashDone];
    }

    void setPinnedRegsSquashDone()
    {
        assert(!status[PinnedRegsSquashDone]);
        status.set(PinnedRegsSquashDone);
    }

    const PCStateBase &pcState() const override { return *pc; }
    void pcState(const PCStateBase &val) override { set(pc, val); }

    bool readPredicate() const override { return instFlags[Predicate]; }

    void setPredicate(bool val) override
    {
        instFlags[Predicate] = val;
        if (traceData) {
            traceData->setPredicate(val);
        }
    }

    bool readMemAccPredicate() const override
    {
        return instFlags[MemAccPredicate];
    }

    void setMemAccPredicate(bool val) override
    {
        instFlags[MemAccPredicate] = val;
    }

    void setTid(ThreadID tid) { threadNumber = tid; }
    void setThreadState(ThreadState *state) { thread = state; }
    gem5::ThreadContext *tcBase() const override;

  public:
    bool strictlyOrdered() const { return instFlags[IsStrictlyOrdered]; }
    void strictlyOrdered(bool so) { instFlags[IsStrictlyOrdered] = so; }
    bool hasRequest() const { return instFlags[ReqMade]; }
    void setRequest() { instFlags[ReqMade] = true; }
    ListIt &getInstListIt() { return instListIt; }
    void setInstListIt(ListIt _instListIt) { instListIt = _instListIt; }

  public:
    unsigned int readStCondFailures() const override;
    void setStCondFailures(unsigned int sc_failures) override;

  public:
    void armMonitor(Addr address) override;
    bool mwait(PacketPtr pkt) override;
    void mwaitAtomic(gem5::ThreadContext *tc) override;
    AddressMonitor *getAddrMonitor() override;

  private:
    uint64_t htmUid = -1;
    uint64_t htmDepth = 0;

  public:
    Tick fetchTick = -1;
    int32_t decodeTick = -1;
    int32_t renameTick = -1;
    int32_t renameEndTick = -1;
    int32_t dispatchTick = -1;
    int32_t issueTick = -1;
    int32_t completeTick = -1;
    int32_t commitTick = -1;
    int32_t storeTick = -1;
    Tick firstIssue = -1;
    Tick lastWakeDependents = -1;

    RegVal readMiscReg(int misc_reg) override;
    void setMiscReg(int misc_reg, RegVal val) override;
    RegVal readMiscRegOperand(const StaticInst *si, int idx) override;
    void setMiscRegOperand(const StaticInst *si, int idx, RegVal val) override;
    void updateMiscRegs();
    void forwardOldRegs();
    void trap(const Fault &fault);

  public:
    RegVal getRegOperand(const StaticInst *si, int idx) override;
    void getRegOperand(const StaticInst *si, int idx, void *val) override;
    void *getWritableRegOperand(const StaticInst *si, int idx) override;
    void setRegOperand(const StaticInst *si, int idx, RegVal val) override;
    void setRegOperand(const StaticInst *si, int idx, const void *val) override;
};

} // namespace o3
} // namespace gem5

#endif // __CPU_O3_DYN_INST_HH__