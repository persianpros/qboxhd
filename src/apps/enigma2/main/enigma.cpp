#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <libsig_comp.h>

#include <lib/actions/action.h>
#include <lib/driver/rc.h>
#include <lib/base/ioprio.h>
#include <lib/base/ebase.h>
#include <lib/base/eerror.h>
#include <lib/base/init.h>
#include <lib/base/init_num.h>
#include <lib/gdi/gfbdc.h>
#include <lib/gdi/glcddc.h>
#include <lib/gdi/grc.h>
#ifdef QBOXHD
#ifdef QBOXHD_MINI
	#include <lib/gdi/lpcqboxmini.h>
#else
	#include <lib/gdi/sensewheel.h>
#endif
#endif
#ifdef WITH_SDL
#include <lib/gdi/sdl.h>
#endif
#include <lib/gdi/epng.h>
#include <lib/gdi/font.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/elabel.h>
#include <lib/gui/elistboxcontent.h>
#include <lib/gui/ewidget.h>
#include <lib/gui/ewidgetdesktop.h>
#include <lib/gui/ewindow.h>
#include <lib/python/connections.h>
#include <lib/python/python.h>

#include "bsod.h"

#ifdef HAVE_GSTREAMER
#include <gst/gst.h>
#endif

#ifdef OBJECT_DEBUG
int object_total_remaining;

void object_dump()
{
	printf("%d items left\n", object_total_remaining);
}
#endif

static eWidgetDesktop *wdsk, *lcddsk;
#ifdef QBOXHD
#ifdef QBOXHD_MINI
static eQBOXFrontButton *frontbutton;
#else
static eQBOXSenseWheel *sensewheel;
#endif
#endif
static int prev_ascii_code;

int getPrevAsciiCode()
{
	int ret = prev_ascii_code;
	prev_ascii_code = 0;
	return ret;
}

void keyEvent(const eRCKey &key)
{
	static eRCKey last(0, 0, 0);
	static int num_repeat;

	ePtr<eActionMap> ptr;
	eActionMap::getInstance(ptr);

	if ((key.code == last.code) && (key.producer == last.producer) && key.flags & eRCKey::flagRepeat)
		num_repeat++;
	else
	{
		num_repeat = 0;
		last = key;
	}

	if (num_repeat == 4)
	{
#ifdef QBOXHD
		ptr->keyPressed(key.producer->getIdentifier(), key.producer->getRCIdentifier(), key.code, eRCKey::flagLong);
#else
		ptr->keyPressed(key.producer->getIdentifier(), key.code, eRCKey::flagLong);
#endif
		num_repeat++;
	}

	if (key.flags & eRCKey::flagAscii)
	{
		prev_ascii_code = key.code;
#ifdef QBOXHD
		ptr->keyPressed(key.producer->getIdentifier(), key.producer->getRCIdentifier(), 510 /* faked KEY_ASCII */, 0);
#else
		ptr->keyPressed(key.producer->getIdentifier(), 510 /* faked KEY_ASCII */, 0);
#endif
	}
	else {
#ifdef QBOXHD
		ptr->keyPressed(key.producer->getIdentifier(), key.producer->getRCIdentifier(), key.code, key.flags);
#else
		ptr->keyPressed(key.producer->getIdentifier(), key.code, key.flags);
#endif
	}
}

/************************************************/
#include <unistd.h>
#include <lib/components/scan.h>
#include <lib/dvb/idvb.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/db.h>
#include <lib/dvb/dvbtime.h>
#include <lib/dvb/epgcache.h>

class eMain: public eApplication, public Object
{
	eInit init;

	ePtr<eDVBDB> m_dvbdb;
	ePtr<eDVBResourceManager> m_mgr;
	ePtr<eDVBLocalTimeHandler> m_locale_time_handler;
	ePtr<eEPGCache> m_epgcache;

public:
	eMain()
	{
		init.setRunlevel(eAutoInitNumbers::main);
		/* TODO: put into init */
		m_dvbdb = new eDVBDB();
		m_mgr = new eDVBResourceManager();
		m_locale_time_handler = new eDVBLocalTimeHandler();
		m_epgcache = new eEPGCache();
		m_mgr->setChannelList(m_dvbdb);
	}

