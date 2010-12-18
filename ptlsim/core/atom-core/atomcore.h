
#ifndef MARSS_ATOM_CORE_H
#define MARSS_ATOM_CORE_H

#include <basecore.h>
#include <branchpred.h>
#include <statelist.h>

/* Logging Macros */
// Base Logging Level
#define ATOM_BASE_LL 5

#define ALWAYSLOG(...) ptl_logfile << __VA_ARGS__ , endl;

#define ATOMLOG1(...) if(logable(ATOM_BASE_LL)) { ptl_logfile << __VA_ARGS__ ; }
#define ATOMLOG2(...) if(logable(ATOM_BASE_LL+1)) { ptl_logfile << __VA_ARGS__ ; }
#define ATOMLOG3(...) if(logable(ATOM_BASE_LL+4)) { ptl_logfile << __VA_ARGS__ ; }

#define ATOMCORELOG(...) ATOMLOG1("Core:", coreid, " ", __VA_ARGS__, endl)
#define ATOMTHLOG1(...) ATOMLOG1("Core:", core.coreid, \
        " Th:", threadid, " ", __VA_ARGS__, endl)
#define ATOMTHLOG2(...) ATOMLOG2("Core:", core.coreid, \
        " Th:", threadid, " ", __VA_ARGS__, endl)
#define ATOMTHLOG3(...) ATOMLOG3("Core:", core.coreid, \
        " Th:", threadid, " ", __VA_ARGS__, endl)

#define ATOMOPLOG1(...) ATOMLOG1("Core:", thread->core.coreid, \
        " Th:", thread->threadid, " AtomOp:0x", hexstring(rip,48), \
        " [", uuid, "] ", __VA_ARGS__, endl)
#define ATOMOPLOG2(...) ATOMLOG2("Core:", thread->core.coreid, \
        " Th:", thread->threadid, " AtomOp:0x", hexstring(rip,48), \
        " [", uuid, "] ", __VA_ARGS__, endl)
#define ATOMOPLOG3(...) ATOMLOG3("Core:", thread->core.coreid, \
        " Th:", thread->threadid, " AtomOp:0x", hexstring(rip,48), \
        " [", uuid, "] ", __VA_ARGS__, endl)

#define ATOMCERR(...) ; //cout << __VA_ARGS__, endl;

#define HEXADDR(addr) hexstring(addr,48)
#define HEXDATA(data) hexstring(data,64)

namespace AtomCoreModel {

    using namespace superstl;
    using namespace Core;

    /* Constants */
    const W8 FU_COUNT = 6;

    const W8 NUM_ATOM_OPS_PER_THREAD = 32;

    const W8 MAX_UOPS_PER_ATOMOP = 4;
    const W8 MAX_REG_ACCESS_PER_ATOMOP = 16;

    const int NUM_FRONTEND_STAGES = 6;

    const int DISPATCH_QUEUE_SIZE = 16;

    const int DTLB_SIZE = 32;
    const int ITLB_SIZE = 32;

    const W8 MAX_FETCH_WIDTH = 2;

    const W8 ICACHE_FETCH_GRANULARITY = 16;

    const W8 MAX_ISSUE_PER_CYCLE = 1;

    const W8 MIN_PIPELINE_CYCLES = 7;

    const W8 STORE_BUF_SIZE = 16;

    const W8 MAX_BRANCH_IN_FLIGHT = 1;

    const int FORWARD_BUF_SIZE = 8;
    
    const W8 MAX_FORWARDING_LATENCY = 1;

    const W8 COMMIT_BUF_SIZE = 16;

    const W8 THREAD_PAUSE_CYCLES = 20;

    enum {
        FU_ALU0     = (1 << 0),
        FU_ALU1     = (1 << 1),
        FU_FPU0     = (1 << 2),
        FU_FPU1     = (1 << 3),
        FU_AGU0     = (1 << 4),
        FU_AGU1     = (1 << 5),
    };

    static const char* fu_names[FU_COUNT] = {
        "alu0",
        "alu1",
        "fp0",
        "fp1",
        "agu0",
        "agu1",
    };

    enum {
        PORT_0 = (1 << 0),
        PORT_1 = (1 << 1),
    };

    enum {
        ISSUE_OK = 0,       // Issue completed
        ISSUE_OK_BLOCK,     // Issue completed, dont' allow second issue
        ISSUE_FAIL,         // Issue failed
        ISSUE_CACHE_MISS,   // Issue had cache miss
        ISSUE_OK_SKIP,      // Issue complted but skip any other
                            // issue in this cycle
        NUM_ISSUE_RETULTS
    };

