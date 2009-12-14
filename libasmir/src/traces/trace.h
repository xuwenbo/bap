#ifndef _TRACE_H_
#define _TRACE_H_

#include "disasm.h"
#include <inttypes.h>

/* Size of buffer to store instructions */
#define FILEBUFSIZE 104857600

/* Trace header values */
#define VERSION_NUMBER 50
#define MAGIC_NUMBER 0xFFFFFFFF

/* Taint origins */
#define TAINT_SOURCE_NIC_IN 0
#define TAINT_SOURCE_KEYBOARD_IN 1
#define TAINT_SOURCE_FILE_IN 2
#define TAINT_SOURCE_NETWORK_OUT 3
#define TAINT_SOURCE_API_TIME_IN 4
#define TAINT_SOURCE_API_FILE_IN 5
#define TAINT_SOURCE_API_REGISTRY_IN 6
#define TAINT_SOURCE_API_HOSTNAME_IN 7
#define TAINT_SOURCE_API_FILE_INFO_IN 8
#define TAINT_SOURCE_API_SOCK_INFO_IN 9
#define TAINT_SOURCE_API_STR_IN 10
#define TAINT_SOURCE_API_SYS_IN 11
#define TAINT_SOURCE_HOOKAPI 12
#define TAINT_SOURCE_LOOP_IV 13
#define TAINT_SOURCE_MODULE 14

/* Starting origin for network connections */
#define TAINT_ORIGIN_START_TCP_NIC_IN 10000
#define TAINT_ORIGIN_START_UDP_NIC_IN 11000
#define TAINT_ORIGIN_MODULE           20000

/* Taint propagation definitions */
#define TP_NONE 0           // No taint propagation
#define TP_SRC 1            // Taint propagated from SRC to DST
#define TP_CJMP 2           // Cjmp using tainted EFLAG
#define TP_MEMREAD_INDEX 3  // Memory read with tainted index
#define TP_MEMWRITE_INDEX 4 // Memory write with tainted index
#define TP_REP_COUNTER 5    // Instruction with REP prefix and tainted counter
#define TP_SYSENTER 6       // Sysenter

/* Trace format definitions */
#define MAX_NUM_OPERANDS 30 // FNSAVE has a memory operand of 108 bytes 
#define MAX_NUM_MEMREGS 5  /* Max number of memregs per memory operand */
#define MAX_NUM_TAINTBYTE_RECORDS 3
#define MAX_STRING_LEN 32
#define MAX_OPERAND_LEN 8 /* Max length of an operand in bytes */
#define MAX_INSN_BYTES 15 /* Maximum number of bytes in a x86 instruction */

#define BLOCK(h) (reinterpret_cast<char*>(&(h)))
#define CHAR 1
#define INT16 2
#define INT32 4
#define INT64 8

enum OpUsage { unknown = 0, esp, counter, membase, memindex, memsegment,
  memsegent0, memsegent1 };

#define TAINT_BYTE_RECORD_FIXED_SIZE 12

typedef struct _taint_byte_record {
  uint32_t source;              // Tainted data source (network,keyboard...)
  uint32_t origin;              // Identifies a network flow
  uint32_t offset;              // Offset in tainted data buffer (network)
} TaintByteRecord;

#define TAINT_RECORD_FIXED_SIZE 4

// taint record is OK
typedef struct _taint_record {
  uint16_t taint_propag;
  uint16_t numRecords;          // How many TaintByteRecord currently used
  TaintByteRecord taintBytes[MAX_NUM_TAINTBYTE_RECORDS];
} taint_record_t;

#define OPERAND_VAL_FIXED_SIZE 28
#define OPERAND_VAL_ENUMS_REAL_SIZE 2

enum OpAccess {A_Unknown = 0, A_RW, A_R, A_W, A_RCW, A_CW, A_CRW, A_CR} ;


typedef struct _operand_val {
  enum OpType type;
  enum OpUsage usage;
  uint32_t length;
  uint32_t addr;
  uint32_t value;
  uint64_t tainted;
  // operand val OK up to here
  //uint8_t access; /* xed_operand_action_enum_t */
  taint_record_t records[MAX_OPERAND_LEN];
  enum OpAccess acc;
} OperandVal;

#define ENTRY_HEADER_FIXED_SIZE 44

/* Entry header description
  address:       Address where instruction is loaded in memory
  tid:           Thread identifier
  inst_size:     Number of bytes in x86 instruction
  num_operands:  Number of operands (includes all except ESP)
  tp:            Taint propagation value. See above.
  eflags:        Value of the EFLAGS register
  cc_op:         Determines operation performed by QEMU on CC_SRC,CC_DST.
                   ONLY REQUIRES 8-bit
  df:            Direction flag. Has to be -1 (x86_df=1) or 1 (x86_df = 0)
                    COULD BE DERIVED FROM eflags
  operand[]:     Operands accessed by instruction
  memregs[][idx]:   Operands used for indirect addressing
    idx == 0 -> Segment register
    idx == 1 -> Base register
    idx == 2 -> Index register
    idx == 3 -> Segent0
    idx == 4 -> Segent1
  rawybytes[]:   Rawbytes of the x86 instruction
*/
typedef struct _entry_header {
  uint32_t address;
  uint32_t tid;
  uint16_t inst_size;
  uint8_t num_operands;
  uint8_t tp;
  uint32_t eflags;
  uint32_t cc_op;
  uint32_t df;

  uint32_t hflags;
  uint32_t aldt;
  uint32_t agdt;
  uint32_t atr;
  uint32_t aidt;
  OperandVal oper;
// entry header OK up to here
  OperandVal operand[MAX_NUM_OPERANDS];
  OperandVal memregs[MAX_NUM_MEMREGS][MAX_NUM_MEMREGS]; // 1-dim MAX_NUM_OPERANDS -> MAX_NUM_MEMREGS
  unsigned char rawbytes[MAX_INSN_BYTES];
} EntryHeader;

#define PROC_RECORD_FIXED_SIZE 40
typedef struct _proc_record {
  char name[MAX_STRING_LEN];
  uint32_t pid;
  uint32_t n_mods;
  uint32_t ldt_base;
} ProcRecord;

#define MODULE_RECORD_FIXED_SIZE 40
typedef struct _module_record {
  char name[MAX_STRING_LEN];
  uint32_t base;
  uint32_t size;
} ModuleRecord;

#define TRACE_HEADER_FIXED_SIZE 12
// trace header OK
typedef struct _trace_header {
  uint32_t magicnumber;
  uint32_t version;
  uint32_t n_procs;
  uint32_t gdt_base;
  uint32_t idt_base;
} TraceHeader;

/* Exported variables */
extern int received_tainted_data;
extern int has_page_fault;
extern int access_user_mem;
extern int insn_already_written;
extern int regmapping[];
extern long insn_counter_traced; // Instruction counter in trace
extern char filebuf[FILEBUFSIZE];
extern int trace_do_not_write;
extern int header_already_written;

/* Exported Functions */
int get_regnum(OperandVal op);
int getOperandOffset (OperandVal *op);
void decode_address(uint32_t address, EntryHeader *eh, int ignore_taint);
unsigned int write_insn(FILE *stream, EntryHeader *eh);
void print_stats();
void xed2_init();

#endif // _TRACE_H_