	~eMain()
	{
		m_dvbdb->saveServicelist();
		m_mgr->releaseCachedChannel();
	}
};

int exit_code;

int main(int argc, char **argv)
{
#ifdef MEMLEAK_CHECK
	atexit(DumpUnfreed);
#endif

#ifdef OBJECT_DEBUG
	atexit(object_dump);
#endif

#ifdef HAVE_GSTREAMER
	gst_init(&argc, &argv);
#endif

	// set pythonpath if unset
	setenv("PYTHONPATH", LIBDIR "/enigma2/python", 0);
	printf("PYTHONPATH: %s\n", getenv("PYTHONPATH"));

	bsodLogInit();

	ePython python;
	eMain main;

#if 1
#ifdef WITH_SDL
	ePtr<gSDLDC> my_dc;
	gSDLDC::getInstance(my_dc);
#else
	ePtr<gFBDC> my_dc;
	gFBDC::getInstance(my_dc);

	int double_buffer = my_dc->haveDoubleBuffering();
#endif

	ePtr<gLCDDC> my_lcd_dc;
	gLCDDC::getInstance(my_lcd_dc);


		/* ok, this is currently hardcoded for arabic. */
			/* some characters are wrong in the regular font, force them to use the replacement font */
	for (int i = 0x60c; i <= 0x66d; ++i)
		eTextPara::forceReplacementGlyph(i);
	eTextPara::forceReplacementGlyph(0xfdf2);
	for (int i = 0xfe80; i < 0xff00; ++i)
		eTextPara::forceReplacementGlyph(i);

#ifdef QBOXHD
	/* IVAN 2009_09_18 */
	unsigned int xres, yres, bpp;
	/* Read from FrameBuffer the resolution */
	if (my_dc->fb->getfbResolution( &xres, &yres, &bpp) < 0)
		eFatal("Framebuffer Error");

	eWidgetDesktop dsk(eSize(xres, yres));
	/* IVAN 2009_09_18 */
// 	eWidgetDesktop dsk(eSize(720, 576));
	eWidgetDesktop dsk_lcd(eSize(DISPLAY_WIDTH, DISPLAY_HEIGHT));
#else
	eWidgetDesktop dsk(eSize(720, 576));
	eWidgetDesktop dsk_lcd(eSize(132, 64));
#endif

	dsk.setStyleID(0);
	dsk_lcd.setStyleID(1);

/*
	if (double_buffer)
	{
		eDebug(" - double buffering found, enable buffered graphics mode.");
		dsk.setCompositionMode(eWidgetDesktop::cmBuffered);
	}
*/

	wdsk = &dsk;
	lcddsk = &dsk_lcd;

	dsk.setDC(my_dc);
	dsk_lcd.setDC(my_lcd_dc);

	ePtr<gPixmap> m_pm;
	loadPNG(m_pm, DATADIR "/enigma2/skin_default/pal.png");
	if (!m_pm)
	{
		eFatal("pal.png not found!");
	} else
		dsk.setPalette(*m_pm);

	dsk.setBackgroundColor(gRGB(0,0,0,0xFF));
#endif

		/* redrawing is done in an idle-timer, so we have to set the context */
	dsk.setRedrawTask(main);
	dsk_lcd.setRedrawTask(main);


	eDebug("Loading spinners...");

	{
		int i;
#define MAX_SPINNER 64
		ePtr<gPixmap> wait[MAX_SPINNER];
		for (i=0; i<MAX_SPINNER; ++i)
		{
#ifdef QBOXHD
			char filename[strlen(DATADIR) + 41];
#else
			char filename[strlen(DATADIR) + 20];
#endif ///QBOXHD
			sprintf(filename, DATADIR "/enigma2/skin_default/spinner/wait%d.png", i + 1);

			loadPNG(wait[i], filename);

			if (!wait[i])
			{
				if (!i)
					eDebug("failed to load %s! (%m)", filename);
				else
					eDebug("found %d spinner!\n", i);
				break;
			}
		}
		if (i)
			my_dc->setSpinner(eRect(ePoint(100, 100), wait[0]->size()), wait, i);
		else
			my_dc->setSpinner(eRect(100, 100, 0, 0), wait, 1);
	}

	gRC::getInstance()->setSpinnerDC(my_dc);

	eRCInput::getInstance()->keyEvent.connect(slot(keyEvent));

	printf("executing main\n");

#ifdef QBOXHD
#ifdef QBOXHD_MINI
	/* FrontButton*/
	frontbutton = new eQBOXFrontButton();
#else
	/* SenseWheel*/
	sensewheel = new eQBOXSenseWheel();
#endif
#endif ///QBOXHD

	bsodCatchSignals();

	setIoPrio(IOPRIO_CLASS_BE, 3);

//	python.execute("mytest", "__main__");
#ifdef QBOXHD
    python.execFile("/usr/local/lib/enigma2/python/mytest.py");
#else
	python.execFile("/usr/lib/enigma2/python/mytest.py");
#endif // QBOXHD

	extern void setFullsize(); // definend in lib/gui/evideo.cpp
	setFullsize();

	if (exit_code == 5) /* python crash */
	{
		eDebug("(exit code 5)");
		bsodFatal(0);
	}

#ifdef QBOXHD
	if (exit_code == 6) /* terminated by signal */
	{
		eDebug("(exit code 6)");
		bsodFatal("enigma2, signal");
	}
#endif

	dsk.paint();
	dsk_lcd.paint();

	{
		gPainter p(my_lcd_dc);
#ifdef QBOXHD
        p.resetClip(eRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT));
#else
		p.resetClip(eRect(0, 0, 132, 64));
#endif // QBOXHD
		p.clear();
	}

	return exit_code;
}

