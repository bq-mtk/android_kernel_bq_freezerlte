#include <linux/string.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>
#include <asm-generic/gpio.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#endif

#include "lcm_drv.h"

#define LCD_DEBUG(fmt)  printk(fmt)

#define GPIO_OUT_ONE  1
#define GPIO_OUT_ZERO 0

unsigned int GPIO_LCD_PWR_EN;
unsigned int GPIO_LCD_RST_EN;
unsigned int LCM_ID_GPIO;
unsigned int GPIO_65132_EN;
unsigned int LCD_ID;

void lcm_request_gpio_control(struct device *dev)
{
	GPIO_LCD_PWR_EN = of_get_named_gpio(dev->of_node, "gpio_lcm_pwr_en", 0);
	GPIO_LCD_RST_EN = of_get_named_gpio(dev->of_node, "gpio_lcm_rst_en", 0);
	LCM_ID_GPIO = of_get_named_gpio(dev->of_node, "lcm_id_gpio", 0);
	GPIO_65132_EN = of_get_named_gpio(dev->of_node, "lcm_bias_enp_gpio", 0);
	LCD_ID = of_get_named_gpio(dev->of_node, "lcm_id_gpio", 0);

	gpio_request(GPIO_LCD_PWR_EN, "GPIO_LCD_PWR_EN");
	pr_debug("[KE/LCM] GPIO_LCD_PWR_EN = 0x%x\n", GPIO_LCD_PWR_EN);

	gpio_request(GPIO_LCD_RST_EN, "GPIO_LCD_RST_EN");
	pr_debug("[KE/LCM] GPIO_LCD_RST_EN = 0x%x\n", GPIO_LCD_RST_EN);

	gpio_request(LCM_ID_GPIO, "LCM_ID_GPIO");
	pr_debug("[KE/LCM] GPIO_LCD_RST_EN = 0x%x\n", GPIO_LCD_RST_EN);

	gpio_request(GPIO_65132_EN, "GPIO_65132_EN");
	pr_debug("[KE/LCM] GPIO_LCD_RST_EN = 0x%x\n", GPIO_LCD_RST_EN);
	
	gpio_request(LCD_ID, "LCD_ID");
	pr_debug("[KE/LCM] GPIO_LCD_RST_EN = 0x%x\n", GPIO_LCD_RST_EN);
}

static int lcm_probe(struct device *dev)
{
	lcm_request_gpio_control(dev);

	return 0;
}

static const struct of_device_id lcm_of_ids[] = {
	{.compatible = "mediatek,mt6753p1_64-lcm",},
	{}
};

static struct platform_driver lcm_driver = {
	.driver = {
		   .name = "b101uan07_fhd_dsi_vdo_tps65132",
		   .owner = THIS_MODULE,
		   .probe = lcm_probe,
#ifdef CONFIG_OF
		   .of_match_table = lcm_of_ids,
#endif
		   },
};

