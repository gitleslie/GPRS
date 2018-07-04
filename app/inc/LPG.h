#include "debug.h"

typedef enum
{
   LPG_SYS_NORMAL,
   //LPG_SYS_PDPactive_SIGNAL,
   //LPG_SYS_RING_SIGNAL
}LPG_SYS_SIGNAL_STAT;

extern LPG_SYS_SIGNAL_STAT  LPG_stat;
void LPG_init(void);
void LPG_set_mode(UINT32 mode);
INT32 LPG_get_current_mode(void);
void LPG_TAsK(void );

