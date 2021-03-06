/*
 * MD218A voice coil motor driver
 *
 *
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#include "lens_info.h"

#define AF_DRVNAME "WV511AAF"
#define I2C_SLAVE_ADDRESS        0x18

#define AF_DEBUG
#ifdef AF_DEBUG
#define LOG_INF(format, args...) pr_debug(AF_DRVNAME " [%s] " format, __func__, ##args)
#else
#define LOG_INF(format, args...)
#endif

static spinlock_t *g_AF_SpinLock;
static struct i2c_client *g_pstAF_I2Cclient;
static int *g_s4AF_Opened;

static int g_i4Dir;
static int g_sr;
static int g_i4MotorStatus;

static unsigned long g_u4AF_INF;
static unsigned long g_u4AF_MACRO = 1023;
static unsigned long g_u4TargetPosition;
static unsigned long g_u4CurrPosition;

static int s4AF_ReadReg(unsigned short *a_pu2Result)
{
	int i4RetValue = 0;
	char pBuff[2];

	i4RetValue = i2c_master_recv(g_pstAF_I2Cclient, pBuff, 2);

	if (i4RetValue < 0) {
		LOG_INF("I2C read failed!!\n");
		return -1;
	}

	*a_pu2Result = (((u16) pBuff[0]) << 4) + (pBuff[1] >> 4);

	return 0;
}

static int s4AF_WriteReg(u16 a_u2Data)
{
	int i4RetValue = 0;

	char puSendCmd[2] = { (char)(a_u2Data >> 4), (char)((a_u2Data & 0xF) << 4) };

	/* LOG_INF("g_sr %d, write %d\n", g_sr, a_u2Data); */
	/* g_pstAF_I2Cclient->ext_flag |= I2C_A_FILTER_MSG; */
	i4RetValue = i2c_master_send(g_pstAF_I2Cclient, puSendCmd, 2);

	if (i4RetValue < 0) {
		LOG_INF("I2C send failed!!\n");
		return -1;
	}

	return 0;
}

static inline int getAFInfo(__user stAF_MotorInfo * pstMotorInfo)
{
	stAF_MotorInfo stMotorInfo;

	stMotorInfo.u4MacroPosition = g_u4AF_MACRO;
	stMotorInfo.u4InfPosition = g_u4AF_INF;
	stMotorInfo.u4CurrentPosition = g_u4CurrPosition;
	stMotorInfo.bIsSupportSR = 1;

	if (g_i4MotorStatus == 1)
		stMotorInfo.bIsMotorMoving = 1;
	else
		stMotorInfo.bIsMotorMoving = 0;

	if (*g_s4AF_Opened >= 1)
		stMotorInfo.bIsMotorOpen = 1;
	else
		stMotorInfo.bIsMotorOpen = 0;

	if (copy_to_user(pstMotorInfo, &stMotorInfo, sizeof(stAF_MotorInfo)))
		LOG_INF("copy to user failed when getting motor information\n");

	return 0;
}

#ifdef LensdrvCM3
static inline int getAFMETA(__user stWV511AAF_MotorMETAInfo * pstMotorMETAInfo)
{
	stWV511AAF_MotorMETAInfo stMotorMETAInfo;

	stMotorMETAInfo.Aperture = 2.8;	/* fn */
	stMotorMETAInfo.Facing = 1;
	stMotorMETAInfo.FilterDensity = 1;	/* X */
	stMotorMETAInfo.FocalDistance = 1.0;	/* diopters */
	stMotorMETAInfo.FocalLength = 34.0;	/* mm */
	stMotorMETAInfo.FocusRange = 1.0;	/* diopters */
	stMotorMETAInfo.InfoAvalibleApertures = 2.8;
	stMotorMETAInfo.InfoAvalibleFilterDensity = 1;
	stMotorMETAInfo.InfoAvalibleFocalLength = 34.0;
	stMotorMETAInfo.InfoAvalibleHypeDistance = 1.0;
	stMotorMETAInfo.InfoAvalibleMinFocusDistance = 1.0;
	stMotorMETAInfo.InfoAvalibleOptStabilization = 0;
	stMotorMETAInfo.OpticalAxisAng[0] = 0.0;
	stMotorMETAInfo.OpticalAxisAng[1] = 0.0;
	stMotorMETAInfo.Position[0] = 0.0;
	stMotorMETAInfo.Position[1] = 0.0;
	stMotorMETAInfo.Position[2] = 0.0;
	stMotorMETAInfo.State = 0;
	stMotorMETAInfo.u4OIS_Mode = 0;

	if (copy_to_user(pstMotorMETAInfo, &stMotorMETAInfo, sizeof(stWV511AAF_MotorMETAInfo)))
		LOG_INF("copy to user failed when getting motor information\n");

	return 0;
}
#endif

