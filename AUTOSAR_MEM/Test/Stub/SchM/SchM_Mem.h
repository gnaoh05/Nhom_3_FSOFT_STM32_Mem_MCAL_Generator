#ifndef SCHM_MEM_H
#define SCHM_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Minimal scheduler contract used by the integration environment.
 * A real AUTOSAR system supplies this declaration through SchM/RTE.
 */
extern void Mem_MainFunction(void);

#ifdef __cplusplus
}
#endif

#endif /* SCHM_MEM_H */
