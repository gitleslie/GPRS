#include "sys_callback.h"
#include "sys_services.h"
#include "sys_ext.h"
#include "string.h"
#include "cmd.h"
#include "app.h"
#include "debug.h"
#include "cmddef.h"
#include "manage.h"
#include "stdio.h"
#include "LPG.h"

#define LPG_CONTROL_ON     1
#define LPG_CONTROL_OFF    0
#define LPG_ON     1
#define LPG_OFF    0
#define LPG_OUTPUT_SET   0
#define LPG_SYS_NORMAL_PERIOD_ON    7000
#define LPG_SYS_NORMAL_PERIOD_OFF   7000
#define LPG_SYS_PDPactive_SIGNAL_PERIOD_ON     1000
#define LPG_SYS_PDPactive_SIGNAL_PERIOD_OFF    1000
#define LPG_SYS_RING_SIGNAL_PERIOD_ON       500
#define LPG_SYS_RING_SIGNAL_PERIOD_OFF      500


static INT32 LPG_ctrl(int  ctrl)
{
       int ret=-1;
        GAPP_OPT_LPG_CONTROL_T  o;
        if(1 == ctrl)
        {
            o.op = TRUE;
        }
        else
        {
            o.op = FALSE;
        }

        ret = sys_set(GAPP_OPT_LPG_CONTROL_ID,&o,sizeof(o));
	  if(!ret)
        {
            //DB(DB2,"ret %d",ret);
            return ret;
        }
    return ret;
}

static INT32 LPG_set(int ON_OFF)
{
    INT32 ret=-1;
    ret = sys_gpio_set(GAPP_IO_4,ON_OFF);
    if(!ret)
    {
           //DB(DB2,"LPG set level:%d ",ON_OFF);
           return ret;
     }
    return ret;
}

static INT32 LPG_cfg(int  output)
{
    INT32 ret=-1;
    ret = sys_gpio_cfg(GAPP_IO_4,output);
    if(!ret)
    {
           //DB(DB2,"LPG set outputl:%d ",output);
           return ret;
     }
    return ret;
}
void LPG_init(void)
{
     LPG_ctrl(LPG_CONTROL_ON );
     LPG_cfg(LPG_OUTPUT_SET);
     LPG_stat = LPG_SYS_NORMAL;
}
static void LPG_NORMAL_PWM(void)
{
     LPG_set(LPG_ON);
     sys_taskSleep(1000);
     LPG_set(LPG_OFF);
     sys_taskSleep(10000);
     DB(DB2,"LPG_NORMAL_PWM...............");
}
static void LPG_PDPactive_SIGNAL_PWM(void)
{
     LPG_set(LPG_ON);
     sys_taskSleep(LPG_SYS_PDPactive_SIGNAL_PERIOD_ON );
     LPG_set(LPG_OFF);
     sys_taskSleep(LPG_SYS_PDPactive_SIGNAL_PERIOD_OFF);
     DB(DB2,"LPG_PDPactive_SIGNAL_PWM...............");
}
static void LPG_RING_SIGNAL_PWM(void)
{
     LPG_set(LPG_ON);
     sys_taskSleep(LPG_SYS_RING_SIGNAL_PERIOD_ON);
     LPG_set(LPG_OFF);
     sys_taskSleep(LPG_SYS_RING_SIGNAL_PERIOD_OFF);
     DB(DB2,"LPG_RING_SIGNAL_PWM...............");
}
void LPG_set_mode(UINT32 mode)
{
     LPG_stat=mode;
}
INT32 LPG_get_current_mode(void)
{
     return LPG_stat;
}
void LPG_TAsK(void)
{
	LPG_stat=LPG_get_current_mode();
	//DB(DB2,"LPG_stat=%d",LPG_stat);
	switch(LPG_stat)
	{
		case LPG_SYS_NORMAL:
		{
			LPG_NORMAL_PWM();
			//LPG_TAsK( );
			sys_taskSend(TASK_THREE,1,0,0,0);
		}
		break;
	/*	case LPG_SYS_PDPactive_SIGNAL:
		{
			LPG_PDPactive_SIGNAL_PWM();
			//LPG_TAsK( );
			sys_taskSend(TASK_THREE,1,0,0,0);
		}
		break;
		case LPG_SYS_RING_SIGNAL:
		{
			LPG_RING_SIGNAL_PWM();
			//LPG_TAsK( );
			sys_taskSend(TASK_THREE,1,0,0,0);
		}
		break;   */
		default:
		{
		}
		break;
	}
}