    static const char* issue_res_names[NUM_ISSUE_RETULTS] = {
        "ok", "block", "fail", "cache-miss", "skip",
    };

    enum {
        COMMIT_OK = 0,      // Commit without any issue
        COMMIT_BARRIER,     // Commit reached a barrier instruction
        COMMIT_INTERRUPT,   // Commit done, but handle pending interrupt
        COMMIT_SMC,         // Commit detected Self Modifying Code
        NUM_COMMIT_RESULTS
    };

    static const char* commit_res_names[NUM_COMMIT_RESULTS] = {
        "ok", "barrier", "interrupt", "smc",
    };

    //
    // Opcodes and properties
    //
#define ALU0 FU_ALU0
#define ALU1 FU_ALU1
#define AGU0 FU_AGU0
#define AGU1 FU_AGU1
#define FPU0 FU_FPU0
#define FPU1 FU_FPU1
#define A 1 // ALU latency, assuming fast bypass
#define L 1 

#define ANYALU ALU0|ALU1
#define ANYLDU AGU0|AGU1
#define ANYSTU AGU0|AGU1
#define ANYFPU FPU0|FPU1
#define ANYINT ANYALU
#define ANYFU  ANYALU | ANYLDU | ANYFPU

#define P0 PORT_0
#define P1 PORT_1
#define AP PORT_0|PORT_1

    struct FunctionalUnitInfo {
        byte opcode;   // Must match definition in ptlhwdef.h and ptlhwdef.cpp!
        byte latency;  // Latency in cycles, assuming ideal bypass
        byte port;     // Which port it can use
        bool pipelined;// If it uses pipelined FU or not
        W16  fu;       // Map of functional units on which this uop can issue
    };