static int __init lcm_gpio_init(void)
{
	pr_notice("LCM: Register lcm driver for b101uan07_fhd_dsi_vdo_tps65132\n");
	if (platform_driver_register(&lcm_driver)) {
		pr_err("LCM: failed to register disp driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit lcm_gpio_exit(void)
{
	platform_driver_unregister(&lcm_driver);
	pr_notice("LCM: Unregister lcm driver done\n");
}
late_initcall(lcm_gpio_init);
module_exit(lcm_gpio_exit);
MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("Display subsystem Driver");
MODULE_LICENSE("GPL");

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (1200)
#define FRAME_HEIGHT (1920)

#define REGFLAG_DELAY								0XFE
#define REGFLAG_END_OF_TABLE						0xFF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE    0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
static LCM_UTIL_FUNCS lcm_util = {
    .set_gpio_out = NULL,
};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V3(para_tbl,size,force_update)        lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    



#if 0//ndef BUILD_LK //xmlsz 20160802
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
//#include <linux/jiffies.h>
#include <linux/uaccess.h>
//#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
/***************************************************************************** 
 * Define
 *****************************************************************************/

#define TPS_I2C_BUSNUM  2//I2C_I2C_LCD_BIAS_CHANNEL//for I2C channel 0
#define I2C_ID_NAME "tps65132"
#define TPS_ADDR 0x3E

/***************************************************************************** 
 * GLobal Variable
 *****************************************************************************/
static struct i2c_board_info __initdata tps65132_board_info = {I2C_BOARD_INFO(I2C_ID_NAME, TPS_ADDR)};
static struct i2c_client *tps65132_i2c_client = NULL;


/***************************************************************************** 
 * Function Prototype
 *****************************************************************************/ 
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tps65132_remove(struct i2c_client *client);
/***************************************************************************** 
 * Data Structure
 *****************************************************************************/

 struct tps65132_dev	{	
	struct i2c_client	*client;
	
};

static const struct i2c_device_id tps65132_id[] = {
	{ I2C_ID_NAME, 0 },
	{ }
};

//#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
//static struct i2c_client_address_data addr_data = { .forces = forces,};
//#endif
static struct i2c_driver tps65132_iic_driver = {
	.id_table	= tps65132_id,
	.probe		= tps65132_probe,
	.remove		= tps65132_remove,
	//.detect		= mt6605_detect,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "tps65132",
	},
 
};
/***************************************************************************** 
 * Extern Area
 *****************************************************************************/ 
 
 

/***************************************************************************** 
 * Function
 *****************************************************************************/ 
static int tps65132_probe(struct i2c_client *client, const struct i2c_device_id *id)
{  
	printk( "tps65132_iic_probe\n");
	printk("TPS: info==>name=%s addr=0x%x\n",client->name,client->addr);
	tps65132_i2c_client  = client;		
	return 0;      
}


static int tps65132_remove(struct i2c_client *client)
{  	
	printk( "tps65132_remove\n");
	tps65132_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}


static int tps65132_write_bytes(unsigned char addr, unsigned char value)
{	
	int ret = 0;
	struct i2c_client *client = tps65132_i2c_client;
	char write_data[2]={0};	
	write_data[0]= addr;
	write_data[1] = value;
	ret=i2c_master_send(client, write_data, 2);
	if(ret<0)
		printk("tps65132 write data fail !!\n");	
	return ret ;
}



/*
 * module load/unload record keeping
 */

static int __init tps65132_iic_init(void)
{

   printk( "tps65132_iic_init\n");
   i2c_register_board_info(TPS_I2C_BUSNUM, &tps65132_board_info, 1);
   printk( "tps65132_iic_init2\n");
   i2c_add_driver(&tps65132_iic_driver);
   printk( "tps65132_iic_init success\n");	
   return 0;
}

static void __exit tps65132_iic_exit(void)
{
   printk( "tps65132_iic_exit\n");
   i2c_del_driver(&tps65132_iic_driver);  
}


module_init(tps65132_iic_init);
module_exit(tps65132_iic_exit);

MODULE_AUTHOR("Xiaokuan Shi");
MODULE_DESCRIPTION("MTK TPS65132 I2C Driver");
MODULE_LICENSE("GPL"); 

#endif

//#define GPIO_65132_EN GPIO_LCD_BIAS_ENP_PIN

//#define GPIO_LCD_BIAS_ENP_PIN		GPIO84
//#define GPIO_LCM_RST				GPIO83
#define LCD_RST_VALUE				GPIO_OUT_ONE
//#define LCD_ID						GPIO32

static struct LCM_setting_table lcm_initialization_setting[] =
{                                                       
	//======Internal setting======
#if 0 // f
	{0x15, 0x8F, 1, {0xA5}},
	{0x15, 0x8C, 1, {0x80}},
	{0x15, 0x01, 0, {0x00}},
	/*
	{0x15, 0xC0, 1, {0x01}},
	{0x15, 0xC1, 1, {0xA0}},
	{0x15, 0xC2, 1, {0x40}},
	{0x15, 0xC3, 1, {0x0C}},
	{0x15, 0xC4, 1, {0x01}},
	{0x15, 0xC5, 1, {0x21}},
	{0x15, 0xC6, 1, {0x29}},
	{0x15, 0xC7, 1, {0x49}},
	{0x15, 0xC8, 1, {0xF0}},
	{0x15, 0xC9, 1, {0x00}},
	{0x15, 0xCA, 1, {0x18}},
	{0x15, 0xCB, 1, {0x0C}},
	{0x15, 0xCC, 1, {0x01}},
	{0x15, 0xCD, 1, {0x7C}},
	{0x15, 0xCE, 1, {0xC0}},
	{0x15, 0xCF, 1, {0x00}},
	{0x15, 0xD0, 1, {0x01}},
	{0x15, 0xD1, 1, {0x44}},
	{0x15, 0xD2, 1, {0x01}},
	{0x15, 0xD3, 1, {0x44}},
	{0x15, 0xD4, 1, {0x18}},
	{0x15, 0xD5, 1, {0x0C}},
	{0x15, 0xD6, 1, {0x01}},
	{0x15, 0xD7, 1, {0x04}},
	{0x15, 0xD8, 1, {0x00}},
	{0x15, 0xD9, 1, {0x09}},
	{0x15, 0xDA, 1, {0x03}},
	{0x15, 0xDB, 1, {0x61}},
	{0x15, 0xDC, 1, {0x02}},
	{0x15, 0xDD, 1, {0x02}},
	{0x15, 0xDE, 1, {0x02}},
	{0x15, 0xDF, 1, {0x00}},
	{0x15, 0x83, 1, {0xAA}},
	{0x15, 0x84, 1, {0x11}},
	{0x15, 0xA9, 1, {0x4B}},
	{0x15, 0x83, 1, {0x00}},
	{0x15, 0x84, 1, {0x00}},
	{0x15, 0x8F, 1, {0x00}},
	*/
#else // from f1005
	//======Internal setting======
	{0x8F, 1, {0xA5}},
	{REGFLAG_DELAY, 10, {}},
	{0x01, 0, {0x00}},
	{REGFLAG_DELAY, 30, {}},
	{0x8F, 1, {0xA5}},
	{REGFLAG_DELAY, 10, {}},
	//{0x85, 1, {0x04}},
	//{0x86, 1, {0x08}},
	//{0x8C, 1, {0x80}},
	{0xC0, 1, {0x01}},
	{0xC1, 1, {0xA0}},
	{0xC2, 1, {0x40}},
	{0xC3, 1, {0x0C}},
	{0xC4, 1, {0x01}},
	{0xC5, 1, {0x21}},
	{0xC6, 1, {0x29}},
	{0xC7, 1, {0x49}},
	{0xC8, 1, {0xF0}},
	{0xC9, 1, {0x00}},
	{0xCA, 1, {0x18}},
	{0xCB, 1, {0x0C}},
	{0xCC, 1, {0x01}},
	{0xCD, 1, {0x7C}},
	{0xCE, 1, {0xC0}},
	{0xCF, 1, {0x00}},
	{0xD0, 1, {0x01}},
	{0xD1, 1, {0x44}},
	{0xD2, 1, {0x01}},
	{0xD3, 1, {0x44}},
	{0xD4, 1, {0x18}},
	{0xD5, 1, {0x0C}},
	{0xD6, 1, {0x01}},
	{0xD7, 1, {0x04}},
	{0xD8, 1, {0x00}},
	{0xD9, 1, {0x09}},
	{0xDA, 1, {0x03}},
	{0xDB, 1, {0x61}},
	{0xDC, 1, {0x02}},
	{0xDD, 1, {0x02}},
	{0xDE, 1, {0x02}},
	{0xDF, 1, {0x00}},
	{0x83, 1, {0xAA}},
	{0x84, 1, {0x11}},
	{0xA9, 1, {0x4B}},
	{0x83, 1, {0x00}},
	{0x84, 1, {0x00}},
	{0x11, 0, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{0x29, 0, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
#endif
};                                                     

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{	
    gpio_direction_output(GPIO, output);	
	gpio_set_value(GPIO, output);
}

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for(i = 0; i < count; i++) {
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {
		case REGFLAG_DELAY :
		    MDELAY(table[i].count);
		    break;
			
		case REGFLAG_END_OF_TABLE :
		    break;
			
		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
	
}


static void lcm_reset(unsigned char enabled)
{
	//mt_set_gpio_mode(GPIO_LCD_RST_EN, GPIO_MODE_00);//[A]Set lcm_rst to gpio mode ,xmlwz ,20130313
	//mt_set_gpio_dir(GPIO_LCD_RST_EN, GPIO_DIR_OUT);
	if(enabled)
	{	
		lcm_set_gpio_output(GPIO_LCD_RST_EN, LCD_RST_VALUE); //[M]For different reset gpio level,ArthurLin,20130809
		MDELAY(40);
		lcm_set_gpio_output(GPIO_LCD_RST_EN, !LCD_RST_VALUE); //[M]For different reset gpio level,ArthurLin,20130809
		MDELAY(15);
		lcm_set_gpio_output(GPIO_LCD_RST_EN, LCD_RST_VALUE); //[M]For different reset gpio level,ArthurLin,20130809
	}else{
		lcm_set_gpio_output(GPIO_LCD_RST_EN, !LCD_RST_VALUE); 		
	}		
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type     = LCM_TYPE_DSI;
    params->width    = FRAME_WIDTH;
    params->height   = FRAME_HEIGHT;
    params->dsi.mode =BURST_VDO_MODE;

    params->physical_width=135;
    params->physical_height=217;

    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM			= LCM_FOUR_LANE;
    params->dsi.data_format.format		= LCM_DSI_FORMAT_RGB888;

    // Highly depends on LCD driver capability.
    // Not support in MT6573
    //params->dsi.packet_size=256;

    // Video mode setting
	
    //params->dsi.intermediat_buffer_num = 2;
		
    params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;//LCM_PACKED_PS_18BIT_RGB666;

    params->dsi.vertical_sync_active				= 5;
    params->dsi.vertical_backporch				= 2;
    params->dsi.vertical_frontporch 				= 13;
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active			= 16;
    params->dsi.horizontal_backporch				= 8;
    params->dsi.horizontal_frontporch				= 116;
    params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;

    // Bit rate calculation
    //params->dsi.pll_div1=34;//32		// fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
    //params->dsi.pll_div2=1; 		// div2=0~15: fout=fvo/(2*div2)
    params->dsi.PLL_CLOCK = 460;
}

#if 0 //def BUILD_LK//xmlsz 20160802

#define TPS65132_SLAVE_ADDR_WRITE  0x7C  
static struct mt_i2c_t TPS65132_i2c;

static int TPS65132_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    TPS65132_i2c.id = 2;//I2C_I2C_LCD_BIAS_CHANNEL;//I2C2;
    /* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
    TPS65132_i2c.addr = (TPS65132_SLAVE_ADDR_WRITE >> 1);
    TPS65132_i2c.mode = ST_MODE;
    TPS65132_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&TPS65132_i2c, write_data, len);
    //printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code);

    return ret_code;
}

#endif

static void lcm_suspend_power(void)
{
	lcm_set_gpio_output(GPIO_65132_EN, GPIO_OUT_ZERO);
}

static void lcm_resume_power(void)
{
	//unsigned char cmd = 0x0;
	//unsigned char data = 0xFF;
	//int ret=0;
	
//	cmd=0x00;
//	data=0x12;
	
	lcm_set_gpio_output(GPIO_65132_EN, GPIO_OUT_ONE);
#if 0//xmlsz 20160802
#ifdef BUILD_LK
	ret=TPS65132_write_byte(cmd,data);
	if(ret)    	
		dprintf(0, "[LK]b101uan07----tps65132----cmd=%0x--i2c write error----\n",cmd);    	
	else
		dprintf(0, "[LK]b101uan07----tps65132----cmd=%0x--i2c write success----\n",cmd);    		
#else
	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]b101uan07----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]b101uan07----tps65132---cmd=%0x-- i2c write success-----\n",cmd);
#endif

	cmd=0x01;
	data=0x12;
#ifdef BUILD_LK
	ret=TPS65132_write_byte(cmd,data);
	if(ret)    	
		dprintf(0, "[LK]b101uan07----tps65132----cmd=%0x--i2c write error----\n",cmd);
	else
		dprintf(0, "[LK]b101uan07----tps65132----cmd=%0x--i2c write success----\n",cmd);
#else
	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]b101uan07----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]b101uan07----tps65132---cmd=%0x-- i2c write success-----\n",cmd);
#endif
	cmd=0x03;
	data=0x43;/*[A]Set APPLICATION in Tablet mode (default is 0x03 ,smartphone mode), xmlwz , 20150317 */
#ifdef BUILD_LK
	ret=TPS65132_write_byte(cmd,data);
	if(ret)    	
		dprintf(0, "[LK]b101uan07----tps65132----cmd=%0x--i2c write error----\n",cmd);    	
	else
		dprintf(0, "[LK]b101uan07----tps65132----cmd=%0x--i2c write success----\n",cmd);   
#else
	ret=tps65132_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]b101uan07----tps65132---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]b101uan07----tps65132---cmd=%0x-- i2c write success-----\n",cmd);
#endif
#endif
}

static void init_lcm_registers(void)
{
    //unsigned int data_array[16];
#if 0  
    
#ifdef BUILD_LK
    printf("%s, LK \n", __func__);
#else
    printk("%s, kernel", __func__);
#endif
    
    data_array[0] = 0x00010500;  //software reset					 
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);
    DSI_clk_HS_mode(1);
    MDELAY(80);
    
    data_array[0] = 0x00290500;  //display on                        
    dsi_set_cmdq(data_array, 1, 1);
#endif
#ifdef BUILD_LK
    printf(" %s, LK  exit \n", __func__);
#else
    printk(" %s, kernel exit\n", __func__);
#endif

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    
}

static void lcm_vdd_power(char ON)
{
    if(ON){
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ONE);
    }else{
	lcm_set_gpio_output(GPIO_LCD_PWR_EN, GPIO_OUT_ZERO);
    }
}

