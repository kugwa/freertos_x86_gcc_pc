#include "stdint.h"

#define GET_EVTCHN_BIT(map, bit) \
    (((map)[(bit) / sizeof(xen_ulong_t)] & (1 << ((bit) % sizeof(xen_ulong_t)))) > 0)

#define SET_EVTCHN_BIT(map, bit) \
    (map)[(bit) / sizeof(xen_ulong_t)] |= (1 << ((bit) % sizeof(xen_ulong_t)))

#define CLEAR_EVTCHN_BIT(map, bit) \
    (map)[(bit) / sizeof(xen_ulong_t)] &= (~(1 << ((bit) % sizeof(xen_ulong_t))))

int xenstore_write(const char *key, const char *value);
int xenstore_read(const char *key, char *value, uint32_t value_len);
int xenstore_init(void);