    //
    // WARNING: This table MUST be kept in sync with the table
    // in ptlhwdef.cpp and the uop enum in ptlhwdef.h!
    //
    const FunctionalUnitInfo fuinfo[OP_MAX_OPCODE] = {
        // name, latency, fumask
        {OP_nop,            A, AP, 1, ANYFU },
        {OP_mov,            A, AP, 1, ANYFU },
        // Logical
        {OP_and,            A, AP, 1, ANYFU },
        {OP_andnot,         A, AP, 1, ANYFU },
        {OP_xor,            A, AP, 1, ANYFU },
        {OP_or,             A, AP, 1, ANYFU },
        {OP_nand,           A, AP, 1, ANYFU },
        {OP_ornot,          A, AP, 1, ANYFU },
        {OP_eqv,            A, AP, 1, ANYFU },
        {OP_nor,            A, AP, 1, ANYFU },
        // Mask, insert or extract bytes
        {OP_maskb,          A, AP, 1, ANYFU },
        // Add and subtract
        {OP_add,            A, AP, 1, ANYFU },
        {OP_sub,            A, AP, 1, ANYFU },
        {OP_adda,           A, AP, 1, ANYFU },
        {OP_suba,           A, AP, 1, ANYFU },
        {OP_addm,           A, AP, 1, ANYFU },
        {OP_subm,           A, AP, 1, ANYFU },
        // Condition code logical ops
        {OP_andcc,          A, AP, 1, ANYINT},
        {OP_orcc,           A, AP, 1, ANYINT},
        {OP_xorcc,          A, AP, 1, ANYINT},
        {OP_ornotcc,        A, AP, 1, ANYINT},
        // Condition code movement and merging
        {OP_movccr,         A, AP, 1, ANYINT},
        {OP_movrcc,         A, AP, 1, ANYINT},
        {OP_collcc,         A, AP, 1, ANYINT},
        // Simple shifting (restricted to small immediate 1..8)
        {OP_shls,           A, P0, 1, ANYINT},
        {OP_shrs,           A, P0, 1, ANYINT},
        {OP_bswap,          A, P0, 1, ANYINT},
        {OP_sars,           A, P0, 1, ANYINT},
        // Bit testing
        {OP_bt,             A, AP, 1, ANYALU},
        {OP_bts,            A, AP, 1, ANYALU},
        {OP_btr,            A, AP, 1, ANYALU},
        {OP_btc,            A, AP, 1, ANYALU},
        // Set and select
        {OP_set,            A, AP, 1, ANYINT},
        {OP_set_sub,        A, AP, 1, ANYINT},
        {OP_set_and,        A, AP, 1, ANYINT},
        {OP_sel,            A, AP, 1, ANYINT},
        {OP_sel_cmp,        A, AP, 1, ANYINT},
        // Branches
        {OP_br,             A, P1, 1, ANYINT},
        {OP_br_sub,         A, P1, 1, ANYINT},
        {OP_br_and,         A, P1, 1, ANYINT},
        {OP_jmp,            A, P1, 1, ANYINT},
        {OP_bru,            A, P1, 1, ANYINT},
        {OP_jmpp,           A, P1, 1, ANYINT},
        {OP_brp,            A, P1, 1, ANYINT},
        // Checks
        {OP_chk,            A, P1, 1, ANYINT},
        {OP_chk_sub,        A, P1, 1, ANYINT},
        {OP_chk_and,        A, P1, 1, ANYINT},
        // Loads and stores
        {OP_ld,             L, P0, 1, ANYLDU},
        {OP_ldx,            L, P0, 1, ANYLDU},
        {OP_ld_pre,         1, P0, 1, ANYLDU},
        {OP_st,             1, P0, 1, ANYSTU},
        {OP_mf,             1, P0, 1, ANYSTU},
        // Shifts, rotates and complex masking
        {OP_shl,            A, P0, 1, ANYALU},
        {OP_shr,            A, P0, 1, ANYALU},
        {OP_mask,           A, P0, 1, ANYALU},
        {OP_sar,            A, P0, 1, ANYALU},
        {OP_rotl,           A, P0, 1, ANYALU},
        {OP_rotr,           A, P0, 1, ANYALU},
        {OP_rotcl,          A, P0, 1, ANYALU},
        {OP_rotcr,          A, P0, 1, ANYALU},
        // Multiplication
        {OP_mull,           4, P0, 1, ANYFPU},
        {OP_mulh,           4, P0, 1, ANYFPU},
        {OP_mulhu,          4, P0, 1, ANYFPU},
        {OP_mulhl,          4, P0, 1, ANYFPU},
        // Bit scans
        {OP_ctz,            3, AP, 1, ANYFPU},
        {OP_clz,            3, AP, 1, ANYFPU},
        {OP_ctpop,          3, AP, 1, ANYFPU},
        {OP_permb,          4, AP, 1, ANYFPU},
        // Integer divide and remainder step
        {OP_div,           32, P0, 0, ANYFPU},
        {OP_rem,           32, P0, 0, ANYFPU},
        {OP_divs,          32, P0, 0, ANYFPU},
        {OP_rems,           1, P0, 0, ANYFPU},
        // Minimum and maximum
        {OP_min,            A, AP, 1, ANYALU},
        {OP_max,            A, AP, 1, ANYALU},
        {OP_min_s,          A, AP, 1, ANYALU},
        {OP_max_s,          A, AP, 1, ANYALU},
        // Floating point
        // uop.size bits have following meaning:
        // 00 = single precision, scalar (preserve high 32 bits of ra)
        // 01 = single precision, packed (two 32-bit floats)
        // 1x = double precision, scalar or packed (use two uops to process 128-bit xmm)
        {OP_fadd,           6, P1, 1, ANYFPU},
        {OP_fsub,           6, P1, 1, ANYFPU},
        {OP_fmul,           6, P1, 1, ANYFPU},
        {OP_fmadd,          6, P1, 1, ANYFPU},
        {OP_fmsub,          6, P1, 1, ANYFPU},
        {OP_fmsubr,         6, P1, 1, ANYFPU},
        {OP_fdiv,           6, P1, 0, ANYFPU},
        {OP_fsqrt,          6, P1, 0, ANYFPU},
        {OP_frcp,           6, P1, 1, ANYFPU},
        {OP_frsqrt,         6, P1, 0, ANYFPU},
        {OP_fmin,           6, P1, 1, ANYFPU},
        {OP_fmax,           6, P1, 1, ANYFPU},
        {OP_fcmp,           6, P1, 1, ANYFPU},
        // For fcmpcc, uop.size bits have following meaning:
        // 00 = single precision ordered compare
        // 01 = single precision unordered compare
        // 10 = double precision ordered compare
        // 11 = double precision unordered compare
        {OP_fcmpcc,         4, P1, 0, ANYFPU},
        // and/andn/or/xor are done using integer uops
        // For these conversions, uop.size bits select truncation mode:
        // x0 = normal IEEE-style rounding
        // x1 = truncate to zero
        {OP_fcvt_i2s_ins,   6, P1, 1, ANYFPU},
        {OP_fcvt_i2s_p,     6, P1, 1, ANYFPU},
        {OP_fcvt_i2d_lo,    6, P1, 1, ANYFPU},
        {OP_fcvt_i2d_hi,    6, P1, 1, ANYFPU},
        {OP_fcvt_q2s_ins,   6, P1, 1, ANYFPU},
        {OP_fcvt_q2d,       6, P1, 1, ANYFPU},
        {OP_fcvt_s2i,       6, P1, 1, ANYFPU},
        {OP_fcvt_s2q,       6, P1, 1, ANYFPU},
        {OP_fcvt_s2i_p,     6, P1, 1, ANYFPU},
        {OP_fcvt_d2i,       6, P1, 1, ANYFPU},
        {OP_fcvt_d2q,       6, P1, 1, ANYFPU},
        {OP_fcvt_d2i_p,     6, P1, 1, ANYFPU},
        {OP_fcvt_d2s_ins,   6, P1, 1, ANYFPU},
        {OP_fcvt_d2s_p,     6, P1, 1, ANYFPU},
        {OP_fcvt_s2d_lo,    6, P1, 1, ANYFPU},
        {OP_fcvt_s2d_hi,    6, P1, 1, ANYFPU},
        // Vector integer uops
        // uop.size defines element size: 00 = byte, 01 = W16, 10 = W32, 11 = W64 (i.e. same as normal ALU uops)
        {OP_vadd,           1, P1, 1, ANYFPU},
        {OP_vsub,           1, P1, 1, ANYFPU},
        {OP_vadd_us,        1, P1, 1, ANYFPU},
        {OP_vsub_us,        1, P1, 1, ANYFPU},
        {OP_vadd_ss,        1, P1, 1, ANYFPU},
        {OP_vsub_ss,        1, P1, 1, ANYFPU},
        {OP_vshl,           1, P1, 1, ANYFPU},
        {OP_vshr,           1, P1, 1, ANYFPU},
        {OP_vbt,            1, P1, 1, ANYFPU},
        {OP_vsar,           1, P1, 1, ANYFPU},
        {OP_vavg,           1, P1, 1, ANYFPU},
        {OP_vcmp,           1, P1, 1, ANYFPU},
        {OP_vmin,           1, P1, 1, ANYFPU},
        {OP_vmax,           1, P1, 1, ANYFPU},
        {OP_vmin_s,         1, P1, 1, ANYFPU},
        {OP_vmax_s,         1, P1, 1, ANYFPU},
        {OP_vmull,          4, P1, 0, ANYFPU},
        {OP_vmulh,          4, P1, 0, ANYFPU},
        {OP_vmulhu,         4, P1, 0, ANYFPU},
        {OP_vmaddp,         4, P1, 1, ANYFPU},
        {OP_vsad,           4, P1, 1, ANYFPU},
        {OP_vpack_us,       2, P1, 1, ANYFPU},
        {OP_vpack_ss,       2, P1, 1, ANYFPU},
        // Special Opcodes
        {OP_ast,			4, P0, 0, ANYINT},
    };

#undef A
#undef L
#undef F

#undef ALU0
#undef ALU1
#undef FPU0
#undef FPU1
#undef L

#undef ANYALU
#undef ANYLDU
#undef ANYSTU
#undef ANYFPU
#undef ANYINT

#undef P0
#undef P1
#undef AP

