/***************************************************************************
Copyright 2016 Hewlett Packard Enterprise Development LP.  
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version. This program is distributed in the
hope that it will be useful, but WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details. You
should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include "cpu.h"
#include "dev.h"
#include "error.h"
#include "misc.h"
#include "known_cpus.h"
#include "xeon-ex.h"
#include <cpuid.h>

// Mainline architectures and processors available here:
// https://software.intel.com/en-us/articles/intel-architecture-and-processor-identification-with-cpuid-model-and-family-numbers
//
// It turns out that CPUID is not an accurate approach to identifying a
// processor as different processors may have the same CPUID.
// So instead we rely on the brand string returned by /proc/cpuinfo:model_name

#define MASK(msb, lsb) (~((~0) << (msb + 1)) & ((~0) << lsb))
#define EXTRACT(val, msb, lsb) ((MASK(msb, lsb) & val) >> lsb)
#define MODEL(eax) EXTRACT(eax, 7, 4)
#define EXTENDED_MODEL(eax) EXTRACT(eax, 19, 16)
#define MODEL_NUMBER(eax) ((EXTENDED_MODEL(eax) << 4) | MODEL(eax))
#define FAMILY(eax) EXTRACT(eax, 11, 8)
#define Extended_Family(eax) EXTRACT(eax, 27, 20)
#define Family_Number(eax) (FAMILY(eax) + Extended_Family(eax))

void cpuid(unsigned int info, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx)
{
    __asm__(
        "cpuid;"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(info));
}

void get_family_model(int *family, int *model)
{
    unsigned int eax, ebx, ecx, edx;
    int success = __get_cpuid(1, &eax, &ebx, &ecx, &edx);
    if (family != NULL)
    {
        *family = success ? Family_Number(eax) : 0;
    }

    if (model != NULL)
    {
        *model = success ? MODEL_NUMBER(eax) : 0;
    }
}

// caller is responsible for freeing memory allocated by this function
char *cpuinfo(char *valname)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL)
    {
        return NULL;
    }

    while ((read = getline(&line, &len, fp)) != -1)
    {
        if (strstr(line, valname))
        {
            char *colon = strchr(line, ':');
            int len = colon - line;
            char *buf = malloc(strlen(line) - len);
            strcpy(buf, &line[len + 2]);
            free(line);
            fclose(fp);
            return buf;
        }
    }

    free(line);
    fclose(fp);
    return NULL;
}

// reads current cpu frequency through the /proc/cpuinfo file
// avoid calling this function often
int cpu_speed_mhz()
{
    size_t val;
    char *str = cpuinfo("cpu MHz");
    val = string_to_size(str);
    free(str);
    return val;
}

// reads cpu LLC cache size through the /proc/cpuinfo file
// avoid calling this function often
size_t cpu_llc_size_bytes()
{
    size_t val;
    char *str = cpuinfo("cache size");
    val = string_to_size(str);
    free(str);
    return val;
}

// caller is responsible for freeing memory allocated by this function
char *cpu_model_name()
{
    return cpuinfo("model name");
}

int match(const char *to_match, const char *regex_text)
{
    int ret;
    const char *p = to_match;
    regex_t regex;
    regmatch_t m[1];

    if ((ret = regcomp(&regex, regex_text, REG_EXTENDED | REG_NEWLINE)) != 0)
    {
        return E_ERROR;
    }
    if ((ret = regexec(&regex, p, 1, m, 0)))
    {
        regfree(&regex);
        return E_ERROR; // no match
    }
    regfree(&regex);
    return E_SUCCESS;
}

int is_Xeon()
{
    char *model_name;
    if ((model_name = cpu_model_name()) == NULL)
    {
        return 0;
    }

    if (match(model_name, "Xeon") == E_SUCCESS)
    {
        free(model_name);
        return 1;
    }
    else
    {
        free(model_name);
        return 0;
    }
}

int is_Intel()
{
    char *model_name;
    if ((model_name = cpu_model_name()) == NULL)
    {
        return 0;
    }

    if (match(model_name, "Intel") == E_SUCCESS)
    {
        free(model_name);
        return 1;
    }
    else
    {
        free(model_name);
        return 0;
    }
}

cpu_model_t *cpu_model()
{
    int i, family, model;
    cpu_model_t *cpu_model = NULL;

    if (!is_Intel())
        return NULL;

    get_family_model(&family, &model);

    int isXeon = is_Xeon(); // Assuming Sapphire Rapids will identify as Xeon

    for (i = 0; known_cpus[i].microarch != Invalid; i++)
    {
        microarch_ID_t c = known_cpus[i];

        if (c.family == family && c.model == model)
        {
            switch (c.microarch)
            {
            case SandyBridge:
            case SandyBridgeXeon: // Group Xeon/non-Xeon if they share base model
                cpu_model = &cpu_model_intel_xeon_ex;
                // Adjust microarch based on isXeon only if non-Xeon variant exists
                if (!isXeon && cpu_model->microarch == SandyBridgeXeon)
                     cpu_model->microarch = SandyBridge;
                break;
            case IvyBridge:
            case IvyBridgeXeon:
                cpu_model = &cpu_model_intel_xeon_ex_v2;
                if (!isXeon && cpu_model->microarch == IvyBridgeXeon)
                     cpu_model->microarch = IvyBridge;
                break;
            case Haswell:
            case HaswellXeon:
                cpu_model = &cpu_model_intel_xeon_ex_v3;
                 if (!isXeon && cpu_model->microarch == HaswellXeon)
                     cpu_model->microarch = Haswell;
                break;
            case SapphireRapidsXeon: // Add Sapphire Rapids case
                 cpu_model = &cpu_model_intel_xeon_spr;
                 // Assuming only Xeon variant for SPR for now
                 if (!isXeon) {
                     // Handle non-Xeon SPR if it exists and needs different settings
                     // cpu_model->microarch = SapphireRapids; // If defined
                     DBG_LOG(WARNING, "Non-Xeon Sapphire Rapids detected, using Xeon settings.\n");
                 }
                 break;
            default:
                // Should not happen if known_cpus is correct
                return NULL;
            }

            // This logic might need refinement if Xeon/non-Xeon have different pmc_events
            // The current xeon-ex.h defines models based on Xeon initially.
            // We adjust the microarch enum value here if it's not a Xeon.
            // if (!isXeon) {
            //     // Check if a non-Xeon enum value exists (e.g., SandyBridge vs SandyBridgeXeon)
            //     if (cpu_model->microarch % 2 != 0) { // Simple check assuming Xeon is odd, non-Xeon is even
            //          cpu_model->microarch = (microarch_t)(cpu_model->microarch - 1);
            //     }
            // }


            DBG_LOG(INFO, "Detected CPU model '%s' (Family: 0x%X, Model: 0x%X)\n",
                    microarch_strings[cpu_model->microarch], family, model);
            break; // Found matching CPU
        }
    }

    if (!cpu_model)
    {
         DBG_LOG(ERROR, "Unsupported CPU detected (Family: 0x%X, Model: 0x%X)\n", family, model);
        return NULL;
    }

    // complete the model with some runtime information
    cpu_model->llc_size_bytes = cpu_llc_size_bytes();
    //    cpu_model->speed_mhz = cpu_speed_mhz();

    return cpu_model;
}
