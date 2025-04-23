#ifndef __CPU_SAPPHIRERAPIDS_H
#define __CPU_SAPPHIRERAPIDS_H

#include <math.h>
#include "thread.h"
#include "cpu/pmc.h"
#include "debug.h"

// NOTE: These event names and encodings are placeholders for Sapphire Rapids.
//       Replace them with actual values from Intel documentation.
#undef FOREACH_PMC_HW_EVENT
#define FOREACH_PMC_HW_EVENT(ACTION)                                                                       \
  ACTION("CYCLE_ACTIVITY:STALLS_L2_MISS", NULL, 0x55305a3) /* Placeholder */                       \
  ACTION("MEM_LOAD_L3_HIT_RETIRED:XSNP_NONE", NULL, 0x5308d2)          /* Placeholder */                       \
  ACTION("MEM_LOAD_L3_MISS_RETIRED:REMOTE_DRAM", NULL, 0x5302d3)     /* Placeholder */                       \
  ACTION("MEM_LOAD_L3_MISS_RETIRED:LOCAL_DRAM", NULL, 0x5301d3)      /* Placeholder */

#undef FOREACH_PMC_EVENT
#define FOREACH_PMC_EVENT(ACTION, prefix)                                                                  \
  ACTION(ldm_stall_cycles, prefix)                                                                         \
  ACTION(remote_dram, prefix)

// NOTE: This factor might need adjustment for Sapphire Rapids.
#define SPR_L3_FACTOR 7.0

extern __thread int tls_hw_local_latency;
extern __thread int tls_hw_remote_latency;
#ifdef MEMLAT_SUPPORT
extern __thread uint64_t tls_global_remote_dram;
extern __thread uint64_t tls_global_local_dram;
#endif

DECLARE_ENABLE_PMC(sapphirerapids, ldm_stall_cycles)
{
    // NOTE: Update event names to actual Sapphire Rapids events
    ASSIGN_PMC_HW_EVENT_TO_ME("CYCLE_ACTIVITY:STALLS_L2_MISS", 0);
    ASSIGN_PMC_HW_EVENT_TO_ME("MEM_LOAD_L3_HIT_RETIRED:XSNP_NONE", 1);
    ASSIGN_PMC_HW_EVENT_TO_ME("MEM_LOAD_L3_MISS_RETIRED:REMOTE_DRAM", 2);
    ASSIGN_PMC_HW_EVENT_TO_ME("MEM_LOAD_L3_MISS_RETIRED:LOCAL_DRAM", 3);

    return E_SUCCESS;
}

DECLARE_CLEAR_PMC(sapphirerapids, ldm_stall_cycles)
{
}

DECLARE_READ_PMC(sapphirerapids, ldm_stall_cycles)
{
   // NOTE: Update event indices and calculation logic based on Sapphire Rapids documentation.
   uint64_t l2_pending_diff  = READ_MY_HW_EVENT_DIFF(0);
   uint64_t llc_hit_diff     = READ_MY_HW_EVENT_DIFF(1);
   uint64_t remote_dram_diff = READ_MY_HW_EVENT_DIFF(2);
   uint64_t local_dram_diff  = READ_MY_HW_EVENT_DIFF(3);

   DBG_LOG(DEBUG, "SPR read stall L2 cycles diff %lu; llc_hit %lu; cycles diff remote_dram %lu; local_dram %lu\n",
		   l2_pending_diff, llc_hit_diff, remote_dram_diff, local_dram_diff);

   if ((remote_dram_diff == 0) && (local_dram_diff == 0)) return 0;
#ifdef MEMLAT_SUPPORT
   tls_global_local_dram += local_dram_diff;
#endif

   // calculate stalls based on L2 stalls and LLC miss/hit (Placeholder logic)
   double num = SPR_L3_FACTOR * (remote_dram_diff + local_dram_diff);
   double den = num + llc_hit_diff;
   if (den == 0) return 0;
   return (uint64_t) ((double)l2_pending_diff * (num / den));
}


DECLARE_ENABLE_PMC(sapphirerapids, remote_dram)
{
    // NOTE: Update event names to actual Sapphire Rapids events
    ASSIGN_PMC_HW_EVENT_TO_ME("CYCLE_ACTIVITY:STALLS_L2_MISS", 0);
    ASSIGN_PMC_HW_EVENT_TO_ME("MEM_LOAD_L3_HIT_RETIRED:XSNP_NONE", 1);
    ASSIGN_PMC_HW_EVENT_TO_ME("MEM_LOAD_L3_MISS_RETIRED:REMOTE_DRAM", 2);
    ASSIGN_PMC_HW_EVENT_TO_ME("MEM_LOAD_L3_MISS_RETIRED:LOCAL_DRAM", 3);

    return E_SUCCESS;
}

DECLARE_CLEAR_PMC(sapphirerapids, remote_dram)
{
}

DECLARE_READ_PMC(sapphirerapids, remote_dram)
{
   // NOTE: Update event indices and calculation logic based on Sapphire Rapids documentation.
   uint64_t l2_pending_diff  = READ_MY_HW_EVENT_DIFF(0);
   uint64_t llc_hit_diff     = READ_MY_HW_EVENT_DIFF(1);
   uint64_t remote_dram_diff = READ_MY_HW_EVENT_DIFF(2);
   uint64_t local_dram_diff  = READ_MY_HW_EVENT_DIFF(3);

   DBG_LOG(DEBUG, "SPR read stall L2 cycles diff %lu; llc_hit %lu; cycles diff remote_dram %lu; local_dram %lu\n",
		   l2_pending_diff, llc_hit_diff, remote_dram_diff, local_dram_diff);

   if ((remote_dram_diff == 0) && (local_dram_diff == 0)) return 0;
#ifdef MEMLAT_SUPPORT
   tls_global_remote_dram += remote_dram_diff;
#endif

   // calculate stalls based on L2 stalls and LLC miss/hit (Placeholder logic)
   double num = SPR_L3_FACTOR * (remote_dram_diff + local_dram_diff);
   double den = num + llc_hit_diff;
   if (den == 0) return 0;
   double stalls = (double)l2_pending_diff * (num / den);

   // calculate remote dram stalls based on total stalls and local/remote dram accesses (Placeholder logic)
   den = (remote_dram_diff * tls_hw_remote_latency) + (local_dram_diff * tls_hw_local_latency);
   if (den == 0) return 0;
   return (uint64_t) (stalls * ((double)(remote_dram_diff * tls_hw_remote_latency) / den));
}


PMC_EVENTS(sapphirerapids, 4) // Assuming 4 counters, verify for SPR
#endif /* __CPU_SAPPHIRERAPIDS_H */