    struct BufferEntry;
    struct AtomThread;
    struct AtomCore;

    //
    // TLB class with one-hot semantics. 36 bit tags are required since
    // virtual addresses are 48 bits, so 48 - 12 (2^12 bytes per page)
    // is 36 bits.
    //
    template <int tlbid, int size>
    struct TranslationLookasideBuffer: 
        public FullyAssociativeTagsNbitOneHot<size, 40> {

        typedef FullyAssociativeTagsNbitOneHot<size, 40> base_t;
        TranslationLookasideBuffer(): base_t() { }

        void reset() {
            base_t::reset();
        }

        // Get the 40-bit TLB tag (36 bit virtual page ID plus 4 bit threadid)
        static W64 tagof(W64 addr, W64 threadid) {
            return bits(addr, 12, 36) | (threadid << 36);
        }

        bool probe(W64 addr, W8 threadid = 0) {
            W64 tag = tagof(addr, threadid);
            return (base_t::probe(tag) >= 0);
        }

        bool insert(W64 addr, W8 threadid = 0) {
            addr = floor(addr, PAGE_SIZE);
            W64 tag = tagof(addr, threadid);
            W64 oldtag;
            int way = base_t::select(tag, oldtag);
            W64 oldaddr = lowbits(oldtag, 36) << 12;
            if (logable(6)) {
                ptl_logfile << "TLB insertion of virt page ",
                            (void*)(Waddr)addr, " (virt addr ",
                            (void*)(Waddr)(addr), ") into way ", way, ": ",
                            ((oldtag != tag) ? "evicted old entry" :
                             "already present"), endl;
            }
            return (oldtag != InvalidTag<W64>::INVALID);
        }

        int flush_all() {
            reset();
            return size;
        }

        int flush_thread(W64 threadid) {
            W64 tag = threadid << 36;
            W64 tagmask = 0xfULL << 36;
            bitvec<size> slotmask = base_t::masked_match(tag, tagmask);
            int n = slotmask.popcount();
            base_t::masked_invalidate(slotmask);
            return n;
        }

        int flush_virt(Waddr virtaddr, W64 threadid) {
            return invalidate(tagof(virtaddr, threadid));
        }
    };

