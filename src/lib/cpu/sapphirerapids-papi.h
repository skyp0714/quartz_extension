#ifndef __CPU_SAPPHIRERAPIDS_PAPI_H
#define __CPU_SAPPHIRERAPIDS_PAPI_H

#include <papi.h>
#include <math.h>
#include "debug.h"
#include "cpu/pmc-papi.h" // Include the base PAPI header

// NOTE: These event names are placeholders for Sapphire Rapids.
//       Replace them with actual PAPI-compatible event names from Intel documentation
//       or PAPI documentation for Sapphire Rapids.
const char *sapphirerapids_native_events[MAX_NUM_EVENTS] = {
    "CYCLE_ACTIVITY:STALLS_L2_MISS", /* Placeholder */
    "MEM_LOAD_L3_HIT_RETIRED:XSNP_NONE",          /* Placeholder */
    "MEM_LOAD_L3_MISS_RETIRED:REMOTE_DRAM",     /* Placeholder */
    "MEM_LOAD_L3_MISS_RETIRED:LOCAL_DRAM"       /* Placeholder */
    // NULL // Ensure the list is NULL-terminated if fewer than MAX_NUM_EVENTS are used
};

// NOTE: Update calculation logic based on Sapphire Rapids documentation and chosen events.
uint64_t sapphirerapids_read_stall_events_local() {
    long long values[MAX_NUM_EVENTS];
    uint64_t events = 0;

    if (pmc_events_read_local_thread(values) == PAPI_OK) {
		uint64_t l2_pending = values[0]; // Placeholder index
		uint64_t llc_hit  = values[1];   // Placeholder index
		uint64_t remote_dram = values[2];// Placeholder index
		uint64_t local_dram  = values[3]; // Placeholder index

		DBG_LOG(DEBUG, "SPR read stall L2 cycles %lu; llc_hit %lu; remote_dram %lu; local_dram %lu\n",
			l2_pending, llc_hit, remote_dram, local_dram);

        // Placeholder calculation logic (similar to Haswell/Ivy Bridge)
		double num = remote_dram + local_dram;
		double den = num + llc_hit;
		if (den == 0) return 0;

		events = (uint64_t)((double)l2_pending * ((double)num / den));
    } else {
        DBG_LOG(ERROR, "SPR read stall cycles failed (local)\n");
    }

    return events;
}

// NOTE: Update calculation logic based on Sapphire Rapids documentation and chosen events.
uint64_t sapphirerapids_read_stall_events_remote() {
    long long values[MAX_NUM_EVENTS];
    uint64_t events = 0;

    if (pmc_events_read_local_thread(values) == PAPI_OK) {
		uint64_t l2_pending = values[0]; // Placeholder index
		uint64_t llc_hit  = values[1];   // Placeholder index
		uint64_t remote_dram = values[2];// Placeholder index
		uint64_t local_dram  = values[3]; // Placeholder index

		DBG_LOG(DEBUG, "SPR read stall L2 cycles %lu; llc_hit %lu; remote_dram %lu; local_dram %lu\n",
			l2_pending, llc_hit, remote_dram, local_dram);

		// Placeholder calculation logic (similar to Haswell/Ivy Bridge)
		double num = remote_dram + local_dram;
		double den = num + llc_hit;
		if (den == 0) return 0;
		double stalls = (double)l2_pending * ((double)num / den);

		// calculate remote dram stalls based on total stalls and local/remote dram accesses
		den = remote_dram + local_dram;
		if (den == 0) return 0;
		events = (uint64_t) (stalls * ((double)remote_dram / den));
    } else {
        DBG_LOG(ERROR, "SPR read stall cycles failed (remote)\n");
    }

    return events;
}

#endif /* __CPU_SAPPHIRERAPIDS_PAPI_H */
