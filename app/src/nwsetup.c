#include "stdio.h"
#include "string.h"
#include "sys_services.h"
#include "debug.h"
#include "manage.h"
#include "sys_ext.h"
#include "nwsetup.h"
#include "lite-log.h"
#include "hardware.h"

#include "cmd.h"

int g_MCC = 0;
int g_MNC = 0;
char PLMN[6];
INT8 g_APN[64];
char IMSI[16];
extern UINT32 SecondCnt;
extern UINT8 GprsAttched;
extern UINT8 GprsActived;

//extern char hdw_imei[15];
INT32 GetAPNFromCarrier() {
#if 1
    memset(IMSI, 0, 16);
    if (0 != AT_GetIMSI(IMSI)) {
        log_err("GAgent_GetAPNFromCarrier:Get cellinfo error,apn set to cmnet.");
        sprintf(g_APN, "cmnet");
        return RET_FAILED;
    } else {
        log_info("IMSI:%s", IMSI);
        strncpy(PLMN, IMSI, 5);
        //log_info("PLMN:%s",PLMN);
        //log_info("GAgent_GetAPNFromCarrier:Get Carrier: %s", PLMN);
    }

    sprintf(g_APN, "cmnet");

    /*if(g_MCC == 460)
    {
        switch(g_MNC){
            case 0:
            case 2:
            case 7:
                sprintf(g_APN, "cmnet");
                break;
            case 01:
                sprintf(g_APN, "3gnet");
                break;
            case 06:
                sprintf(g_APN, "UNIM2M.GZM2MAPN");    //UNIM2M.GZM2MAPN
                break;
            default:
                GAgent_Printf(GAGENT_INFO, "GAgent_GetAPNFromCarrier: 460,but APN not in list ,pls add APN");
                return RET_FAILED;
        }
    }*/
    if (strcmp(PLMN, "46006") == 0) {
        sprintf(g_APN, "UNIM2M.GZM2MAPN");    //中国联通物�
    } else if (strcmp(PLMN, "46004") == 0) {
        sprintf(g_APN, "CMCC.IOTAPN");    //中国移动物联
    } else if ((strcmp(PLMN, "46000") == 0) || (strcmp(PLMN, "46002") == 0) || (strcmp(PLMN, "46007") == 0)) {
        sprintf(g_APN, "cmnet");    //中国移动
    } else if ((strcmp(PLMN, "46001") == 0) || (strcmp(PLMN, "46009") == 0)) {
        sprintf(g_APN, "3gnet");    //中国联�
    } else {
        log_info("GetAPNFromCarrier: APN not in list ,pls add APN");
        return RET_FAILED;
    }
#endif
    //log_info("GetAPNFromCarrier: Sent APN = %s", g_APN);
    return RET_SUCCESS;
}

#if 0
INT32 GAgent_GetAPNFromCarrier()
{
#if 1

    CellInfo_t f_cellinfo[7];
    memset(f_cellinfo,0,sizeof(CellInfo_t)*7);
    if(0 != AT_GetCellInfo(f_cellinfo)){
        GAgent_Printf(GAGENT_ERROR, "GAgent_GetAPNFromCarrier:Get cellinfo error,apn set to cmnet.");
        sprintf(g_APN, "cmnet");
        return RET_FAILED;
    }else{
        g_MCC = f_cellinfo[0].iMcc;
        g_MNC = f_cellinfo[0].iMnc;
        GAgent_Printf(GAGENT_INFO, "GAgent_GetAPNFromCarrier:Get Carrier: %d %d", g_MCC, g_MNC);
        //GAgent_Printf(GAGENT_INFO, "Get Carrier: %d %d", f_cellinfo[0].iMcc, f_cellinfo[0].iMnc);
    }
    sprintf(g_APN, "cmnet");

    if(g_MCC == 460)
    {
        switch(g_MNC){
            case 0:
            case 2:
            case 7:
                sprintf(g_APN, "cmnet");
                break;
            case 01:
                sprintf(g_APN, "3gnet");
                break;
            case 06:
                sprintf(g_APN, "UNIM2M.GZM2MAPN");    //UNIM2M.GZM2MAPN
                break;
            default:
                GAgent_Printf(GAGENT_INFO, "GAgent_GetAPNFromCarrier: 460,but APN not in list ,pls add APN");
                return RET_FAILED;
        }
    }
    else{
        GAgent_Printf(GAGENT_INFO, "GAgent_GetAPNFromCarrier: APN not in list ,pls add APN");
        return RET_FAILED;
        }
#endif
    GAgent_Printf(GAGENT_INFO, "GAgent_GetAPNFromCarrier: Sent APN = %s", g_APN);
    return RET_SUCCESS;

}
#endif


