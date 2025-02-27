/* 
 * e2_proc_avs.c
 */
 
#include <linux/proc_fs.h>  	/* proc fs */ 
#include <asm/uaccess.h>    	/* copy_from_user */

#include <linux/dvb/video.h>	/* Video Format etc */

#include <linux/dvb/audio.h>
#include <linux/smp_lock.h>
#include <linux/string.h>

#include "../backend.h"
#include "../dvb_module.h"
#include "linux/dvb/stm_ioctls.h"

//Dagobert Hack
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/rawmidi.h>
#include <sound/initval.h>

extern struct snd_kcontrol ** pseudoGetControls(int* numbers);
static int snd_pseudo_integer_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol);
static int snd_pseudo_integer_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol);
#if defined(CONFIG_SH_QBOXHD_1_0) || defined(CONFIG_SH_QBOXHD_MINI_1_0)
int avs_command_kernel(unsigned int cmd, void *arg) {}
#else
extern int avs_command_kernel(unsigned int cmd, void *arg);
#endif

extern int stmfb_set_output_configuration(struct stmfbio_output_configuration *c, struct stmfb_info *i);
extern int stmfb_get_output_configuration(struct stmfbio_output_configuration *c, struct stmfb_info *i);
struct stmfb_info* stmfb_get_fbinfo_ptr();

//stgfb.h
struct stmfbio_output_configuration
{
  __u32 outputid;
  __u32 caps;
  __u32 failed;
  __u32 activate;
  
  __u32 sdtv_encoding;
  __u32 analogue_config;
  __u32 dvo_config;
  __u32 hdmi_config;
  __u32 mixer_background;
  __u8  brightness;
  __u8  saturation;
  __u8  contrast;
  __u8  hue;
};

#define STMFBIO_OUTPUT_CAPS_HDMI_CONFIG      (1L<<3)
#define STMFBIO_OUTPUT_CAPS_ANALOGUE_CONFIG  (1L<<1)
#define STMFBIO_OUTPUT_HDMI_RGB               (0)
#define STMFBIO_OUTPUT_HDMI_YUV               (1L<<1)
#define STMFBIO_OUTPUT_HDMI_444               (0)
#define STMFBIO_OUTPUT_HDMI_422               (1L<<2)

#define STMFBIO_OUTPUT_ANALOGUE_RGB        (1L<<0)
#define STMFBIO_OUTPUT_ANALOGUE_YPrPb      (1L<<1)
#define STMFBIO_OUTPUT_ANALOGUE_YC         (1L<<2)
#define STMFBIO_OUTPUT_ANALOGUE_CVBS       (1L<<3)


#include "../../../../../sound/pseudocard/pseudo_mixer.h"

#define PSEUDO_ADDR(x) (offsetof(struct snd_pseudo_mixer_settings, x))

#include <avs_core.h>

#define AVSIOSET   	 	0x1000
#define AVSIOSTANDBY    (99|AVSIOSET)


extern struct DeviceContext_s* DeviceContext;

static int current_standby = 0;
static int current_input = 0;
#if defined(CUBEREVO) || defined(CUBEREVO_MINI) || defined(CUBEREVO_MINI2) || defined(CUBEREVO_MINI_FTA) || defined(CUBEREVO_250HD) || defined(CUBEREVO_2000HD) || defined(CUBEREVO_9500HD) || defined(TF7700) || defined(UFS922) || defined(HL101)
static int current_volume = 0;
#else
static int current_volume = 31;
#endif

static int current_e2_volume = 31;

int proc_avs_0_volume_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
#define cMaxAttenuationE2			63

   int logarithmicAttenuation[cMaxAttenuationE2] =
	{
		 0,  0,  1,  1,  1,  1,  2,  2,  2,  3,  3,  3,  4,  4,  4,  5,
		 5,  5,  6,  6,  6,  7,  7,  8,  8,  9,  9,  9, 10, 10, 11, 11,
		12, 13, 13, 14, 14, 15, 16, 16, 17, 18, 19, 19, 20, 21, 22, 23,
		24, 25, 27, 28, 29, 31, 33, 35, 37, 40, 43, 47, 51, 58, 70 
	};

	char 		*page;
	char		*myString;
	ssize_t 	ret = -ENOMEM;
	int		result;
	
	printk("%s %d - ", __FUNCTION__, count);

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page) 
	{
		int number = 0;
		struct snd_kcontrol ** kcontrol = pseudoGetControls(&number);
		struct snd_kcontrol *single_control = NULL;
		int vLoop;
		int volume = 0;

		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);

		sscanf(myString, "%d", &volume);

