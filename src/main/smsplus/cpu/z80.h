#pragma once

#include "cpuintrf.h"

enum {
  Z80_PC,
  Z80_SP,
  Z80_A,
  Z80_B,
  Z80_C,
  Z80_D,
  Z80_E,
  Z80_H,
  Z80_L,
  Z80_AF,
  Z80_BC,
  Z80_DE,
  Z80_HL,
  Z80_IX,
  Z80_IY,
  Z80_AF2,
  Z80_BC2,
  Z80_DE2,
  Z80_HL2,
  Z80_R,
  Z80_I,
  Z80_IM,
  Z80_IFF1,
  Z80_IFF2,
  Z80_HALT,
  Z80_DC0,
  Z80_DC1,
  Z80_DC2,
  Z80_DC3,
  Z80_WZ
};

enum {
  Z80_TABLE_op,
  Z80_TABLE_cb,
  Z80_TABLE_ed,
  Z80_TABLE_xy,
  Z80_TABLE_xycb,
  Z80_TABLE_ex /* cycles counts for taken jr/jp/call and interrupt latency (rst opcodes) */
};

/****************************************************************************/
/* The Z80 registers. HALT is set to 1 when the CPU is halted, the refresh  */
/* register is calculated as follows: refresh=(Z80.r&127)|(Z80.r2&128)      */
/****************************************************************************/
typedef struct {
  PAIR pc, sp, af, bc, de, hl, ix, iy, wz;
  PAIR af2, bc2, de2, hl2;

  uint8_t r;
  uint8_t r2;
  uint8_t iff1;
  uint8_t iff2;

  uint8_t halt;
  uint8_t im;
  uint8_t i;
  uint8_t nmi_state; /* nmi line state */

  uint8_t nmi_pending; /* nmi pending */
  uint8_t irq_state;   /* irq line state */
  uint8_t after_ei;    /* are we in the EI shadow? */
  uint8_t _pad00;

  const struct z80_irq_daisy_chain *daisy;
  int (*irq_callback)(int irqline);
} __attribute__((packed, aligned(1))) Z80_Regs;

extern int z80_cycle_count;
extern Z80_Regs Z80;

void z80_init(int index, int clock, const void *config, int (*irqcallback)(int));
void z80_reset(void);
void z80_exit(void);
int z80_execute(int cycles);
void z80_burn(int cycles);
void z80_get_context(void *dst);
void z80_set_context(void *src);
void z80_set_irq_line(int irqline, int state);
void z80_reset_cycle_count(void);
int z80_get_elapsed_cycles(void);

extern unsigned char *cpu_readmap[64];
extern unsigned char *cpu_writemap[64];

extern void (*cpu_writemem16)(int address, int data);
extern void (*cpu_writeport16)(uint16 port, uint8_t data);
extern uint8_t (*cpu_readport16)(uint16 port);
