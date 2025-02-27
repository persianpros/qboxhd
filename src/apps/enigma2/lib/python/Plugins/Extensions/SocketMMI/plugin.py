from qboxhd import QBOXHD
from Plugins.Plugin import PluginDescriptor
from SocketMMI import SocketMMIMessageHandler

socketHandler = None

def main(session, **kwargs):
	socketHandler.startMMI()

def menu(menuid, **kwargs):
	if menuid == "setup" and socketHandler and socketHandler.connected():
		return [(socketHandler.getName(), main, "socket_mmi", 0)]
	return [ ]

def sessionstart(reason, session):
	socketHandler.setSession(session)

def autostart(reason, **kwargs):
	global socketHandler
	if reason == 1:
		socketHandler = None
	else:
		socketHandler = SocketMMIMessageHandler()

def Plugins(**kwargs):
	return [ PluginDescriptor(name = "SocketMMI", description = _("Python frontend for /tmp/mmi.socket"), where = PluginDescriptor.WHERE_MENU, fnc = menu),
		PluginDescriptor(where = PluginDescriptor.WHERE_SESSIONSTART, fnc = sessionstart),
 		PluginDescriptor(where = PluginDescriptor.WHERE_AUTOSTART, fnc = autostart) ]