    template <int tlbid, int size>
    static inline ostream& operator <<(ostream& os,
            const TranslationLookasideBuffer<tlbid, size>& tlb)
    {
        return tlb.print(os);
    }

    typedef TranslationLookasideBuffer<0, DTLB_SIZE> DTLB;
    typedef TranslationLookasideBuffer<1, ITLB_SIZE> ITLB;

    struct BranchPredictorUpdateInfo: public PredictorUpdate {
        int stack_recover_idx;
        int bptype;
        W64 ripafter;
    };

    struct StoreBufferEntry;
    
    /**
     * @brief Represents one Atom Instruction in pipeline.
     *
     * In Intel Atom architecture, most of the x86 instructions are not
     * break-down into uOps, they execute one x86 instruction at a time. In
     * PTLsim based model we always breakdown x86 instruction into uOps. This
     * AtomOp try to mimic execution of one x86 instruction at a time by
     * grouping all the uOps of that instruction in one AtomOp and execute them
     * togather in the simulator.
     */
    struct AtomOp : public selfqueuelink {

        // Functions
        AtomOp();

        void reset();

        void change_state(StateList& new_state, bool place_at_head = false);

        void commit_flags(int idx);

        // Fetch Functions
        bool fetch();
        void setup_registers();

        // Issue Functions
        W8 issue(bool first_issue);
        bool can_issue();
        bool all_src_ready();
        W64 read_reg(W16 reg, W8 uop_idx);
        W8 execute_uop(W8 idx);
        W8 execute_ast(TransOp& uop);
        W8 execute_fence(TransOp& uop);
        bool check_execute_exception(int idx);
        W8 execute_load(TransOp& uop);
        W8 execute_store(TransOp& uop, W8 idx);
        W64 get_load_data(W64 addr, TransOp& uop);
        W64 generate_address(TransOp& uop, bool is_st);
        W64 get_virt_address(TransOp& uop, bool is_st);
        W64 get_phys_address(TransOp& uop, bool is_st, W64 virtaddr);
        void dtlb_walk_completed();

        // Forward
        void forward();
        
        // Writeback/Commit
        int writeback();

        // Variables
        AtomThread *thread;

        StateList   *current_state_list;
        BufferEntry *buf_entry;

        W8 is_branch:1, is_ldst:1, is_fp:1, is_sse:1, is_nonpipe:1,
           is_barrier:1, is_ast: 1, pad:1;

        W64 rip;

        W64  page_fault_addr;
        bool had_exception;
        W32  error_code;
        W32  exception;

        W8   execution_cycles;
        W64  radata;
        W64  rbdata;
        W64  rcdata;

        IssueState state;
        Waddr      cache_virtaddr;

        W8   port_mask;
        bool som, eom;
        W32  fu_mask;

        uopimpl_func_t synthops[MAX_UOPS_PER_ATOMOP];
        TransOp        uops[MAX_UOPS_PER_ATOMOP];
        W16            rflags[MAX_UOPS_PER_ATOMOP];
        W8             num_uops_used;
        W64            uuid;