static inline int moveAF(unsigned long a_u4Position)
{
	int ret = 0;

	if ((a_u4Position > g_u4AF_MACRO) || (a_u4Position < g_u4AF_INF)) {
		LOG_INF("out of range\n");
		return -EINVAL;
	}

	if (*g_s4AF_Opened == 1) {
		unsigned short InitPos;

		ret = s4AF_ReadReg(&InitPos);

		if (ret == 0) {
			LOG_INF("Init Pos %6d\n", InitPos);

			spin_lock(g_AF_SpinLock);
			g_u4CurrPosition = (unsigned long)InitPos;
			spin_unlock(g_AF_SpinLock);

		} else {
			spin_lock(g_AF_SpinLock);
			g_u4CurrPosition = 0;
			spin_unlock(g_AF_SpinLock);
		}

		spin_lock(g_AF_SpinLock);
		*g_s4AF_Opened = 2;
		spin_unlock(g_AF_SpinLock);
	}

	if (g_u4CurrPosition < a_u4Position) {
		spin_lock(g_AF_SpinLock);
		g_i4Dir = 1;
		spin_unlock(g_AF_SpinLock);
	} else if (g_u4CurrPosition > a_u4Position) {
		spin_lock(g_AF_SpinLock);
		g_i4Dir = -1;
		spin_unlock(g_AF_SpinLock);
	} else {
		return 0;
	}

	spin_lock(g_AF_SpinLock);
	g_u4TargetPosition = a_u4Position;
	spin_unlock(g_AF_SpinLock);

	/* LOG_INF("move [curr] %d [target] %d\n", g_u4CurrPosition, g_u4TargetPosition); */

	spin_lock(g_AF_SpinLock);
	g_sr = 3;
	g_i4MotorStatus = 0;
	spin_unlock(g_AF_SpinLock);

	if (s4AF_WriteReg((unsigned short)g_u4TargetPosition) == 0) {
		spin_lock(g_AF_SpinLock);
		g_u4CurrPosition = (unsigned long)g_u4TargetPosition;
		spin_unlock(g_AF_SpinLock);
	} else {
		LOG_INF("set I2C failed when moving the motor\n");

		spin_lock(g_AF_SpinLock);
		g_i4MotorStatus = -1;
		spin_unlock(g_AF_SpinLock);
	}

	return 0;
}

static inline int setAFInf(unsigned long a_u4Position)
{
	spin_lock(g_AF_SpinLock);
	g_u4AF_INF = a_u4Position;
	spin_unlock(g_AF_SpinLock);
	return 0;
}

static inline int setAFMacro(unsigned long a_u4Position)
{
	spin_lock(g_AF_SpinLock);
	g_u4AF_MACRO = a_u4Position;
	spin_unlock(g_AF_SpinLock);
	return 0;
}

/* ////////////////////////////////////////////////////////////// */
long WV511AAF_Ioctl(struct file *a_pstFile, unsigned int a_u4Command, unsigned long a_u4Param)
{
	long i4RetValue = 0;

	switch (a_u4Command) {
	case AFIOC_G_MOTORINFO:
		i4RetValue = getAFInfo((__user stAF_MotorInfo *) (a_u4Param));
		break;
#ifdef LensdrvCM3
	case WV511AAFIOC_G_MOTORMETAINFO:
		i4RetValue = getAFMETA((__user stAF_MotorInfo *) (a_u4Param));
		break;
#endif
	case AFIOC_T_MOVETO:
		i4RetValue = moveAF(a_u4Param);
		break;

	case AFIOC_T_SETINFPOS:
		i4RetValue = setAFInf(a_u4Param);
		break;

	case AFIOC_T_SETMACROPOS:
		i4RetValue = setAFMacro(a_u4Param);
		break;

	default:
		LOG_INF("No CMD\n");
		i4RetValue = -EPERM;
		break;
	}

	return i4RetValue;
}

/* Main jobs: */
/* 1.Deallocate anything that "open" allocated in private_data. */
/* 2.Shut down the device on last close. */
/* 3.Only called once on last time. */
/* Q1 : Try release multiple times. */
int WV511AAF_Release(struct inode *a_pstInode, struct file *a_pstFile)
{
	LOG_INF("Start\n");

	if (*g_s4AF_Opened == 2) {
		g_sr = 5;
		s4AF_WriteReg(200);
		msleep(20);
		s4AF_WriteReg(100);
		msleep(20);
	}

	if (*g_s4AF_Opened) {
		LOG_INF("Free\n");

		spin_lock(g_AF_SpinLock);
		*g_s4AF_Opened = 0;
		spin_unlock(g_AF_SpinLock);
	}

	LOG_INF("End\n");

	return 0;
}

void WV511AAF_SetI2Cclient(struct i2c_client *pstAF_I2Cclient, spinlock_t *pAF_SpinLock,
			   int *pAF_Opened)
{
    printk("zhanghf %s \n",__func__);
	g_pstAF_I2Cclient = pstAF_I2Cclient;
	g_AF_SpinLock = pAF_SpinLock;
	g_s4AF_Opened = pAF_Opened;
}
