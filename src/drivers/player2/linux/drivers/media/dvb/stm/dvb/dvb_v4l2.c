/***********************************************************************
 *
 * File: dvb_v4l2.c
 * Copyright (c) 2007 STMicroelectronics Limited.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * Implementation of linux dvb v4l2 control device
 *
\***********************************************************************/

/******************************
 * INCLUDES
 ******************************/
#include <asm/semaphore.h>
#include <asm/page.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/pgtable.h>

#include <linux/module.h>
#include <linux/dvb/audio.h>
#include <linux/dvb/video.h>
#include <linux/dvb/dmx.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/stm/stm_v4l2.h>
#include <linux/ioport.h>
#include <linux/bpa2.h>
#include <linux/delay.h>

#include <include/stmdisplay.h>
#include <linux/stm/stmcoredisplay.h>
#include <stmdisplayplane.h>
#include <stmdisplayblitter.h>
#include <stmvout.h>


#include "dvb_module.h"
#include "backend.h"
#include "dvb_v4l2.h"

#define V4L2_CID_STM_BLANK		(V4L2_CID_PRIVATE_BASE+100)

#if defined(CONFIG_SH_QBOXHD_1_0) || defined(CONFIG_SH_QBOXHD_MINI_1_0)
extern struct DvbContext_s*     DvbContextArray[3];
#else
extern struct DvbContext_s *DvbContext;
#endif

struct linuxdvb_v4l2 {
	struct v4l2_input 			input;
	struct v4l2_crop 			crop;
	struct v4l2_buffer 			buffer[MAX_BUFFERS];
	void              			*address[MAX_BUFFERS];
	struct ldvb_v4l2_capture 	*capture;
	unsigned int 				blank;
};

struct bpa2_part     *partition = NULL;

typedef struct {
  struct v4l2_fmtdesc fmt;
  int    depth;
} format_info;

static format_info stmfb_v4l2_mapping_info [] =
{
  /*
   * This isn't strictly correct as the V4L RGB565 format has red
   * starting at the least significant bit. Unfortunately there is no V4L2 16bit
   * format with blue starting at the LSB. It is recognised in the V4L2 API
   * documentation that this is strange and that drivers may well lie for
   * pragmatic reasons.
   */
  [SURF_RGB565]     = {{ 0, V4L2_BUF_TYPE_VIDEO_CAPTURE, 0,
			 "RGB-16 (5-6-5)", V4L2_PIX_FMT_RGB565 },    16},

  [SURF_ARGB1555]     = {{ 0, V4L2_BUF_TYPE_VIDEO_CAPTURE, 0,
			 "RGBA-16 (5-5-5-1)", V4L2_PIX_FMT_BGRA5551 }, 16},

  [SURF_ARGB4444]     = {{ 0, V4L2_BUF_TYPE_VIDEO_CAPTURE, 0,
			 "RGBA-16 (4-4-4-4)", V4L2_PIX_FMT_BGRA4444 }, 16},

  [SURF_RGB888]     = {{ 0, V4L2_BUF_TYPE_VIDEO_CAPTURE, 0,
			 "RGB-24 (B-G-R)", V4L2_PIX_FMT_BGR24 },     24},

  [SURF_ARGB8888]   = {{ 0, V4L2_BUF_TYPE_VIDEO_CAPTURE, 0,
			 "ARGB-32 (8-8-8-8)", V4L2_PIX_FMT_BGR32 },  32}, /* Note that V4L2 doesn't define
									   * the alpha channel
									   */

  [SURF_BGRA8888]   = {{ 0, V4L2_BUF_TYPE_VIDEO_CAPTURE, 0,
			 "BGRA-32 (8-8-8-8)", V4L2_PIX_FMT_RGB32 },  32}, /* Bigendian BGR as BTTV driver,
									   * not as V4L2 spec
									   */

  [SURF_YCBCR422R]  = {{ 0, V4L2_BUF_TYPE_VIDEO_CAPTURE, 0,
			 "YUV 4:2:2 (U-Y-V-Y)", V4L2_PIX_FMT_UYVY }, 16},

  [SURF_YUYV]       = {{ 0, V4L2_BUF_TYPE_VIDEO_CAPTURE, 0,
			 "YUV 4:2:2 (Y-U-Y-V)", V4L2_PIX_FMT_YUYV }, 16},

  [SURF_YCBCR422MB] = {{ 0, V4L2_BUF_TYPE_VIDEO_CAPTURE, 0,
			 "YUV 4:2:2MB", V4L2_PIX_FMT_STM422MB },     8}, /* Bits per pixel for Luma only */

  [SURF_YCBCR420MB] = {{ 0, V4L2_BUF_TYPE_VIDEO_CAPTURE, 0,
			 "YUV 4:2:0MB", V4L2_PIX_FMT_STM420MB },     8}, /* Bits per pixel for Luma only */

  [SURF_YUV420]     = {{ 0, V4L2_BUF_TYPE_VIDEO_CAPTURE, 0,
			 "YUV 4:2:0 (YUV)", V4L2_PIX_FMT_YUV420 },   8}, /* Bits per pixel for Luma only */

  [SURF_YVU420]     = {{ 0, V4L2_BUF_TYPE_VIDEO_CAPTURE, 0,
			 "YUV 4:2:0 (YVU)", V4L2_PIX_FMT_YVU420 },   8}, /* Bits per pixel for Luma only */

  [SURF_YUV422P]    = {{ 0, V4L2_BUF_TYPE_VIDEO_CAPTURE, 0,
			 "YUV 4:2:2 (YUV)", V4L2_PIX_FMT_YUV422P },  8}, /* Bits per pixel for Luma only */

  /* Make sure the array covers all the SURF_FMT enumerations  */
  [SURF_END]         = {{ 0, 0, 0, "", 0 }, 0 }
};