eWidgetDesktop *getDesktop(int which)
{
	return which ? lcddsk : wdsk;
}

eApplication *getApplication()
{
	return eApp;
}

void runMainloop()
{
	eApp->runLoop();
}

void quitMainloop(int exitCode)
{
#ifdef QBOXHD_MINI
	FILE *f = fopen("/proc/stb/lpc/was_timer_wakeup", "w");
#else
	FILE *f = fopen("/proc/stb/fp/was_timer_wakeup", "w");
#endif

	if (f)
	{
		fprintf(f, "%d", 0);
		fclose(f);
	}
	else
	{
		int fd = open("/dev/dbox/fp0", O_WRONLY);
		if (fd >= 0)
		{
			if (ioctl(fd, 10 /*FP_CLEAR_WAKEUP_TIMER*/) < 0)
				eDebug("FP_CLEAR_WAKEUP_TIMER failed (%m)");
			close(fd);
		}
		else
			eDebug("open /dev/dbox/fp0 for wakeup timer clear failed!(%m)");
	}
	exit_code = exitCode;
	eApp->quit(0);
	eDebug("quitMainloop(exitCode=%d)<",exitCode);
}

#include "version.h"

const char *getEnigmaVersionString()
{
	return
#ifdef ENIGMA2_CHECKOUT_TAG
		ENIGMA2_CHECKOUT_TAG
#else
#ifdef QBOXHD
#ifdef QBOXHD_MINI
		"QBoxHD mini"
#else
		"QBoxHD"
#endif
#else
		"HEAD"
#endif ///QBOXHD
#endif
			"-" __DATE__;
}
#include <malloc.h>

void dump_malloc_stats(void)
{
	struct mallinfo mi = mallinfo();
	eDebug("MALLOC: %d total", mi.uordblks);
}