static void lcm_init(void)
{
#ifdef BUILD_LK
	//lcm_reset(0);//[malata]Init reset-pin low ,xmlwz,20150507 
	MDELAY(1);
	//upmu_set_vpa_vosel(0);
	//upmu_set_vpa_en(0);
	//MDELAY(1);
#endif
	lcm_reset(0);//[malata]Init reset-pin low ,xmlwz,20150507 

	//power on vddin enable->VPA_PMU 3.3V
	lcm_vdd_power(1);
	//upmu_set_rg_vgp3_vosel(3);
	//upmu_set_rg_vgp3_en(0x1);
	MDELAY(40);

	//power on avdd and avee
	lcm_resume_power();

	MDELAY(20);
	lcm_reset(1);
	MDELAY(10);//[malata]more than 5ms,xmlwz,20150507
	
	init_lcm_registers();//xmlsz 20160802
}


static void lcm_suspend(void)
{
	unsigned int data_array[16];
#ifdef BUILD_LK
    printf("%s, LK \n", __func__);
#else
    printk("%s, kernel", __func__);
#endif

	data_array[0] = 0x00280500;  //display off                        
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(5);
	data_array[0] = 0x00100500;  //display off                        
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(5);
	lcm_reset(0);
	lcm_suspend_power();

	//power on vddin disable
	lcm_vdd_power(0);
	//upmu_set_rg_vgp3_vosel(0);
	//upmu_set_rg_vgp3_en(0);

	//DSI_clk_HS_mode(0);
	MDELAY(10);
}


static void lcm_resume(void)
{
#ifdef BUILD_LK
    printf("%s, LK \n", __func__);
#else
    printk("%s, kernel", __func__);
#endif
	lcm_init();
}
/*static unsigned int lcm_compare_id(void)
{
	unsigned int id=0;
#if defined(BUILD_LK)
	printf("b101uan07_dsi_vdo_tps65132  lcm_compare_id \n");
#endif
	id = mt_get_gpio_in(LCD_ID);

	return (id==1)?1:0;
}*/
LCM_DRIVER b101uan07_fhd_dsi_vdo_tps65132_lcm_drv = 
{
	.name			= "b101uan07_fhd_dsi_vdo_tps65132",
	.set_util_funcs		= lcm_set_util_funcs,
	.get_params		= lcm_get_params,
	.init				= lcm_init,
	.suspend			= lcm_suspend,
	.resume			= lcm_resume,
//	.compare_id		= lcm_compare_id,
};