unsigned long GetPhysicalContiguous(unsigned long ptr,size_t size)
{
	struct mm_struct *mm = current->mm;
	//struct vma_area_struct *vma = find_vma(mm, ptr);
	unsigned virt_base =  (ptr / PAGE_SIZE) * PAGE_SIZE;
	unsigned phys_base = 0;

	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *ptep, pte;

	spin_lock(&mm->page_table_lock);

	pgd = pgd_offset(mm, virt_base);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
		goto out;

	pmd = pmd_offset((pud_t*)pgd, virt_base);
	if (pmd_none(*pmd) || pmd_bad(*pmd))
		goto out;

	ptep = pte_offset_map(pmd, virt_base);

	if (!ptep)
		goto out;

	pte = *ptep;

	if (pte_present(pte)) {
		phys_base = __pa(page_address(pte_page(pte)));
	}

	if (!phys_base)
		goto out;

	spin_unlock(&mm->page_table_lock);
	return phys_base + (ptr - virt_base);

out:
	spin_unlock(&mm->page_table_lock);
	return 0;
}

void* stm_v4l2_findbuffer(unsigned long userptr, unsigned int size,int device)
{
	int i;
	unsigned long result;
	unsigned long virtual;

	if (!g_ldvb_v4l2_device[device].inuse)
		return 0;

	result = GetPhysicalContiguous(userptr,size);

	virtual = (unsigned long)phys_to_virt(result);

	if (result) {
		struct linuxdvb_v4l2 *ldvb = g_ldvb_v4l2_device[device].priv;

		for (i=0;i<MAX_BUFFERS;i++)
		{
			if (ldvb->buffer[i].length &&
			    (result >= ldvb->buffer[i].m.offset) &&
			    ((result + size) < (ldvb->buffer[i].m.offset + ldvb->buffer[i].length)))
				return (void*)virtual;
		}
	}

	return 0;
}

stm_display_buffer_t*       ManifestorLastDisplayedBuffer;
EXPORT_SYMBOL(ManifestorLastDisplayedBuffer);

