from qboxhd import QBOXHD, QBOXHD_MINI
import Plugins.Plugin
from Components.config import config
from Components.config import ConfigSubsection
from Components.config import ConfigSelection
from Components.config import ConfigInteger
from Components.config import ConfigSubList
from Components.config import ConfigSubDict
from Components.config import ConfigText
from Components.config import configfile
from Components.config import ConfigYesNo
from skin import loadSkin

currentmcversion = "093"
currentmcplatform = "sh4"

config.plugins.mc_favorites = ConfigSubsection()
config.plugins.mc_favorites.foldercount = ConfigInteger(0)
config.plugins.mc_favorites.folders = ConfigSubList()

config.plugins.mc_globalsettings = ConfigSubsection()
config.plugins.mc_globalsettings.language = ConfigSelection(default="EN", choices = [("EN", _("English"))])

if QBOXHD:
	config.plugins.mc_globalsettings.showinmainmenu = ConfigYesNo(default=True)
	config.plugins.mc_globalsettings.checkforupdate = ConfigYesNo(default=False)
	config.plugins.mc_globalsettings.showinextmenu = ConfigYesNo(default=False)
else:
	config.plugins.mc_globalsettings.showinmainmenu = ConfigYesNo(default=True)
	config.plugins.mc_globalsettings.checkforupdate = ConfigYesNo(default=True)
	config.plugins.mc_globalsettings.showinextmenu = ConfigYesNo(default=True)
	
config.plugins.mc_globalsettings.currentversion = ConfigInteger(0, (0, 999))
config.plugins.mc_globalsettings.currentplatform = ConfigText(default = currentmcplatform)

config.plugins.mc_globalsettings.dst_top = ConfigInteger(0, (0, 999))
config.plugins.mc_globalsettings.dst_left = ConfigInteger(0, (0, 999))
config.plugins.mc_globalsettings.dst_width = ConfigInteger(720, (1, 720))
config.plugins.mc_globalsettings.dst_height = ConfigInteger(576, (1, 576))

config.plugins.mc_globalsettings.currentskin = ConfigSubsection()
config.plugins.mc_globalsettings.currentskin.path = ConfigText(default = "default/skin.xml")

config.plugins.mc_globalsettings.currentversion.value = currentmcversion
config.plugins.mc_globalsettings.currentplatform.value = currentmcplatform

# Load Skin
try:
	if QBOXHD:
		loadSkin("../../../../usr/local/lib/enigma2/python/Plugins/Extensions/MediaCenter/skins/" + config.plugins.mc_globalsettings.currentskin.path.value)
	else:
		loadSkin("../../../usr/lib/enigma2/python/Plugins/Extensions/MediaCenter/skins/" + config.plugins.mc_globalsettings.currentskin.path.value)
except Exception, e:
	if QBOXHD:
		loadSkin("../../../../usr/local/lib/enigma2/python/Plugins/Extensions/MediaCenter/skins/default/skin.xml")
	else:
		loadSkin("../../../usr/lib/enigma2/python/Plugins/Extensions/MediaCenter/skins/default/skin.xml")

# Favorite Folders
def addFavoriteFolders():
	i = len(config.plugins.mc_favorites.folders)
	config.plugins.mc_favorites.folders.append(ConfigSubsection())
	config.plugins.mc_favorites.folders[i].name = ConfigText("", False)
	config.plugins.mc_favorites.folders[i].basedir = ConfigText("/", False)
	config.plugins.mc_favorites.foldercount.value = i+1
	return i

for i in range(0, config.plugins.mc_favorites.foldercount.value):
	addFavoriteFolders()


# VLC PLAYER CONFIG
config.plugins.mc_vlc = ConfigSubsection()
config.plugins.mc_vlc.lastDir = ConfigText(default="")

config.plugins.mc_vlc.foldercount = ConfigInteger(0)
config.plugins.mc_vlc.folders = ConfigSubList()

config.plugins.mc_vlc.vcodec = ConfigSelection({"mp1v": "MPEG1", "mp2v": "MPEG2"}, "mp2v")
config.plugins.mc_vlc.vb = ConfigInteger(1000, (100, 9999))
config.plugins.mc_vlc.acodec = ConfigSelection({"mpga":"MP1", "mp2a": "MP2", "mp3": "MP3"}, "mp2a")
config.plugins.mc_vlc.ab = ConfigInteger(128, (64, 320))
config.plugins.mc_vlc.samplerate = ConfigSelection({"0":"as Input", "44100": "44100", "48000": "48000"}, "0")
config.plugins.mc_vlc.channels = ConfigInteger(2, (1, 9))
config.plugins.mc_vlc.width = ConfigSelection(["352", "704", "720"])
config.plugins.mc_vlc.height = ConfigSelection(["288", "576"])
config.plugins.mc_vlc.fps = ConfigInteger(25, (1, 99))
config.plugins.mc_vlc.aspect = ConfigSelection(["none", "16:9", "4:3"], "none")
config.plugins.mc_vlc.soverlay = ConfigYesNo()
config.plugins.mc_vlc.checkdvd = ConfigYesNo(True)
config.plugins.mc_vlc.notranscode = ConfigYesNo(False) 

config.plugins.mc_vlc.servercount = ConfigInteger(0)
config.plugins.mc_vlc.servers = ConfigSubList()

def addVlcServerConfig():
	i = len(config.plugins.mc_vlc.servers)
	config.plugins.mc_vlc.servers.append(ConfigSubsection())
	config.plugins.mc_vlc.servers[i].host = ConfigText("", False)
	config.plugins.mc_vlc.servers[i].httpport = ConfigInteger(8080, (0,65535))
	config.plugins.mc_vlc.servers[i].basedir = ConfigText("/", False)
	config.plugins.mc_vlc.servercount.value = i+1
	return i

for i in range(0, config.plugins.mc_vlc.servercount.value):
	addVlcServerConfig()