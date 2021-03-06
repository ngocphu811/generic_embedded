#include "arch.h"
#include "system.h"

static volatile u32_t g_crit_entry = 0;

void enter_critical(void) {
  if ((__get_CONTROL() & 3) == 3) {
    // TODO PETER
    //print("enter critical from user!!\n");
  }
#ifndef CONFIG_ARCH_CRITICAL_DISABLE_IRQ
  __disable_irq();
#else
  CONFIG_ARCH_CRITICAL_DISABLE_IRQ;
#endif
  g_crit_entry++;
  TRACE_IRQ_OFF(g_crit_entry);
}

void exit_critical(void) {
  ASSERT(g_crit_entry > 0);
  g_crit_entry--;
  TRACE_IRQ_ON(g_crit_entry);
  if (g_crit_entry == 0) {
#ifndef CONFIG_ARCH_CRITICAL_ENABLE_IRQ
    __enable_irq();
#else
  CONFIG_ARCH_CRITICAL_ENABLE_IRQ;
#endif
  }
}

bool within_critical(void) {
  return g_crit_entry > 0;
}

void arch_reset(void) {
  NVIC_SystemReset();
}

void arch_break_if_dbg(void) {
  if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) {
    // if debug is enabled, fire breakpoint
    __asm__ volatile ("bkpt #0\n");
  }
}

void arch_sleep(void) {
  __WFI();
}

void arch_busywait_us(u32_t us) {
  if (us == 0) return;
  register volatile system_counter_type *count_val =
      (volatile system_counter_type *)&(STM32_SYSTEM_TIMER->CNT);
  register system_counter_type mark = *count_val;
  register u32_t delta_ticks = us*(SYS_CPU_FREQ/1000000);
  {
    register u32_t timer_tick_cycle = SYS_CPU_FREQ/SYS_MAIN_TIMER_FREQ;
    register u32_t abs_hit = mark+delta_ticks;

    bool precede = *count_val > mark;
    if (precede) {
      while (abs_hit >= timer_tick_cycle) {
        // wait for wrap
        while (*count_val >= mark);
        // wait for hit
        while (*count_val < mark);
        delta_ticks -= timer_tick_cycle;
        abs_hit -= timer_tick_cycle;
      }
    } else {
      while (abs_hit >= timer_tick_cycle) {
        // wait for wrap
        while (*count_val < mark);
        // wait for hit
        while (*count_val >= mark);
        delta_ticks -= timer_tick_cycle;
        abs_hit -= timer_tick_cycle;
      }
    }
  }
  {
    register system_counter_type hit = mark+delta_ticks;
    if (hit < mark) {
      while (*count_val > hit);
    }
    while (*count_val < hit);
  }
}

void irq_disable(void) {
  __disable_irq();
}

void irq_enable(void) {
  __enable_irq();
}