static int linuxdvb_v4l2_capture_thread( struct linuxdvb_v4l2 *ldvb )
{
	stm_display_buffer_t       buffer;
	struct stmcore_display_pipeline_data     pipeline_data;
	stm_blitter_operation_t op;
	stm_rect_t              dstrect;
	stm_rect_t              srcrect;

	memset(&op,0,sizeof(op));

	srcrect.top  = 0;
	srcrect.left = 0;

	dstrect.top  = 0;
	dstrect.left = 0;

	// Get a handle to the blitter
	stmcore_get_display_pipeline (0, &pipeline_data);

	while (ldvb->capture && readl(&ldvb->capture->running)) {
		stm_display_buffer_t*       ptr = (stm_display_buffer_t*) readl(&ManifestorLastDisplayedBuffer);

		if (ldvb->capture->physical_address && ldvb->capture->complete == 0 && ptr) {
			memcpy(&buffer,ptr,sizeof(stm_display_buffer_t));

#if 0
			printk("%s:%d  %d %d %d %d\n",__FUNCTION__,__LINE__,
			       ldvb->capture->width,
			       ldvb->capture->height,
			       ldvb->capture->stride,
			       ldvb->capture->size);
#endif

			dstrect.right  = ldvb->capture->width  - 1;
			dstrect.bottom = ldvb->capture->height - 1;

			srcrect.right  = buffer.src.Rect.width - 1;
			srcrect.bottom = buffer.src.Rect.height - 1;

			op.ulFlags = 0;
			op.srcSurface.ulMemory       = buffer.src.ulVideoBufferAddr;
			op.srcSurface.ulSize         = buffer.src.ulVideoBufferSize;
			op.srcSurface.ulWidth        = buffer.src.Rect.width;
			op.srcSurface.ulHeight       = buffer.src.Rect.height;
			op.srcSurface.ulStride       = buffer.src.ulStride;
			op.srcSurface.format         = buffer.src.ulColorFmt;
//			op.srcSurface.ulChromaOffset = buffer.src.chromaBufferOffset;

			op.dstSurface.ulMemory = ldvb->capture->physical_address;
			op.dstSurface.ulSize   = ldvb->capture->size;
			op.dstSurface.ulWidth  = ldvb->capture->width;
			op.dstSurface.ulHeight = ldvb->capture->height;
			op.dstSurface.ulStride = ldvb->capture->stride;
			op.dstSurface.format   = ldvb->capture->buffer_format;

			if (stm_display_blitter_blit(pipeline_data.blitter, &op, &srcrect, &dstrect))
				printk("%s:%d Error during blitter operation\n",__FUNCTION__,__LINE__);

			ldvb->capture->complete = 1;
		}

		mdelay(32); // Sleep for every 32 milliseconds
	}

	return 0;
}

