#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include "hp_ipc.h"
#include "hp_sys.h"
#include "hp_vi.h"
#include "hp_vpss.h"
#include "hp_venc.h"


HI_S32 IPC_Init(HI_U32 u32StreamNum, FARMAT_S *stFmt_a)
{
	HI_S32 i,s32Ret;
	if((STREAM_MAX_NUM < u32StreamNum) || (NULL == stFmt_a))
	{
		printf("Invalid parament\n");
		return HI_FAILURE;
	}
	g_stIpcVedio.e_chip = HI3518EV200;//主控芯片型号
	g_stIpcVedio.e_sensor_type = SENSOR_PIXELPLUS_PS3210K;//sensor型号
	g_stIpcVedio.e_wdrMode = WDR_MODE_NONE;                 //非宽动态
	g_stIpcVedio.e_pixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;//YUV420
	g_stIpcVedio.u32StreamNum = u32StreamNum;               //通道数
	for(i = 0; i < u32StreamNum; i++)
	{
		s32GetPicSize(stFmt_a[i].e_Size,&(g_stIpcVedio.astStreamFmt[i].stSize));
		g_stIpcVedio.astStreamFmt[i].e_Size = stFmt_a[i].e_Size;//分辨率    
		g_stIpcVedio.astStreamFmt[i].f32FrameRate = stFmt_a[i].f32FrameRate;//帧率
		g_stIpcVedio.astStreamFmt[i].e_payload = PT_H264;   //H.264编码
		g_stIpcVedio.astStreamFmt[i].e_RCMode  = RC_MODE_CBR;//H.264编码的码率控制方法
	}


    s32Ret = mpi_sys_init(&g_stIpcVedio);//初始化 MPP 系统
    if (HI_SUCCESS != s32Ret)
    {
        printf("system init failed with %d!\n", s32Ret);
        goto END_EXIT_0;
    }
	
    s32Ret = VI_init(&g_stIpcVedio);//VI初始化
    if (HI_SUCCESS != s32Ret)
    {
        printf("start vi failed!\n");
        goto END_EXIT_1;
    }
	
    s32Ret = VPSS_init(&g_stIpcVedio);//VPSS初始化
	if (HI_SUCCESS != s32Ret)
    {
        printf("start vpss failed!\n");
        goto END_EXIT_2;
    }

    //VPSS还要与VI绑定，以便VI的数据直接输入VPSS，
    //不需要我们手动从VI复制数据到VPSS,绑定时将绑定源设置为VI的哪
    //一个通道，目的源设置为VPSS的一个通道
	s32Ret = mpi_sys_Bind(&g_stIpcVedio);
	if (HI_SUCCESS != s32Ret)
    {
        printf("Bind failed!\n");
        goto END_EXIT_3;
    }
	
	s32Ret = VENC_init(&g_stIpcVedio);//设置VENC模块
	if (HI_SUCCESS != s32Ret)
    {
        printf("start venc failed!\n");
        goto END_EXIT_4;
    }
	
	return HI_SUCCESS;
	
END_EXIT_4:
	VENC_exit();
END_EXIT_3:
	mpi_sys_UnBind(&g_stIpcVedio);
END_EXIT_2:
	VPSS_exit();
END_EXIT_1:
	VI_exit();
END_EXIT_0:
	mpi_sys_exit();
	exit(0);
	return HI_SUCCESS;
}

HI_S32 IPC_exit(void)
{	
	VENC_exit();
	mpi_sys_UnBind(&g_stIpcVedio);
	VPSS_exit();
	VI_exit();
	mpi_sys_exit();
	return HI_SUCCESS;
}