#if defined(CUBEREVO) || defined(CUBEREVO_MINI) || defined(CUBEREVO_MINI2) || defined(CUBEREVO_MINI_FTA) || defined(CUBEREVO_250HD) || defined(CUBEREVO_2000HD) || defined(CUBEREVO_9500HD) || defined(TF7700) || defined(UFS922) || defined(HL101)
      current_volume = volume;
#else
/* Dagobert: 04.10.2009: e2 delivers values from 0 to 63 db. the ak4705 
 * needs values which are a little bit more sophisticated.
 * the range is from mute (value 0) to -60 db (value 1) to +6db (value 34) 
 * which is represented by 6 bits in 2db steps
 * 
 * so we have a value range of 0 - 34. in my opinion the algo must be:
 *
 * current_volume = ((volume * 100) / 63 * 34) / 100;
 * current_volume = 34 - current_volume;
 *
 * mybe someone could test it.
 *
 */

 current_volume = 31 - volume / 4; 
#endif
		
		for (vLoop = 0; vLoop < number; vLoop++)
		{
			if (kcontrol[vLoop]->private_value == PSEUDO_ADDR(master_volume))
			{
				single_control = kcontrol[vLoop];
				//printk("Find master_volume control at %p\n", single_control);
				break;
			}		
		}	

		if ((kcontrol != NULL) && (single_control != NULL))
		{
			struct snd_ctl_elem_value ucontrol;

			current_e2_volume = volume;
			
/* Dagobert: 04.10.2009: e2 delivers values from 0 to 63 db. the current 
 * pseudo_mixer needs a value from 0 to "-70".
 * ->see pseudo_mixer.c line 722.
 */

/* Dagobert: 06.10.2009: Volume is a logarithmical value ...

  scale range 

        volume = ((volume * 100) / cMaxVolumeE2 * cMaxVolumePlayer) / 100;
        volume = 0 - volume;
*/
        volume = 0 - logarithmicAttenuation[current_e2_volume];
		  
		  //printk("Pseudo Mixer controls = %p\n", kcontrol);
		  ucontrol.value.integer.value[0] = volume;
		  ucontrol.value.integer.value[1] = volume;
		  ucontrol.value.integer.value[2] = volume;
		  ucontrol.value.integer.value[3] = volume;
		  ucontrol.value.integer.value[4] = volume;
		  ucontrol.value.integer.value[5] = volume;

		#if defined(CONFIG_SH_QBOXHD_1_0) || defined(CONFIG_SH_QBOXHD_MINI_1_0)
		/* For 'Side Left' and 'Side Right' in pseudo_card 'Master' */
		  ucontrol.value.integer.value[6] = volume;
		  ucontrol.value.integer.value[7] = volume;
		#endif


		  snd_pseudo_integer_put(single_control, &ucontrol);

		} else
		{
			printk("Pseudo Mixer does not deliver controls\n");
		}

		if(current_input == 1)
	  		avs_command_kernel(AVSIOSVOL, current_volume);

		kfree(myString);
	}
	
	ret = count;
out:
	
	free_page((unsigned long)page);
	return ret;
}

int proc_avs_0_volume_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
		int len = 0;
		printk("%s\n", __FUNCTION__);

	   len = sprintf(page, "%d\n", current_e2_volume);

      return len;
}

int proc_avs_0_input_choices_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);

	len = sprintf(page, "encoder scart\n");
        return len;
}

int proc_avs_0_input_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	char		*myString;
	ssize_t 	ret = -ENOMEM;
	int		result;
	
	printk("%s %d - ", __FUNCTION__, count);

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page) 
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);
		
	    	if(!strncmp("encoder", myString, count - 1))
	    	{
			avs_command_kernel(SAAIOSSRCSEL, SAA_SRC_ENC);
#if defined(CUBEREVO) || defined(CUBEREVO_MINI) || defined(CUBEREVO_MINI2) || defined(CUBEREVO_MINI_FTA) || defined(CUBEREVO_250HD) || defined(CUBEREVO_2000HD) || defined(CUBEREVO_9500HD) || defined(TF7700) || defined(UFS922) || defined(HL101)
			avs_command_kernel(AVSIOSVOL, current_volume);
#else
			avs_command_kernel(AVSIOSVOL, 31);
#endif
			current_input = 0;
		}
		
	    	if(!strncmp("scart", myString, count - 1))
	    	{
	      		avs_command_kernel(SAAIOSSRCSEL, SAA_SRC_SCART);
	      		avs_command_kernel(AVSIOSVOL, current_volume);
	      		current_input = 1;
	    	}

		kfree(myString);
		//result = sscanf(page, "%3s %3s %3s %3s %3s", s1, s2, s3, s4, s5);
	}

	ret = count;
out:
	
	free_page((unsigned long)page);
	return ret;
}

