from qboxhd import QBOXHD
from Screen import Screen
from Components.ConfigList import ConfigListScreen
from Components.config import config, ConfigSubsection, ConfigSelection, getConfigListEntry
from InfoBarGenerics import InfoBarNotifications
import Screens.Standby
from Tools import Notifications

class JobView(InfoBarNotifications, Screen, ConfigListScreen):
	def __init__(self, session, job, parent=None, cancelable = True, backgroundable = True, afterEvent = 0):
		from Components.Sources.StaticText import StaticText
		from Components.Sources.Progress import Progress
		from Components.Sources.Boolean import Boolean
		from Components.ActionMap import ActionMap
		Screen.__init__(self, session, parent)
		InfoBarNotifications.__init__(self)
		ConfigListScreen.__init__(self, [])
		self.parent = parent
		self.job = job

		self["job_name"] = StaticText(job.name)
		self["job_progress"] = Progress()
		self["job_task"] = StaticText()
		self["summary_job_name"] = StaticText(job.name)
		self["summary_job_progress"] = Progress()
		self["summary_job_task"] = StaticText()
		self["job_status"] = StaticText()
		self["finished"] = Boolean()
		self["cancelable"] = Boolean(cancelable)
		self["backgroundable"] = Boolean(backgroundable)

		self["key_blue"] = StaticText(_("Background"))

		self.onShow.append(self.windowShow)
		self.onHide.append(self.windowHide)

		self["setupActions"] = ActionMap(["ColorActions", "SetupActions"],
		{
		    "green": self.ok,
		    "red": self.abort,
		    "blue": self.background,
		    "cancel": self.ok,
		    "ok": self.ok,
		}, -2)

		self.afterevents = [ "nothing", "standby", "deepstandby", "close" ]
		self.settings = ConfigSubsection()
		self.settings.afterEvent = ConfigSelection(choices = [("nothing", _("do nothing")), ("close", _("Close")), ("standby", _("go to standby")), ("deepstandby", _("go to deep standby"))], default = self.afterevents[afterEvent])
		self.setupList()
		self.state_changed()

	def setupList(self):
		self["config"].setList( [ getConfigListEntry(_("After event"), self.settings.afterEvent) ])
		
	def keyLeft(self):
		ConfigListScreen.keyLeft(self)
		self.setupList()

	def keyRight(self):
		ConfigListScreen.keyRight(self)
		self.setupList()
	
	def windowShow(self):
		self.job.state_changed.append(self.state_changed)

	def windowHide(self):
		if len(self.job.state_changed) > 0:
		    self.job.state_changed.remove(self.state_changed)

	def state_changed(self):
		j = self.job
		self["job_progress"].range = j.end
		self["summary_job_progress"].range = j.end
		self["job_progress"].value = j.progress
		self["summary_job_progress"].value = j.progress
		#print "JobView::state_changed:", j.end, j.progress
		self["job_status"].text = j.getStatustext()
		if j.status == j.IN_PROGRESS:
			self["job_task"].text = j.tasks[j.current_task].name
			self["summary_job_task"].text = j.tasks[j.current_task].name
		else:
			self["job_task"].text = ""
			self["summary_job_task"].text = j.getStatustext()
		if j.status in (j.FINISHED, j.FAILED):
			self.performAfterEvent()
			self["backgroundable"].boolean = False
			if j.status == j.FINISHED:
				self["finished"].boolean = True
				self["cancelable"].boolean = False
			elif j.status == j.FAILED:
				self["cancelable"].boolean = True

	def background(self):
		if self["backgroundable"].boolean == True:
			self.close(True)

	def ok(self):
		if self.job.status in (self.job.FINISHED, self.job.FAILED):
			self.close(False)

	def abort(self):
		if self.job.status in (self.job.FINISHED, self.job.FAILED):
			self.close(False)
		if self["cancelable"].boolean == True:
			self.job.cancel()

	def performAfterEvent(self):
		self["config"].hide()
		if self.settings.afterEvent.getValue() == "nothing":
			return
		elif self.settings.afterEvent.getValue() == "close":
			self.abort()
		from Screens.MessageBox import MessageBox
		if self.settings.afterEvent.getValue() == "deepstandby":
			if not Screens.Standby.inTryQuitMainloop:
				Notifications.AddNotificationWithCallback(self.sendTryQuitMainloopNotification, MessageBox, _("A sleep timer wants to shut down\nyour QBoxHD. Shutdown now?"), timeout = 20)
		elif self.settings.afterEvent.getValue() == "standby":
			if not Screens.Standby.inStandby:
				Notifications.AddNotificationWithCallback(self.sendStandbyNotification, MessageBox, _("A sleep timer wants to set your\nQBoxHD to standby. Do that now?"), timeout = 20)

	def sendStandbyNotification(self, answer):
		if answer:
			Notifications.AddNotification(Screens.Standby.Standby)

	def sendTryQuitMainloopNotification(self, answer):
		if answer:
			Notifications.AddNotification(Screens.Standby.TryQuitMainloop, 1)
