# -*- coding: ISO-8859-1 -*-
#===============================================================================
# VLC Player Plugin by A. L�tsch 2007
#                   modified by Volker Christian 2008
#
# This is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2, or (at your option) any later
# version.
#===============================================================================
from qboxhd import QBOXHD
from Components.Language import language
from Tools.Directories import resolveFilename, SCOPE_PLUGINS, SCOPE_LANGUAGE
import os,gettext

def localeInit():
	lang = language.getLanguage()[:2] # getLanguage returns e.g. "fi_FI" for "language_country"
	os.environ["LANGUAGE"] = lang # Enigma doesn't set this (or LC_ALL, LC_MESSAGES, LANG). gettext needs it!
	gettext.bindtextdomain("enigma2", resolveFilename(SCOPE_LANGUAGE))
	gettext.textdomain("enigma2")
	gettext.bindtextdomain("VlcPlayer", resolveFilename(SCOPE_PLUGINS, "Extensions/VlcPlayer/locale"))

def _(txt):
	t = gettext.dgettext("VlcPlayer", txt)
	if t == txt:
		print "[VLC] fallback to default translation for", txt
		t = gettext.gettext(txt)
	return t

localeInit()