int proc_avs_0_input_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);

	if(current_input == 0)
    		len = sprintf(page, "encoder\n");
  	else
    		len = sprintf(page, "scart\n");

        return len;
}

int proc_avs_0_fb_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	char		*myString;
	ssize_t 	ret = -ENOMEM;
	int		result;
	
	printk("%s %d - ", __FUNCTION__, count);

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page) 
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);
		kfree(myString);
		//result = sscanf(page, "%3s %3s %3s %3s %3s", s1, s2, s3, s4, s5);
	}
	
	ret = count;
out:
	
	free_page((unsigned long)page);
	return ret;
}


int proc_avs_0_fb_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);

	len = sprintf(page, "low\n");

        return len;
}

int proc_avs_0_colorformat_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	char		*myString;
	ssize_t 	ret = -ENOMEM;
	int		result;
	
	printk("%s %d - ", __FUNCTION__, count);

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page) 
	{
		void* fb =  stmfb_get_fbinfo_ptr();
		struct fb_info *info = (struct fb_info*) fb;

		struct stmfbio_output_configuration outputConfig;
		int err = 0;
		int alpha = 0;
		int hdmi_colour = 0;
		int scart_colour = 0;
		int hdmi0scart1yuv2 = 1;

		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);
		
		sscanf(myString, "%d", &alpha);

//0rgb 1yuv 2422
		if (strncmp("hdmi_rgb", page, count - 1) == 0)
		{
                        hdmi_colour = 0;
			hdmi0scart1yuv2 = 0;
		} else if (strncmp("hdmi_yuv", page, count - 1) == 0)
		{
                        hdmi_colour = 1;
			hdmi0scart1yuv2 = 0;
		} else if (strncmp("hdmi_422", page, count - 1) == 0)
		{
                        hdmi_colour = 2;
			hdmi0scart1yuv2 = 0;
		} else if (strncmp("rgb", page, count - 1) == 0)
		{
                        scart_colour = SAA_MODE_RGB;
			hdmi0scart1yuv2 = 1;
		} else if (strncmp("cvbs", page, count - 1) == 0)
		{
                        scart_colour = SAA_MODE_FBAS;
			hdmi0scart1yuv2 = 1;
		} else if (strncmp("svideo", page, count - 1) == 0)
		{
                        scart_colour = SAA_MODE_SVIDEO;
			hdmi0scart1yuv2 = 1;
		} else if  (strncmp("yuv", page, count - 1) == 0)
		{
			hdmi0scart1yuv2 = 2;
		}

		if (hdmi0scart1yuv2 == 0) {
			outputConfig.outputid = 1;

			stmfb_get_output_configuration(&outputConfig,info);

			outputConfig.caps = 0;
			outputConfig.activate = 0; //STMFBIO_ACTIVATE_IMMEDIATE;

			outputConfig.caps |= STMFBIO_OUTPUT_CAPS_HDMI_CONFIG;
			outputConfig.hdmi_config &= ~(STMFBIO_OUTPUT_HDMI_YUV|STMFBIO_OUTPUT_HDMI_422);

			switch(hdmi_colour)
			{
				case 1:
					outputConfig.hdmi_config |= STMFBIO_OUTPUT_HDMI_YUV;
					break;
				case 2:
					outputConfig.hdmi_config |= (STMFBIO_OUTPUT_HDMI_YUV|STMFBIO_OUTPUT_HDMI_422);
					break;
				default:
					break;
			}

			err = stmfb_set_output_configuration(&outputConfig, info);
		} else if (hdmi0scart1yuv2 == 1) {
			avs_command_kernel(SAAIOSMODE, scart_colour);

			outputConfig.outputid = 1;

			stmfb_get_output_configuration(&outputConfig,info);

			outputConfig.caps = 0;
			outputConfig.activate = 0; //STMFBIO_ACTIVATE_IMMEDIATE;

			outputConfig.caps |= STMFBIO_OUTPUT_CAPS_ANALOGUE_CONFIG;
			outputConfig.analogue_config = 0;

			switch(scart_colour)
			{
				case SAA_MODE_RGB:
					outputConfig.analogue_config |= (STMFBIO_OUTPUT_ANALOGUE_RGB|STMFBIO_OUTPUT_ANALOGUE_CVBS);
					break;
				case SAA_MODE_FBAS:
					outputConfig.analogue_config |= STMFBIO_OUTPUT_ANALOGUE_CVBS;
					break;
				case SAA_MODE_SVIDEO:
					outputConfig.analogue_config |= STMFBIO_OUTPUT_ANALOGUE_YC;
					break;
				default:
					break;
			}

			err = stmfb_set_output_configuration(&outputConfig, info);
			if (err != 0) {
				printk("SET SCART COLOR - %d - ", count);
			}

		} else {
			outputConfig.outputid = 1;

			stmfb_get_output_configuration(&outputConfig,info);

			outputConfig.caps = 0;
			outputConfig.activate = 0; //STMFBIO_ACTIVATE_IMMEDIATE;

			outputConfig.caps |= STMFBIO_OUTPUT_CAPS_ANALOGUE_CONFIG;
			outputConfig.analogue_config = 0;

			outputConfig.analogue_config |= STMFBIO_OUTPUT_ANALOGUE_YPrPb;

			err = stmfb_set_output_configuration(&outputConfig, info);
			if (err != 0) {
				printk("SET SCART COLOR - %d - ", count);
			}
		}

		//if(ioctl(fbfd, STMFBIO_SET_OUTPUT_CONFIG, &outputConfig)<0)
		//perror("setting output configuration failed");

		kfree(myString);
	}
	
	ret = count;