        W8  src_registers[MAX_REG_ACCESS_PER_ATOMOP];
        W8  dest_registers[MAX_UOPS_PER_ATOMOP];
        W64 dest_register_values[MAX_UOPS_PER_ATOMOP];

        StoreBufferEntry* stores[MAX_UOPS_PER_ATOMOP];

        BranchPredictorUpdateInfo predinfo;

        W8 cycles_left;
    };

    /**
     * @brief Entry for Store Buffer
     */
    struct StoreBufferEntry {

        StoreBufferEntry()
            : op(NULL), index_(0), addr(0), data(0), bytemask(0)
              , virtaddr(0), size(0), mmio(0)
        { }

        int init(int index)
        {
            index_ = index;
            addr = 0;
            data = 0xdeadbeefdeadbeef;
            bytemask = 0;
            virtaddr = 0;
            size = 0;
            mmio = 0;
            return 0;
        }

        int index()
        {
            return index_;
        }

        void validate()
        { }

        void write_to_ram(Context& ctx)
        {
            ctx.storemask_virt(virtaddr, data, bytemask, size);
        }

        AtomOp* op;
        W16s    index_;
        W64     addr;
        W64     data;
        W8      bytemask;
        W64     virtaddr;
        W8      size;
        bool    mmio;
    };

    /**
     * @brief Entry of Fetch-Queue/Dispatch-Queue
     */
    struct BufferEntry {

        BufferEntry()
            : op(NULL), index_(0)
        { }

        int init(int index)
        {
            index_ = index;
            return 0;
        }

        int index()
        { return index_; }

        void validate() { }

        AtomOp* op;
        W16s    index_;
    };

    struct ForwardEntry {

        ForwardEntry()
            : data(0xdeadbeefdeadbeef)
        { }

        void reset()
        { data = 0xdeadbeefdeadbeef; }

        W64 data;
    };

    /**
     * @brief Hardware-Thread of AtomCore
     *
     * This class represents single hardware thread of the core.
     * In multi-threaded Atom core, Only one thread can run. Thread switching
     * is done when thread halts in a L2-cache miss.
     */
    struct AtomThread {
        AtomThread(AtomCore& core, W8 threadid, Context& ctx);

        void reset();

        bool fetch();
        bool fetch_probe_itlb();
        bool fetch_from_icache();
        bool fetch_into_atomop();
        bool fetch_check_current_bb();

        void frontend();

        bool dispatch(AtomOp *op);

        bool issue();
        void redirect_fetch(W64 rip);

        void complete();

        void forward();

        void transfer();

        bool writeback();
        bool commit_queue();
        bool handle_exception();
        bool handle_interrupt();
        bool handle_barrier();
        void add_to_commitbuf(AtomOp* op);

        void itlb_walk();
        void dtlb_walk();

        bool access_dcache(Waddr addr, W64 rip, W8 type, W64 uuid);

        bool dcache_wakeup(void *arg);
        bool icache_wakeup(void *arg);

        StoreBufferEntry* get_storebuf_entry();

        void flush_pipeline();

        bool ready_to_switch();

        void write_temp_reg(W16 reg, W64 data);
        W64  read_reg(W16 reg);

        W8      threadid;
        W64     fetch_uuid;
        bool    register_invalid[TRANSREG_COUNT];
        AtomOp* register_owner[TRANSREG_COUNT];
        W16     register_flags[TRANSREG_COUNT];
        W64     temp_registers[11];

        AtomCore& core;
        Context&  ctx;

        BasicBlock* current_bb;
        RIPVirtPhys fetchrip;
        W8          bb_transop_index;

        bool  waiting_for_icache_miss;
        W64   current_icache_block;
        Waddr icache_miss_addr;
        bool  itlb_exception;
        W64   itlb_exception_addr;
        bool  stall_frontend;
        W8    itlb_walk_level;

        W8      dtlb_walk_level;
        W64     dtlb_miss_addr;
        AtomOp* dtlb_miss_op;
        W16     forwarded_flags;
        W16     internal_flags;
        int     pause_counter;
        bool    mmio_pending;
        bool    inst_in_pipe;

        BranchPredictorInterface branchpred;

        /**
         * @brief Track no of un-resolved branch instructions in pipeline
         */
        W8 branches_in_flight;
        
