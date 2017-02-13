#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>

#include "kd_camera_hw.h"

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"


#define PFX "[kd_camera_hw]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    printk(KERN_INFO PFX "%s: " fmt, __FUNCTION__ ,##arg)

#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG PK_DBG_FUNC
#define PK_ERR(fmt, arg...)         printk(KERN_ERR PFX "%s: " fmt, __FUNCTION__ ,##arg)
#else
#define PK_DBG(a,...)
#define PK_ERR(a,...)
#endif

int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{
u32 pinSetIdx = 0;//default main sensor

#define IDX_PS_CMRST 0
#define IDX_PS_CMPDN 4

#define IDX_PS_MODE 1
#define IDX_PS_ON   2
#define IDX_PS_OFF  3
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
                    //for sub sensor 
                    {GPIO_CAMERA_CMRST1_PIN,
                        GPIO_CAMERA_CMRST1_PIN_M_GPIO,
                        GPIO_OUT_ONE,
                        GPIO_OUT_ZERO,
                        GPIO_CAMERA_CMPDN1_PIN,
                        GPIO_CAMERA_CMPDN1_PIN_M_GPIO,
                        GPIO_OUT_ZERO,
                        GPIO_OUT_ONE,
                    }
                   };

    if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx){
	if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_GC5004_MIPI_RAW,currSensorName))){
        pinSetIdx = 0;
	}else{
	printk("[serinity-fix]kdCISModulePowerOn get in---  sensorIdx not compare with sensor name: %s\n", currSensorName);  
	}
    }
    else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx) {
        if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_GC2355_MIPI_RAW,currSensorName))){
        pinSetIdx = 1;
	}else{
	printk("[serinity-fix]kdCISModulePowerOn get in---  sensorIdx not compare with sensor name: %s\n", currSensorName);  
	}
    }

    //power ON
    if (On) {
        //in case


	   
        if(pinSetIdx == 0)  //main
        {
        //disable sub
			if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_GC5004_MIPI_RAW,currSensorName)))
        		{
			        printk("Power on GC5004, pinSetIdx=%d \n",pinSetIdx); 
			if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1500,mode_name))
			{
			     PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
			     //return -EIO;
			     goto _kdCISModulePowerOn_exit_;
			}
		 	 msleep(20);
			if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))
			{
				PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
				//return -EIO;
				goto _kdCISModulePowerOn_exit_;
			}
			 msleep(20);
	        	if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_1800,mode_name))
	        	{
	         	  	 PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
	          	 	 //return -EIO;
	          	 	 goto _kdCISModulePowerOn_exit_;
	       		}
			 msleep(20);
			if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name))
			{
				PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
				//return -EIO;
				goto _kdCISModulePowerOn_exit_;
			}                   
       			 msleep(50);
        // wait power to be stable 
        mdelay(2); 
        
        printk("[serinity-fix] Reset CMPDN1 pin for main sensor%d \n",GPIO_CAMERA_CMPDN1_PIN); //FIXME
		if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
		if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
		if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,1)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module

		//enable main
        printk("[serinity-fix] Enable CMRST pin for main sensor%d \n",GPIO_CAMERA_CMRST_PIN); //FIXME
    	if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
	printk("[serinity-fix] Enable CMPDN pin for main sensor%d \n",GPIO_CAMERA_CMPDN_PIN); //FIXME
        if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN_PIN,GPIO_CAMERA_CMPDN_PIN_M_GPIO)){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
        if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
        if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
        if(mt_set_gpio_out(GPIO_CAMERA_CMPDN_PIN,0)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
        
        if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,0)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor
        mdelay(2);
        if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,1)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor
        mdelay(2);
            }
        }
		else if (pinSetIdx == 1)  //sub
		{
		if (currSensorName && ((0 == strcmp(SENSOR_DRVNAME_GC2355_MIPI_RAW,currSensorName)) || (0 == strcmp(SENSOR_DRVNAME_GC0310MIPI_YUV,currSensorName))))
        	{
        	printk("[serinity-fix] Power on GC2355, pinSetIdx=%d \n",pinSetIdx); 

			 if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800,mode_name))
		    {
		        PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
		        //return -EIO;
		        goto _kdCISModulePowerOn_exit_;
		    } 

			 if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1800,mode_name))
		    {
		        PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
		        //return -EIO;
		        goto _kdCISModulePowerOn_exit_;
		    }                    
		   
		    
		    if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_D2, VOL_2800,mode_name))
		    {
		        PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
		        //return -EIO;
		        goto _kdCISModulePowerOn_exit_;
		    }
			if(TRUE != hwPowerOn(CAMERA_POWER_VCAM_A2, VOL_2800,mode_name))
			{
				PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
				//return -EIO;
				goto _kdCISModulePowerOn_exit_;
			}                   
       			 msleep(50);
			
        mdelay(2); 
        
        printk("[serinity-fix] Reset CMPDN pin for sub sensor%d \n",GPIO_CAMERA_CMPDN1_PIN); //FIXME
		if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
		if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
		if(mt_set_gpio_out(GPIO_CAMERA_CMPDN_PIN,1)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module

        printk("[serinity-fix] Enable CMRST pin for sub sensor%d \n",GPIO_CAMERA_CMRST_PIN); //FIXME
    	if(mt_set_gpio_mode(GPIO_CAMERA_CMRST_PIN,GPIO_CAMERA_CMRST_PIN_M_GPIO)){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
	printk("[serinity-fix] Enable CMPDN1 pin for sub sensor%d \n",GPIO_CAMERA_CMPDN1_PIN); //FIXME
        if(mt_set_gpio_mode(GPIO_CAMERA_CMPDN1_PIN,GPIO_CAMERA_CMPDN1_PIN_M_GPIO)){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
        if(mt_set_gpio_dir(GPIO_CAMERA_CMRST_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
        if(mt_set_gpio_dir(GPIO_CAMERA_CMPDN1_PIN,GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
        if(mt_set_gpio_out(GPIO_CAMERA_CMPDN1_PIN,0)){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
        
        if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,0)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor
        mdelay(2);
        if(mt_set_gpio_out(GPIO_CAMERA_CMRST_PIN,1)){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor
        mdelay(2);
            	}
		}

    }
    else {//power OFF

	    
		

        //PK_DBG("[OFF]sensorIdx:%d \n",SensorIdx);
        if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_MODE])){PK_DBG("[CAMERA SENSOR] set gpio mode failed!! \n");}
            if(mt_set_gpio_mode(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_MODE])){PK_DBG("[CAMERA LENS] set gpio mode failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST],GPIO_DIR_OUT)){PK_DBG("[CAMERA SENSOR] set gpio dir failed!! \n");}
            if(mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN],GPIO_DIR_OUT)){PK_DBG("[CAMERA LENS] set gpio dir failed!! \n");}
            if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMRST],pinSet[pinSetIdx][IDX_PS_CMRST+IDX_PS_OFF])){PK_DBG("[CAMERA SENSOR] set gpio failed!! \n");} //low == reset sensor
    	    if(mt_set_gpio_out(pinSet[pinSetIdx][IDX_PS_CMPDN],pinSet[pinSetIdx][IDX_PS_CMPDN+IDX_PS_OFF])){PK_DBG("[CAMERA LENS] set gpio failed!! \n");} //high == power down lens module
        }

        if (pinSetIdx == 0 )  //main
        {

	    	if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,mode_name)) {
	            PK_DBG("[CAMERA SENSOR] Fail to OFF analog power\n");
	            //return -EIO;
	            goto _kdCISModulePowerOn_exit_;
	        }
		if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2,mode_name)) {
	            PK_DBG("[CAMERA SENSOR] Fail to OFF analog power\n");
	            //return -EIO;
	            goto _kdCISModulePowerOn_exit_;
	        }
	        if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2,mode_name))
	        {
	            PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
	            //return -EIO;
	            goto _kdCISModulePowerOn_exit_;
	        }     	
	        if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D, mode_name)) {
	            PK_DBG("[CAMERA SENSOR] Fail to OFF digital power\n");
	            //return -EIO;
	            goto _kdCISModulePowerOn_exit_;
	        }
	           
    	}
		else if(pinSetIdx == 1)
		{
	        if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A,mode_name))
	        {
	            PK_DBG("[CAMERA SENSOR] Fail to enable analog power\n");
	            //return -EIO;
	            goto _kdCISModulePowerOn_exit_;
	        }     	
	       
	        if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D2,mode_name))
	        {
	            PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
	            //return -EIO;
	            goto _kdCISModulePowerOn_exit_;
	        }  
		 if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_D,mode_name))
	        {
	            PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
	            //return -EIO;
	            goto _kdCISModulePowerOn_exit_;
	        }  
		if(TRUE != hwPowerDown(CAMERA_POWER_VCAM_A2,mode_name)) {
	            PK_DBG("[CAMERA SENSOR] Fail to OFF analog power\n");
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

//#endif //#if defined(MT6516)