int linuxdvb_ioctl(struct stm_v4l2_handles *handle,struct stm_v4l2_driver *driver, int device, int type, struct file *file, unsigned int cmd, void *arg)
{
	struct linuxdvb_v4l2 *ldvb = handle->v4l2type[type].handle;
#if defined(CONFIG_SH_QBOXHD_1_0) || defined(CONFIG_SH_QBOXHD_MINI_1_0)
	struct DvbContext_s *DvbContext = DvbContextArray[0];

	printk("-----> %s(): Aqui me atora: DvbContext[0]!!!!!!!!\n", __func__);
#endif

	// Need a mutex here

	switch(cmd)
	{
	case VIDIOC_ENUMINPUT:
	{
		struct v4l2_input* input = arg;

		// check consistency of index
		if (input->index < (0+driver->start_index[device]) || input->index >= (g_ldvb_v4l2Count + driver->start_index[device]))
			goto err_inval;

		strcpy(input->name, g_ldvb_v4l2_device[input->index - driver->start_index[device]].name);

		break;
	}

	case VIDIOC_S_INPUT:
	{
		int id = *((unsigned int*)arg);

		id -= driver->start_index[device];

		// check consistency of index
		if (id < 0 || id >= g_ldvb_v4l2Count)
		{
			//DVB_ERROR("index out of bounds (%d >= %d)\n",id,driver->start_index[device]);
			goto err_inval;
		}

		// check if resource already in use
		if (g_ldvb_v4l2_device[id].inuse)
		{
			DVB_ERROR("Device already in use \n");
			goto err_inval;
		}

		// allocate handle for driver registration
		handle->v4l2type[type].handle = kmalloc(sizeof(struct linuxdvb_v4l2),GFP_KERNEL);
		if (!handle->v4l2type[type].handle)
		{
			DVB_ERROR("kmalloc failed\n");
			return -ENODEV;
		}

		ldvb = handle->v4l2type[type].handle;
		memset(ldvb,0,sizeof(struct linuxdvb_v4l2));

		ldvb->input.index = id;

		g_ldvb_v4l2_device[id].inuse = 1;
		g_ldvb_v4l2_device[id].priv = ldvb;

		break;
	}

	case VIDIOC_G_INPUT:
	{
		if (ldvb == NULL) {
			DVB_ERROR("driver handle NULL. Need to call VIDIOC_S_INPUT first. \n");
			return -ENODEV;
		}

		*((unsigned int*)arg) = ldvb->input.index;

		break;
	}

	case VIDIOC_S_CROP:
	{
		struct v4l2_crop *crop = (struct v4l2_crop*)arg;

		if (ldvb == NULL) {
			DVB_ERROR("driver handle NULL. Need to call VIDIOC_S_INPUT first. \n");
			return -ENODEV;
		}

		if (!crop->type) {
			DVB_ERROR("crop->type = %d\n", crop->type);
			return -EINVAL;
		}

		// get crop values
		ldvb->crop.type = crop->type;
		ldvb->crop.c = crop->c;


		if (crop->type == V4L2_BUF_TYPE_VIDEO_OVERLAY)
			VideoSetOutputWindow (&DvbContext->DeviceContext[ldvb->input.index],
					      crop->c.left, crop->c.top, crop->c.width, crop->c.height);
		else if (crop->type == V4L2_BUF_TYPE_PRIVATE+1)
			VideoSetInputWindow (&DvbContext->DeviceContext[ldvb->input.index],
					     crop->c.left, crop->c.top, crop->c.width, crop->c.height);

		break;
	}

	case VIDIOC_CROPCAP:
	{
		struct v4l2_cropcap *cropcap = (struct v4l2_cropcap*)arg;
		video_size_t         video_size;

		if (ldvb == NULL) {
			printk("%s Error: driver handle NULL. Need to call VIDIOC_S_INPUT first. \n", __FUNCTION__);
			return -ENODEV;
		}

		if (cropcap->type == V4L2_BUF_TYPE_PRIVATE+1) {
			VideoIoctlGetSize (&DvbContext->DeviceContext[ldvb->input.index], &video_size);
			cropcap->bounds.left                    = 0;
			cropcap->bounds.top                     = 0;
			cropcap->bounds.width                   = video_size.w;
			cropcap->bounds.height                  = video_size.h;

			VideoGetPixelAspectRatio (&DvbContext->DeviceContext[ldvb->input.index],
						  &cropcap->pixelaspect.numerator, &cropcap->pixelaspect.denominator);

			printk("%s VIDIOC_CROPCAP, type = %d\n", __FUNCTION__, cropcap->type);
		}
		break;
	}

	case VIDIOC_QUERYBUF:
	{
		struct v4l2_buffer *buf = arg;

		if (ldvb == NULL) {
			DVB_ERROR("driver handle NULL. Need to call VIDIOC_S_INPUT first. \n");
			return -ENODEV;
		}

		if (buf->index > MAX_BUFFERS || buf->index < 0)
			goto err_inval;

		if (ldvb->buffer[buf->index].length == 0)
			goto err_inval;

		*buf = ldvb->buffer[buf->index];

		break;
	}

	case VIDIOC_STREAMON:
	{
		char task_name[] = "v4l2_dvp capture";

		if (!ldvb)
			goto err_inval;

		if (!ldvb->capture)
			goto err_inval;

		if (!ldvb->capture->buffer_format)
			goto err_inval;

		if (ldvb->capture->thread)
			goto err_inval;

		ldvb->capture->running = 1;
		ldvb->capture->thread = kthread_create( (void *)linuxdvb_v4l2_capture_thread, ldvb, "%s", task_name );
		wake_up_process( ldvb->capture->thread );

		break;
	}

	case VIDIOC_STREAMOFF:
	{
		if (!ldvb)
			goto err_inval;

		if (!ldvb->capture)
			goto err_inval;

		if (!ldvb->capture->thread)
			goto err_inval;

		ldvb->capture->running = 0;
		ldvb->capture->thread  = NULL;

		break;
	}

	case VIDIOC_S_FMT:
	{
		struct v4l2_format *fmt = arg;
		int n,surface = -1;;

		if (!ldvb)
			goto err_inval;

		if (!fmt)
			goto err_inval;

		if (fmt->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
			goto err_inval;

		if (!ldvb->capture) {
			ldvb->capture = kmalloc(sizeof(struct ldvb_v4l2_capture),GFP_KERNEL);
			memset(ldvb->capture,0,sizeof(struct ldvb_v4l2_capture));
		}

		for (n=0;n<sizeof(stmfb_v4l2_mapping_info)/sizeof(format_info);n++)
			if (stmfb_v4l2_mapping_info[n].fmt.pixelformat == fmt->fmt.pix.pixelformat)
				surface = n;

		if (surface == -1) {
			// Not sure if freeing this is correct..
			kfree(ldvb->capture);
			ldvb->capture = NULL;
			goto err_inval;
		}

		ldvb->capture->buffer_format = surface;
		ldvb->capture->width         = fmt->fmt.pix.width;
		ldvb->capture->height        = fmt->fmt.pix.height;
		if (!fmt->fmt.pix.bytesperline)
			fmt->fmt.pix.bytesperline = (stmfb_v4l2_mapping_info[surface].depth * fmt->fmt.pix.width) / 8;

		ldvb->capture->stride = fmt->fmt.pix.bytesperline;

		break;
	}

	case VIDIOC_QBUF:
	{
		struct v4l2_buffer *buf = arg;
		unsigned long addr 		= 0;

		if (!ldvb)
			goto err_inval;
		if (!ldvb->capture)
			goto err_inval;
		if (!buf)
			goto err_inval;
		if (buf->memory != V4L2_MEMORY_USERPTR)
			goto err_inval;

		// Currently we support capture of only one buffer
		// at a time, anything else doesn't make sense.
		if (ldvb->capture->physical_address)
			return -EIO;

		addr = GetPhysicalContiguous(buf->m.userptr,buf->length);

		if (!addr) return -EIO;

		ldvb->capture->physical_address = addr;
		ldvb->capture->size             = buf->length;

		break;
	}

	case VIDIOC_DQBUF:
	{
		//struct v4l2_buffer *buf = arg;

		if (!arg)
			goto err_inval;

		if (!ldvb)
			goto err_inval;

		if (!ldvb->capture)
			goto err_inval;

		if (!ldvb->capture->physical_address) // If there is no physical address
			if (!ldvb->capture->complete) // And we are not complete
				return -EIO;          // Return an IO error, plese queue a buffer...

		// Otherwise loop until the blit has been completed
		while (readl(&ldvb->capture->complete)==0)
			mdelay(10);

		ldvb->capture->physical_address = 0;           // Mark as done, so we can queue a new buffer
		ldvb->capture->complete         = 0;

		break;
	}

	// We use the buf type private to allow obtaining physically contiguous buffers.
	// We also need this to get capture buffers so we can do capture of mpeg2 stream etc.
	case VIDIOC_REQBUFS:          //_IOWR ('V',  8, struct v4l2_requestbuffers)
	{
		struct v4l2_requestbuffers *rb = arg;
		int n,m,count;
		void *data;
		unsigned int dataSize;

		if (ldvb == NULL) {
			DVB_ERROR("driver handle NULL. Need to call VIDIOC_S_INPUT first. \n");
			return -ENODEV;
		}

		switch (rb->type)
		{
			case V4L2_BUF_TYPE_VIDEO_CAPTURE:
			{
				if (rb->memory != V4L2_MEMORY_USERPTR)
					goto err_inval;

				if (!ldvb->capture)
					return -EIO;

				break;
			}

			case V4L2_BUF_TYPE_PRIVATE:
			{
				/* Private only supports MMAP buffers */
				if (rb->memory != V4L2_MEMORY_MMAP)
					goto err_inval;

				/* Reserved field is used for size of buffer */
				if (rb->reserved[0] == 0)
					goto err_inval;

				/* Injitially we can give zero buffers */
				count = rb->count;
				rb->count = 0;

				/* Go through the count and see how many buffer we can give */
				for(m=0;m<count;m++)
				{
					/* See if we have any free */
					for (n=0;n<MAX_BUFFERS;n++)
						if (ldvb->buffer[n].length == 0) break;

					/* If not return IO error */
					if (n == MAX_BUFFERS) {
						if (m==0) return -EIO;
						else return 0;
					}

					partition = bpa2_find_part("v4l2-coded-video-buffers");

					/* fallback to big phys area */
					if (partition == NULL)
						partition = bpa2_find_part("bigphysarea");

					if (partition == NULL)
					{
						DVB_ERROR("Failed to find any suitable bpa2 partitions while trying to allocate memory\n");
						return -ENOMEM;
					}

					dataSize = (rb->reserved[0] + (PAGE_SIZE-1)) / PAGE_SIZE ;

					/* Let's see if we can allocate some memory */
					data = (void*)bpa2_alloc_pages(partition,dataSize,4,GFP_KERNEL);

					if (!data) {
						if (m==0) return -EIO;
						else return 0;
					}

					/* Now we know everything good fill the info in */
					ldvb->buffer[n].index     = n;
					ldvb->buffer[n].length    = rb->reserved[0];
					ldvb->buffer[n].m.offset  = (unsigned int)data;
					ldvb->buffer[n].type      = rb->type;
					ldvb->address[n]          = ioremap_cache((unsigned int)data,dataSize);

					rb->count = m+1;
				}

				break;
			}

			default:
				return -EINVAL;
		};

		break;
	}

	case VIDIOC_S_CTRL:
	{
	    ULONG  ctrlid 				= 0;
	    ULONG  ctrlvalue 			= 0;
	    struct v4l2_control* pctrl 	= arg;
	    int ret 					= 0;

	    ctrlid		= pctrl->id;
	    ctrlvalue 	= pctrl->value;

	    switch (ctrlid)
	    {
	        case V4L2_CID_STM_BLANK:
	        {
	    		if (ldvb == NULL) {
	    			DVB_ERROR("driver handle NULL. Need to call VIDIOC_S_INPUT first. \n");
	    			return -ENODEV;
	    		}

	    		ret = StreamEnable (DvbContext->DeviceContext[ldvb->input.index].VideoStream, ctrlvalue);
	    		if( ret < 0 )
	    		{
	    		    DVB_ERROR( "StreamEnable failed (ctrlvalue = %lu)\n", ctrlvalue);
	    		    return -EINVAL;
	    		}

	    		ldvb->blank	= ctrlvalue;

	    		break;
	        }

	        /*
		    case V4L2_CID_STM_SRC_COLOUR_MODE:
		    case V4L2_CID_STM_FULL_RANGE :
		    case V4L2_CID_AUDIO_MUTE:
		    case V4L2_CID_STM_AUDIO_SAMPLE_RATE:
		    case V4L2_CID_STM_BACKGROUND_RED:
		    case V4L2_CID_STM_BACKGROUND_GREEN:
		    case V4L2_CID_STM_BACKGROUND_BLUE:
	         */

	        default:
	        {
	        	DVB_ERROR("Control Id not handled. \n");
	        	return -ENODEV;
	        }
	    }

	    break;
	}

	case VIDIOC_G_CTRL:
	{
	    struct v4l2_control* pctrl = arg;

	    switch (pctrl->id)
	    {
	        case V4L2_CID_STM_BLANK:
	            pctrl->value = ldvb->blank;

	        default:
	        {
	        	DVB_ERROR("Control Id not handled. \n");
	        	return -ENODEV;
	        }
	    }

	    break;
	}


	default:
		return -ENODEV;
	}

	return 0;

err_inval:
	return -EINVAL;
}


static int linuxdvb_close(struct stm_v4l2_handles *handle, int type, struct file *file)
{
	int i;
	struct linuxdvb_v4l2 *ldvb = handle->v4l2type[type].handle;

	if (!ldvb) return 0;

	if (partition)
		for (i=0;i<MAX_BUFFERS;i++)
			if (ldvb->buffer[i].length && ldvb->buffer[i].m.offset)
				bpa2_free_pages(partition,ldvb->buffer[i].m.offset);

	g_ldvb_v4l2_device[ldvb->input.index].inuse = 0;

	if (ldvb->capture) {

		// Need to ensure thread has been stopped
		ldvb->capture->running = 0;
		mdelay(10);
		kfree(ldvb->capture);
		ldvb->capture = NULL;
	}

	handle->v4l2type[type].handle = NULL;
	kfree(ldvb);

	return 0;
}

static struct page* linuxdvb_vm_nopage(struct vm_area_struct *vma, unsigned long vaddr, int *type)
{
	struct page *page;
	void *page_addr;
	unsigned long page_frame;

	if (vaddr > vma->vm_end)
		return NOPAGE_SIGBUS;

	/*
	 * Note that this assumes an identity mapping between the page offset and
	 * the pfn of the physical address to be mapped. This will get more complex
	 * when the 32bit SH4 address space becomes available.
	 */
	page_addr = (void*)((vaddr - vma->vm_start) + (vma->vm_pgoff << PAGE_SHIFT));

	page_frame = ((unsigned long)page_addr >> PAGE_SHIFT);

	if(!pfn_valid(page_frame))
		return NOPAGE_SIGBUS;

	page = virt_to_page(__va(page_addr));

	get_page(page);

	if (type)
		*type = VM_FAULT_MINOR;
	return page;
}

static void linuxdvb_vm_open(struct vm_area_struct *vma)
{
	//DVB_DEBUG("/n");
}

static void linuxdvb_vm_close(struct vm_area_struct *vma)
{
	//DVB_DEBUG("/n");
}

static struct vm_operations_struct linuxdvb_vm_ops_memory =
{
	.open     = linuxdvb_vm_open,
	.close    = linuxdvb_vm_close,
	.nopage   = linuxdvb_vm_nopage,
};

static int linuxdvb_mmap(struct stm_v4l2_handles *handle, int type, struct file *file, struct vm_area_struct*  vma)
{
	struct linuxdvb_v4l2 *ldvb = handle->v4l2type[type].handle;
	int n;

	if (!(vma->vm_flags & VM_WRITE))
		return -EINVAL;

//  if (!(vma->vm_flags & VM_SHARED))
//    return -EINVAL;

	for (n=0;n<MAX_BUFFERS;n++)
		if (ldvb->buffer[n].length && (ldvb->buffer[n].m.offset == (vma->vm_pgoff*PAGE_SIZE)))
			break;

	if (n==MAX_BUFFERS)
		return -EINVAL;

	if (ldvb->buffer[n].length != (vma->vm_end - vma->vm_start))
		return -EINVAL;

	vma->vm_flags       |= /*VM_RESERVED | VM_IO |*/ VM_DONTEXPAND | VM_LOCKED;
//  vma->vm_page_prot    = pgprot_noncached(vma->vm_page_prot);
	vma->vm_private_data = ldvb;

	vma->vm_ops = &linuxdvb_vm_ops_memory;

	return 0;
}

struct stm_v4l2_driver linuxdvb_overlay = {
	.type         = STM_V4L2_VIDEO_INPUT,
	.ioctl        = linuxdvb_ioctl,
	.close        = linuxdvb_close,
	.poll         = NULL,
	.mmap         = linuxdvb_mmap,
};

void linuxdvb_v4l2_init(void)
{
	int i;

	for (i=0;i<STM_V4L2_MAX_DEVICES;i++) {
		linuxdvb_overlay.number_controls[i] = 0;
		linuxdvb_overlay.number_indexs[i]   = 3;
	}

	stm_v4l2_register_driver(&linuxdvb_overlay);
}