out:
	
	free_page((unsigned long)page);
	return ret;
}

int proc_avs_0_colorformat_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	void* fb =  stmfb_get_fbinfo_ptr();
	struct fb_info *info = (struct fb_info*) fb;

	struct stmfbio_output_configuration outputConfig;
	int err = 0;
	int alpha = 0;
	int hdmi_colour = 0;
	int len = 0;
	printk("%s %d\n", __FUNCTION__, count);

	outputConfig.outputid = 1;

	stmfb_get_output_configuration(&outputConfig,info);

	if (outputConfig.hdmi_config & STMFBIO_OUTPUT_HDMI_422)
		len = sprintf(page, "hdmi_422\n");
	else if (outputConfig.hdmi_config & STMFBIO_OUTPUT_HDMI_YUV)
		len = sprintf(page, "hdmi_yuv\n");
	else
		len = sprintf(page, "hdmi_rgb\n");

	if (outputConfig.analogue_config & STMFBIO_OUTPUT_ANALOGUE_RGB)
		len += sprintf(page+len, "rgb\n");
	else if (outputConfig.analogue_config & STMFBIO_OUTPUT_ANALOGUE_CVBS)
		len += sprintf(page+len, "cvbs\n");
	else if (outputConfig.analogue_config & STMFBIO_OUTPUT_ANALOGUE_YC)
		len += sprintf(page+len, "svideo\n");
	else if (outputConfig.analogue_config & STMFBIO_OUTPUT_ANALOGUE_YPrPb)
		len += sprintf(page+len, "yuv\n");
	else
		len += sprintf(page+len, "not defined\n");
		
        return len;
}

int proc_avs_0_colorformat_choices_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);

	len = sprintf(page, "rgb cvbs svideo yuv hdmi_rgb hdmi_yuv hdmi_422\n");

        return len;
}

int proc_avs_0_sb_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	char		*myString;
	ssize_t 	ret = -ENOMEM;
	int		result;
	
	printk("%s %d - ", __FUNCTION__, count);

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page) 
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);
		kfree(myString);
		//result = sscanf(page, "%3s %3s %3s %3s %3s", s1, s2, s3, s4, s5);
	}
	
	ret = count;
out:
	
	free_page((unsigned long)page);
	return ret;
}


int proc_avs_0_sb_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);

	len = sprintf(page, "auto\n");

        return len;
}

int proc_avs_0_standby_write(struct file *file, const char __user *buf,
                           unsigned long count, void *data)
{
	char 		*page;
	char		*myString;
	ssize_t 	ret = -ENOMEM;
	int		result;
	
	printk("%s %d - ", __FUNCTION__, count);

	page = (char *)__get_free_page(GFP_KERNEL);
	if (page) 
	{
		ret = -EFAULT;
		if (copy_from_user(page, buf, count))
			goto out;

		myString = (char *) kmalloc(count + 1, GFP_KERNEL);
		strncpy(myString, page, count);
		myString[count] = '\0';

		printk("%s\n", myString);

		if (strncmp("on", page, count - 1) == 0)
		{
                        current_standby = 1;
		} else if (strncmp("off", page, count - 1) == 0)
		{
                        current_standby = 0;
		} 

		avs_command_kernel(AVSIOSTANDBY, current_standby);

		kfree(myString);
		//result = sscanf(page, "%3s %3s %3s %3s %3s", s1, s2, s3, s4, s5);
	}
	
	ret = count;
out:
	
	free_page((unsigned long)page);
	return ret;
}


int proc_avs_0_standby_read (char *page, char **start, off_t off, int count,
			  int *eof, void *data_unused)
{
	int len = 0;
	printk("%s\n", __FUNCTION__);

	if(current_standby == 0)
		len = sprintf(page, "off\n");
	if(current_standby == 1)
		len = sprintf(page, "on\n");
		
        return len;
}
