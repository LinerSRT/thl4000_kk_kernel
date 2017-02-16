#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/xlog.h>
#include <linux/kernel.h>


#include "kd_camera_hw.h"

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"

/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[kd_camera_hw]"
#define printf_NONE(fmt, arg...)    do {} while (0)
#define printf_FUNC(fmt, arg...)    xlog_printk(ANDROID_LOG_INFO, PFX , fmt, ##arg)

#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define printf printf_FUNC
#define PK_ERR(fmt, arg...)         xlog_printk(ANDROID_LOG_ERR, PFX , fmt, ##arg)
#define PK_XLOG_INFO(fmt, args...) \
                do {    \
                   xlog_printk(ANDROID_LOG_INFO, PFX , fmt, ##arg); \
                } while(0)
#else
#define printf(a,...)
#define PK_ERR(a,...)
#define PK_XLOG_INFO(fmt, args...)
#endif

// this should be opened when SMT4 is ready
//#define POWER_ENABLE_VGP3   

extern void ISP_MCLK1_EN(BOOL En);
extern void ISP_MCLK2_EN(BOOL En);
extern void ISP_MCLK3_EN(BOOL En);


int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{
u32 pinSetIdx = 0;//default main sensor

u32 pinSet[2][8] = {
                    //for main sensor 
                    {GPIO_CAMERA_CMRST_PIN,
                     GPIO_CAMERA_CMRST_PIN_M_GPIO,   /* mode */
                        GPIO_OUT_ONE,                   /* ON state */
                        GPIO_OUT_ZERO,                  /* OFF state */
                     GPIO_CAMERA_CMPDN_PIN,
                     GPIO_CAMERA_CMPDN_PIN_M_GPIO,
                        GPIO_OUT_ZERO,
                        GPIO_OUT_ONE,
                    },
                    // for sub sensor
                    {GPIO_CAMERA_CMRST1_PIN,
                     GPIO_CAMERA_CMRST1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                     GPIO_CAMERA_CMPDN1_PIN,
                        GPIO_CAMERA_CMPDN1_PIN_M_GPIO,
                        GPIO_OUT_ZERO,
                        GPIO_OUT_ONE,
                    },
                   };

    // Main & Sub camera share the same bus
    if ( (DUAL_CAMERA_MAIN_SENSOR == SensorIdx) && currSensorName && (0 == strcmp(SENSOR_DRVNAME_GC5004_MIPI_RAW, currSensorName))){
        pinSetIdx = 0;
    }
    else if ( DUAL_CAMERA_SUB_SENSOR == SensorIdx && currSensorName 
         && (0 == strcmp(SENSOR_DRVNAME_GC2355_MIPI_RAW, currSensorName))) { // xichen mark for disable sub sensor
        pinSetIdx = 1;
    }
    else
    {
        printf("[camera-fix] Not Match ! Bypass:  SensorIdx = %d (1:Main , 2:Sub), SensorName=%s\n", SensorIdx, currSensorName);
        goto _kdCISModulePowerOn_exit_;
    }

   
    //power ON
    if (On) {
		printf("[camera-fix] kdCISModulePowerOn -on:currSensorName=%s\n",currSensorName);
		printf("[camera-fix] kdCISModulePowerOn -on:pinSetIdx=%d\n",pinSetIdx);
		

			if (pinSetIdx == 0)
			{	
			      
              //First Power Pin low and Reset Pin Low
              if (GPIO_CAMERA_INVALID != GPIO_CAMERA_CMPDN_PIN) {
                  if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN_PIN,GPIO_CAMERA_CMPDN_PIN_M_GPIO)){printf("[camera-fix] [CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                  if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN_PIN,1)){printf("[camera-fix] [CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                  if(mt_set_gpio_out(GPIO_CAMERA_CMPDN_PIN,GPIO_OUT_ONE)){printf("[camera-fix] [CAMERA LENS] set gpio failed!! (CMPDN)\n");}
              }
  
              
              if (GPIO_CAMERA_INVALID != GPIO_CAMERA_CMRST_PIN) {
  
                  if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){printf("[camera-fix] [CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
                  if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,1)){printf("[camera-fix] [CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
                  if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ZERO)){printf("[camera-fix] [CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
                  
              }
  
          
          mdelay(50);
      
          printf("[camera-fix] kdCISModulePowerOn get in---  SENSOR_DRVNAME_GC5004_MIPI_RAW (main) \n");


      //VCAM_A
          if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))
          {
              printf("[camera-fix] [CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %d\n", CAMERA_POWER_VCAM_A);
              goto _kdCISModulePowerOn_exit_;
          }
          mdelay(10);
          
          //VCAM_IO
          if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800, mode_name))
          {
              printf("[camera-fix] [CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %d \n", CAMERA_POWER_VCAM_D2);
              goto _kdCISModulePowerOn_exit_;
          }
          mdelay(10);
  

           if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1500,mode_name))
          {
               printf("[camera-fix] [CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %d \n", CAMERA_POWER_VCAM_D);
               goto _kdCISModulePowerOn_exit_;
          }
          mdelay(10);
  
          //AF_VCC
           if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name))
           {
               printf("[camera-fix] [CAMERA SENSOR] Fail to enable analog power (VCAM_AF), power id = %d \n", CAMERA_POWER_VCAM_A2);
               goto _kdCISModulePowerOn_exit_;
           }
          mdelay(10);


      //enable active sensor
                  if (GPIO_CAMERA_INVALID != GPIO_CAMERA_CMRST_PIN) {
                      if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){printf("[camera-fix] [CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
                      if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT)){printf("[camera-fix] [CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
                      if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ONE)){printf("[camera-fix] [CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
                  }
                  mdelay(5);
                  
                  if (GPIO_CAMERA_INVALID != GPIO_CAMERA_CMPDN_PIN) {
                      if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN_PIN,GPIO_CAMERA_CMPDN_PIN_M_GPIO)){printf("[camera-fix] [CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
                      if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN_PIN,GPIO_DIR_OUT)){printf("[camera-fix] [CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
                      if(mt_set_gpio_out(GPIO_CAMERA_CMPDN_PIN,GPIO_OUT_ZERO)){printf("[camera-fix] [CAMERA LENS] set gpio failed!! (CMPDN)\n");}
                  }
                  mdelay(5);

			}
			else if (pinSetIdx == 1)	// sub camera: GC2235
			{ 
			    if(mt_set_gpio_mode(GPIO_CAMERA_CMRST1_PIN,GPIO_CAMERA_CMRST1_PIN_M_GPIO)){printf("[camera-fix] [CAMERA SENSOR] set gpio mode failed!! \n");}     
                if(mt_set_gpio_dir(GPIO_CAMERA_CMRST1_PIN,GPIO_DIR_OUT)){printf("[camera-fix] [CAMERA SENSOR] set gpio dir failed!! \n");}      
                //if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ZERO)){printf("[camera-fix] [CAMERA SENSOR] set gpio failed!! \n");} //In IDA stock dont have this point
                if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN,GPIO_CAMERA_CMPDN1_PIN_M_GPIO)){printf("[camera-fix] [CAMERA SENSOR] set gpio mode failed!! \n");}       
                if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT)){printf("[camera-fix] [CAMERA SENSOR] set gpio dir failed!! \n");}        
                if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,GPIO_OUT_ZERO)){printf("[camera-fix] [CAMERA LENS] set gpio failed!! \n");} //1+7 but massive pinSet don have pinSet[1][8]

                   
			if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))
	        	{
	         	printf("[camera-fix] [CAMERA SENSOR] Fail to enable digital power\n");
	          	//return -EIO;
	          	 goto _kdCISModulePowerOn_exit_;
	       		}
			if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))
			{
				printf("[camera-fix] [CAMERA SENSOR] Fail to enable analog power\n");
				//return -EIO;
				goto _kdCISModulePowerOn_exit_;
			}
			if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1800,mode_name))
			{
			    printf("[camera-fix] [CAMERA SENSOR] Fail to enable digital power\n");
			    //return -EIO;
			    goto _kdCISModulePowerOn_exit_;
			}
			if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name))
			{
				printf("[camera-fix] [CAMERA SENSOR] Fail to enable analog power\n");
				//return -EIO;
				goto _kdCISModulePowerOn_exit_;
			}

                 
    		  	if (GPIO_CAMERA_INVALID != GPIO_CAMERA_CMRST1_PIN){
    			  	if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN,GPIO_CAMERA_CMPDN1_PIN_M_GPIO)){printf("[camera-fix] [CAMERA SENSOR] set gpio mode failed!! \n");}
    			  	if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,1)){printf("[camera-fix] [CAMERA SENSOR] set gpio dir failed!! \n");}
    			  	if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,GPIO_OUT_ZERO)){printf("[camera-fix] [CAMERA SENSOR] set gpio failed!! \n");} // In IDA CMPDN1 pin have OUT_ZERO, not OUT_ONE
    			  	
    			  	//RST pin
    				 if(mt_set_gpio_mode(GPIO_CAMERA_CMRST1_PIN,GPIO_CAMERA_CMRST1_PIN_M_GPIO)){printf("[camera-fix] [CAMERA SENSOR] set gpio mode failed!! \n");}
    				 if(mt_set_gpio_dir(GPIO_CAMERA_CMRST1_PIN,1)){printf("[camera-fix] [CAMERA SENSOR] set gpio dir failed!! \n");}
    				 if(mt_set_gpio_out(GPIO_CAMERA_CMRST1_PIN,GPIO_OUT_ONE)){printf("[camera-fix] [CAMERA SENSOR] set gpio failed!! \n");}
    	  	    }
		  	mdelay(15);
			}
		}
    else {//power OFF
        if(pinSetIdx == 0 ) {
        	
           printf("[camera-fix] kdCISModulePower--off get in---SENSOR_DRVNAME_OV8865_MIPI_RAW \n");
           //Set Power Pin low and Reset Pin Low
           if (GPIO_CAMERA_INVALID != GPIO_CAMERA_CMPDN_PIN) {
               if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN_PIN,GPIO_CAMERA_CMPDN_PIN_M_GPIO)){printf("[camera-fix] [CAMERA LENS] set gpio mode failed!! (CMPDN)\n");}
               if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN_PIN,GPIO_DIR_OUT)){printf("[camera-fix] [CAMERA LENS] set gpio dir failed!! (CMPDN)\n");}
               if(mt_set_gpio_out(GPIO_CAMERA_CMPDN_PIN,GPIO_OUT_ONE)){printf("[camera-fix] [CAMERA LENS] set gpio failed!! (CMPDN)\n");}
           }
           
           //Set Reset Pin Low
            if (GPIO_CAMERA_INVALID != GPIO_CAMERA_CMRST_PIN) {
               if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){printf("[camera-fix] [CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");}
               if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT)){printf("[camera-fix] [CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");}
               if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,GPIO_OUT_ZERO)){printf("[camera-fix] [CAMERA SENSOR] set gpio failed!! (CMRST)\n");}
           }
           mdelay(5);
    
           if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name)) {
               printf("[camera-fix] [CAMERA SENSOR] Fail to OFF digital power\n");
               //return -EIO;
               goto _kdCISModulePowerOn_exit_;
           }
    
           if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2,mode_name))
           {
               printf("[camera-fix] [CAMERA SENSOR] Fail to enable analog power\n");
               //return -EIO;
               goto _kdCISModulePowerOn_exit_;
           }
    
           if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,mode_name)) {
               printf("[camera-fix] [CAMERA SENSOR] Fail to OFF analog power\n");
               //return -EIO;
               goto _kdCISModulePowerOn_exit_;
           }
    
           if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2,mode_name))
           {
               printf("[camera-fix] [CAMERA SENSOR] Fail to enable analog power\n");
               //return -EIO;
               goto _kdCISModulePowerOn_exit_;
           }
        }else if(pinSetIdx == 1) {
            
            printf("[camera-fix] [%s][CameraPowerOFF] %s camera : %s\n", __FUNCTION__ , (pinSetIdx == 0 ? "Main" : "Sub") , currSensorName);
            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][4]) {
                  if(mt_set_gpio_mode(pinSet[pinSetIdx][0],pinSet[pinSetIdx][0+1])){printf("[camera-fix] [CAMERA SENSOR] set gpio mode failed!! \n");}
                  if(mt_set_gpio_dir(pinSet[pinSetIdx][0],GPIO_DIR_OUT)){printf("[camera-fix] [CAMERA SENSOR] set gpio dir failed!! \n");}           
                  if(mt_set_gpio_out(pinSet[pinSetIdx][0],pinSet[pinSetIdx][0+3])){printf("[camera-fix] [CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor

                  if(mt_set_gpio_mode(pinSet[pinSetIdx][4],pinSet[pinSetIdx][4+1])){printf("[camera-fix] [CAMERA SENSOR] set gpio mode failed!! \n");}
                  if(mt_set_gpio_dir(pinSet[pinSetIdx][4],GPIO_DIR_OUT)){printf("[camera-fix] [CAMERA SENSOR] set gpio dir failed!! \n");}
                  if(mt_set_gpio_out(pinSet[pinSetIdx][4],pinSet[pinSetIdx][4+2])){printf("[camera-fix] [CAMERA SENSOR] set gpio failed!! \n");} //high == power down lens module
            }
            
                if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,mode_name))
	        {
	            printf("[camera-fix] [CAMERA SENSOR] Fail to enable analog power\n");
	            //return -EIO;
	            goto _kdCISModulePowerOn_exit_;
	        }     	
	       
	        if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2,mode_name))
	        {
	            printf("[camera-fix] [CAMERA SENSOR] Fail to enable digital power\n");
	            //return -EIO;
	            goto _kdCISModulePowerOn_exit_;
	        }  
		 if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D,mode_name))
	        {
	            printf("[camera-fix] [CAMERA SENSOR] Fail to enable digital power\n");
	            //return -EIO;
	            goto _kdCISModulePowerOn_exit_;
	        }  
		if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2,mode_name)) {
	            printf("[camera-fix] [CAMERA SENSOR] Fail to OFF analog power\n");
	            //return -EIO;
	            goto _kdCISModulePowerOn_exit_;
	        }
        }
    }//

	return 0;

_kdCISModulePowerOn_exit_:
    return -EIO;
}

EXPORT_SYMBOL(kdCISModulePowerOn);


//!--
//