        RIPVirtPhys dispatchrip;
        Queue<BufferEntry, DISPATCH_QUEUE_SIZE+1> dispatchq;

        Queue<StoreBufferEntry, STORE_BUF_SIZE+1> storebuf;

        /**
         * @brief Simulated Queue to store AtomOp for Commit
         *
         * This queue is used to store multiple AtomOp consisting of one x86
         * instruction. Some special/complex x86 instructions consume multiple
         * AtomOps and for correct architecture state we commit them togather.
         */
        Queue<BufferEntry, COMMIT_BUF_SIZE> commitbuf;
        bool                                eom_found;

        /**
         * @brief Special instructions can disable issue for sanity
         *
         * Special instructions or instructions with longer dealy can disable
         * pipeline issue untill they are completed. These instructions will
         * enable this flag when they are issued.
         * Also AtomOps that cause exceptions disable Issue untill the
         * exception is handled.
         */
        bool issue_disabled;

        bool    handle_interrupt_at_next_eom;
        AtomOp* exception_op;
        bool    running;
        bool    ready;


        AtomOp atomOps[NUM_ATOM_OPS_PER_THREAD];

        /**
         * @brief list of AtomOp states
         *
         * Virtual list of AtomOp stats that is used within pipeline for
         * keeping track of all AtomOp
         */
        ListOfStateLists op_lists;

        StateList op_free_list;
        StateList op_fetch_list;
        StateList op_dispatched_list;
        StateList op_executing_list;
        StateList op_cache_miss_list;
        StateList op_tlb_miss_list;
        StateList op_forwarding_list;
        StateList op_waiting_to_writeback_list;
        StateList op_ready_to_writeback_list;

        /*
         * Cache Access
         */
        Signal dcache_signal;
        Signal icache_signal;
    };

    /**
     * @brief In-Order Core modeled based on Intel Atom
     *
     * This AtomCore model simulate an in-order pipeline with support for
     * multi-threading. In some cases it tries to mimic Intel Atom
     * architecture.
     */
    struct AtomCore : public BaseCore {

        AtomCore(BaseCoreMachine& machine, int num_threads);
        
        void reset();
        bool runcycle();
        void check_ctx_changes();
        void flush_tlb(Context& ctx);
        void flush_tlb_virt(Context& ctx, Waddr virtaddr);
        void dump_state(ostream& os);
        void update_stats(PTLsimStats* stats);
        void flush_pipeline();
        W8   get_coreid();

        // Pipeline related functions
        void fetch();

        void frontend();

        void issue();

        void complete();

        void forward();
        void set_forward(W8 reg, W64 data);
        void clear_forward(W8 reg);

        void transfer();

        bool writeback();

        void flush_shared_structs(W8 threadid);

        void try_thread_switch();

        W8   coreid;
        W8   threadcount;
        bool in_thread_switch;

        AtomThread** threads;
        AtomThread*  running_thread;

        /**
         * @brief Fetch/Decode Queue
         *
         * This queue simulate the frontend pipeline which handles 'fetch' and
         * 'decode' stages. This queue is shared between threads. When one
         * thread is stopped, this queue is flushed to start fetch/decode of
         * other thread.
         *
         * Note: Here 'Queue' keeps 1 entry for tracking 'head' and 'tail' of
         * the ring buffer. So We add +1 to our fetchq.
         */
        Queue<BufferEntry, NUM_FRONTEND_STAGES+1> fetchq;

        /**
         * @brief Fully Associative Array to store Forwarding Data
         *
         * This forward buffer is shared between threads and on thread switch
         * its cleared.
         */
        FullyAssociativeArray<W16, ForwardEntry, FORWARD_BUF_SIZE> forwardbuf;

        DTLB dtlb;
        ITLB itlb;

        // fu_available is used across cycles for non-pipeliend instructions
        // fu_used is used within cycle to make sure that we dont issue
        // multiple instructions to same FU in one cycle
        W64 fu_available:32, fu_used:32;
        W8  port_available;
    };

    /* Checker - saved stores to compare after executing emulated instruction */
    struct CheckStores {
      W64 virtaddr;
      W64 data;
      W8 sizeshift;
      W8 bytemask;
    };

    extern CheckStores *checker_stores;
    extern int checker_stores_count;

    void reset_checker_stores();

    void add_checker_store(StoreBufferEntry* buf, W8 sizeshift);
    
}; // namespace AtomCoreModel

#endif // MARSS_ATOM_CORE_H