void Network_Tick_Form_TCP()
{
    /*int GprsStatus = 0;
    UINT32 ip = 0;
    INT32 ret = 0;
    UINT8 *p;
    static UINT8 attchErrCnt = 0;
    //fflush(stdout);
    GprsAttched = 1;
    GprsActived = 0;
    if (0 == GprsActived) {
            ip = 0;
            log_info("GetAPNFromCarrier!!");
            if (RET_FAILED == GetAPNFromCarrier()) {
                log_err("Get APN error!!!check network And Sim card!!");
                return;
            } else {
                //ret = sys_PDPActive("CMNET",NULL,NULL,&ip);
                log_info("APN == %s",g_APN);
                ret = sys_PDPActive(g_APN, NULL, NULL, &ip);
            }

            p = (UINT8 *) &ip;
            if ((0 == ret) && (0 != ip)) {
                GprsActived = 1;
                log_info("ret %d,ip_addr:%d.%d.%d.%d", ret, p[0], p[1], p[2], p[3]);
                log_info("Ready to connect MQTT Server");
            }
        }*/
    GprsAttched = 1;
    GprsActived = 1;
}




void Network_Tick() {
    int GprsStatus = 0;
    UINT32 ip = 0;
    INT32 ret = 0;
    UINT8 *p;
    static UINT8 attchErrCnt = 0;
    static UINT8 ga_errCnt = 0;
    static UINT8 ip_errCnt = 0;
    //fflush(stdout);
    /*if (SecondCnt < 10)
        return;*/
    //if (0 == (SecondCnt % 2)) {
        // log_info("SecondCnt == %d",SecondCnt);
        GprsStatus = AT_GetGRegister();
        // log_info("AT_GetGRegister Time == %d",sys_getSysTick() / 16384);
        //log_info("GprsStatus == %d",GprsStatus);
        if (1 == GprsStatus) {
            GprsAttched = 1;
            attchErrCnt = 0;
        } else {
            GprsAttched = 0;
            attchErrCnt++;
            if (attchErrCnt > 100) {
                log_info("Bacause attchErrCnt > 100,sys_softReset");
                sys_taskSend(TASK_ONE,T1_SOFT_RESET,0,0,0);
                //sys_softReset();
            }
        }
    //}
    if(SecondCnt < 4)
       return ;
    if (1 == GprsAttched) {
        if (0 == GprsActived) {
            ip = 0;
            //log_info("GetAPNFromCarrier!!");
            if (RET_FAILED == GetAPNFromCarrier()) {
                // log_err("Get APN error!!!check network And Sim card!!  ga_errCnt == %d",ga_errCnt); 
                ga_errCnt++;
                if(ga_errCnt >20)
                    sys_taskSend(TASK_ONE,T1_SOFT_RESET,0,0,0);
                return;
            } else {
                //ret = sys_PDPStatus(&ip);
                //ret = sys_PDPActive("CMNET",NULL,NULL,&ip);
                log_info("Begin TO PDPActive");
                ret = sys_PDPActive(g_APN, NULL, NULL, &ip);
                log_info("End PDPDActive And ret == %d",ret);
                log_info("g_APN == %s",g_APN);
                p = (UINT8 *)&ip;
                log_info("ret %d,ip_addr:%d.%d.%d.%d", ret, p[0], p[1], p[2], p[3]);
            }

            p = (UINT8 *) &ip;
            if ((0 == ret) && (0 != ip)) {
                GprsActived = 1;
                log_info("ret %d,ip_addr:%d.%d.%d.%d", ret, p[0], p[1], p[2], p[3]);
                //log_info("Ready to connect MQTT Server");
                log_info("GPRS Network Init OK And Time == %d",sys_getSysTick() / 16384);

                cmd_aliconn(NULL);
            }
            if(ip == 0)
            {
                log_info("IP error");
                ip_errCnt++;
                if(ip_errCnt > 30)
                {
                    sys_taskSend(TASK_ONE,T1_SOFT_RESET,0,0,0);
                }
            }
        }
    }
}

