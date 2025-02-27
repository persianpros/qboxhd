#include <fcntl.h>
#include <sys/ioctl.h>

#include <lib/base/init.h>
#include <lib/base/init_num.h>
#include <lib/base/ebase.h>

#include <lib/base/eerror.h>
#include <lib/base/nconfig.h> // access to python config
#include <lib/dvb/db.h>
#include <lib/dvb/pmt.h>
#include <lib/dvb_ci/dvbci.h>
#include <lib/dvb_ci/dvbci_session.h>
#include <lib/dvb_ci/dvbci_camgr.h>
#include <lib/dvb_ci/dvbci_ui.h>
#include <lib/dvb_ci/dvbci_appmgr.h>
#include <lib/dvb_ci/dvbci_mmi.h>

#include <dvbsi++/ca_program_map_section.h>

#ifdef QBOXHD
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "add_func.h"
#include "starci2win.h"

#define eDebugCI eDebug
#define TIME_POLL_CAM   100000	//100ms

#define F_MAP_SIZE      8192UL  
#define F_MAP_MASK      (F_MAP_SIZE - 1)

#define F_BASE_ADDRESS    0x3800000   

extern CI_Info_t ci_info[2];

static unsigned char isUsed;
static unsigned char reset_state=0;

int first_tuner_found;

#endif // QBOXHD

eDVBCIInterfaces *eDVBCIInterfaces::instance = 0;

eDVBCIInterfaces::eDVBCIInterfaces()
{
	int num_ci = 0;

	instance = this;

#ifdef QBOXHD
    isUsed=0;

    int  cnt;
    char *ptr,
         line[32];

    // FIXME: If one of these calls fail, 
	do{
	    if ((nim_fd = fopen("/proc/bus/nim_sockets", "r")) == NULL) {
        	eDebug("%s(): open(): %m\n", __func__);
			break;
    	}
	
	    if ((cnt = fread(line, sizeof(char), 32, nim_fd)) == -1) {
        	eDebug("%s(): read(): %m\n", __func__);
			break;
	    }
	
	    ptr = strstr(line, ":");
	    
	    first_tuner_found = *(ptr - 1) - 48;
	    if (first_tuner_found < 0 || first_tuner_found > 2) {
        	eDebug("%s(): Invalid tuner position '%d'", __func__, first_tuner_found);
			break;
    	}
	}while(0);

#endif // QBOXHD

	eDebug("scanning for common interfaces..");

	while (1)
	{
		struct stat s;
		char filename[128];
		sprintf(filename, "/dev/ci%d", num_ci);

		if (stat(filename, &s))
			break;

		ePtr<eDVBCISlot> cislot;

		cislot = new eDVBCISlot(eApp, num_ci);
		m_slots.push_back(cislot);

		++num_ci;
	}

	for (eSmartPtrList<eDVBCISlot>::iterator it(m_slots.begin()); it != m_slots.end(); ++it)
		it->setSource(TUNER_A);

#ifdef QBOXHD
    setInputSource(0, TUNER_A);
    setInputSource(1, TUNER_B);
#else
	if (num_ci > 1) // // FIXME .. we force DM8000 when more than one CI Slot is avail
	{
		setInputSource(0, TUNER_A);
		setInputSource(1, TUNER_B);
		setInputSource(2, TUNER_C);
		setInputSource(3, TUNER_D);
	}
	else
	{
		setInputSource(0, TUNER_A);
		setInputSource(1, TUNER_B);
	}
#endif // QBOXHD

	eDebug("done, found %d common interface slots", num_ci);
}

eDVBCIInterfaces::~eDVBCIInterfaces()
{
    fclose(nim_fd);

//     munmap(map_base, F_MAP_SIZE);
//     close(f_fd);
}

eDVBCIInterfaces *eDVBCIInterfaces::getInstance()
{
//printf("\n");
//printf("------------------------------>*eDVBCIInterfaces::getInstance\n");
//printf("\n");
	return instance;
}

eDVBCISlot *eDVBCIInterfaces::getSlot(int slotid)
{
//printf("\n");
//printf("------------------------------>eDVBCIInterfaces::getSlot\n");
//printf("\n");
	for(eSmartPtrList<eDVBCISlot>::iterator i(m_slots.begin()); i != m_slots.end(); ++i)
		if(i->getSlotID() == slotid)
			return i;

	eDebug("FIXME: request for unknown slot");

	return 0;
}

int eDVBCIInterfaces::getSlotState(int slotid)
{
//printf("\n");
//printf("------------------------------>eDVBCIInterfaces::getSlotState\n");
//printf("\n");
	eDVBCISlot *slot;

	if( (slot = getSlot(slotid)) == 0 )
		return eDVBCISlot::stateInvalid;

	return slot->getState();
}

int eDVBCIInterfaces::reset(int slotid)
{
//printf("\n");
//printf("------------------------------>eDVBCIInterfaces::reset\n");
//printf("\n");
	eDVBCISlot *slot;

	if( (slot = getSlot(slotid)) == 0 )
		return -1;
#ifdef QBOXHD
	eDVBCISession::deleteSessions(slot);
	ciRemoved(slot);
#endif ///QBOXHD
	return slot->reset();
}

int eDVBCIInterfaces::initialize(int slotid)
{
//printf("\n");
//printf("------------------------------>eDVBCIInterfaces::initialize\n");
//printf("\n");
	eDVBCISlot *slot;

	if( (slot = getSlot(slotid)) == 0 )
		return -1;

	slot->removeService();

	return sendCAPMT(slotid);
}

#ifdef QBOXHD
void reset_the_modules(int v)
{
	Module_t mod;
	if( (v!=0) && (v!=1) )
	{
		eDebug("Wrong parameter value: %d\n", v);
		return;
	}

	reset_state=v;
	sleep(1);	/* wait that the PCMCIA communication is stopped */

	mod.configuration=v;
	if(ci_info[0].fd_ci>=0)
	{
		eDebug("Module A ...");
		mod.module=MODULE_A;
		ioctl(ci_info[0].fd_ci,IOCTL_ACTIVATION_MODULE_RST,&mod);
	}
	if(ci_info[1].fd_ci>=0)
	{
		eDebug("Module B ...");
		mod.module=MODULE_B;
		ioctl(ci_info[1].fd_ci,IOCTL_ACTIVATION_MODULE_RST,&mod);
	}
}

/* This is a function that permits in the python files to reset the CI modules */
void eDVBCIInterfaces::rst_all_modules(int value)	/* For standby... */
{
	eDebug("rst_all_modules ... value: %d\n",value);
	reset_the_modules(value);
}
#else
void eDVBCIInterfaces::rst_all_modules(int value)	/* For standby... */
{
	eDebug("rst_all_modules ... value: %d - DUMMY\n",value);
}
#endif ///QBOXHD

int eDVBCIInterfaces::sendCAPMT(int slotid)
{
//printf("\n");
//printf("------------------------------>eDVBCIInterfaces::sendCAPMT\n");
//printf("\n");
	eDVBCISlot *slot;

	if( (slot = getSlot(slotid)) == 0 )
		return -1;

	PMTHandlerList::iterator it = m_pmt_handlers.begin();
	while (it != m_pmt_handlers.end())
	{
		eDVBCISlot *tmp = it->cislot;
		while (tmp && tmp != slot)
			tmp = tmp->linked_next;
		if (tmp)
		{
			tmp->sendCAPMT(it->pmthandler);  // send capmt
			break;
		}
		++it;
	}

	return 0;
}

int eDVBCIInterfaces::startMMI(int slotid)
{
//printf("\n");
//printf("------------------------------>eDVBCIInterfaces::startMMI\n");
//printf("\n");
	eDVBCISlot *slot;

	if( (slot = getSlot(slotid)) == 0 )
		return -1;

	return slot->startMMI();
}

int eDVBCIInterfaces::stopMMI(int slotid)
{
//printf("\n");
//printf("------------------------------>eDVBCIInterfaces::stopMMI\n");
//printf("\n");
	eDVBCISlot *slot;

	if( (slot = getSlot(slotid)) == 0 )
		return -1;

	return slot->stopMMI();
}

int eDVBCIInterfaces::answerText(int slotid, int answer)
{
//printf("\n");
//printf("------------------------------>eDVBCIInterfaces::answerTex\n");
//printf("\n");
	eDVBCISlot *slot;

	if( (slot = getSlot(slotid)) == 0 )
		return -1;

	return slot->answerText(answer);
}

int eDVBCIInterfaces::answerEnq(int slotid, char *value)
{
//printf("\n");
//printf("------------------------------>eDVBCIInterfaces::answerEnq\n");
//printf("\n");

	eDVBCISlot *slot;

	if( (slot = getSlot(slotid)) == 0 )
		return -1;

	return slot->answerEnq(value);
}

int eDVBCIInterfaces::cancelEnq(int slotid)
{
//printf("\n");
//printf("------------------------------>eDVBCIInterfaces::cancelEnq\n");
//printf("\n");
	eDVBCISlot *slot;

	if( (slot = getSlot(slotid)) == 0 )
		return -1;

	return slot->cancelEnq();
}

void eDVBCIInterfaces::ciRemoved(eDVBCISlot *slot)
{
	if (slot->use_count)
	{
		eDebug("CI Slot %d: removed... usecount %d", slot->getSlotID(), slot->use_count);
		for (PMTHandlerList::iterator it(m_pmt_handlers.begin());
			it != m_pmt_handlers.end(); ++it)
		{
			if (it->cislot == slot) // remove the base slot
				it->cislot = slot->linked_next;
			else if (it->cislot)
			{
				eDVBCISlot *prevSlot = it->cislot, *hSlot = it->cislot->linked_next;
				while (hSlot)
				{
					if (hSlot == slot) {
						prevSlot->linked_next = slot->linked_next;
						break;
					}
					prevSlot = hSlot;
					hSlot = hSlot->linked_next;
				}
			}
		}
#ifndef QBOXHD
		if (slot->linked_next)
			slot->linked_next->setSource(slot->current_source);
		else // last CI in chain
			setInputSource(slot->current_tuner, slot->current_source);
#endif
		slot->linked_next = 0;
		slot->use_count=0;
		slot->plugged=true;
		slot->user_mapped=false;
		slot->removeService(0xFFFF);
#ifndef QBOXHD
        // When the CAM is removed, the PMT and ts are assigned to a CAM slot. If this happens, 
        // when the CAM is inserted again it's not assigned to the current channel because e2 already assigned it
        // We want to 
		recheckPMTHandlers();
#endif // QBOXHD
	}
}

static bool canDescrambleMultipleServices(int slotid)
{
//printf("\n");
//printf("------------------------------>canDescrambleMultipleServices\n");
//printf("\n");
	char configStr[255];
	snprintf(configStr, 255, "config.ci.%d.canDescrambleMultipleServices", slotid);
	std::string str;
	ePythonConfigQuery::getConfigValue(configStr, str);
	if ( str == "auto" )
	{
		std::string appname = eDVBCI_UI::getInstance()->getAppName(slotid);
		if (appname.find("AlphaCrypt") != std::string::npos)
			return true;
	}
	else if (str == "yes")
		return true;
	return false;
}

void eDVBCIInterfaces::recheckPMTHandlers()
{
	eDebugCI("recheckPMTHandlers()");
	for (PMTHandlerList::iterator it(m_pmt_handlers.begin());
		it != m_pmt_handlers.end(); ++it)
	{
		CAID_LIST caids;
		ePtr<eDVBService> service;
		eServiceReferenceDVB ref;
		eDVBCISlot *tmp = it->cislot;
		eDVBServicePMTHandler *pmthandler = it->pmthandler;
		eDVBServicePMTHandler::program p;
		bool plugged_cis_exist = false;

		pmthandler->getServiceReference(ref);
		pmthandler->getService(service);

		eDebugCI("recheck %p %s", pmthandler, ref.toString().c_str());
		for (eSmartPtrList<eDVBCISlot>::iterator ci_it(m_slots.begin()); ci_it != m_slots.end(); ++ci_it)
			if (ci_it->plugged && ci_it->getCAManager())
			{
				eDebug("Slot %d plugged", ci_it->getSlotID());
				ci_it->plugged = false;
				plugged_cis_exist = true;
			}

		// check if this pmt handler has already assigned CI(s) .. and this CI(s) are already running
		if (!plugged_cis_exist)
		{
			while(tmp)
			{
				if (!tmp->running_services.empty())
					break;
				tmp=tmp->linked_next;
			}
			if (tmp) // we dont like to change tsmux for running services
			{
				eDebugCI("already assigned and running CI!\n");
				continue;
			}
		}

		if (!pmthandler->getProgramInfo(p))
		{
			int cnt=0;
			for (caidSet::reverse_iterator x(p.caids.rbegin()); x != p.caids.rend(); ++x, ++cnt)
				caids.push_front(*x);
			if (service && cnt)
				service->m_ca = caids;
		}

		if (service)
			caids = service->m_ca;

		if (caids.empty())
			continue; // unscrambled service

		for (eSmartPtrList<eDVBCISlot>::iterator ci_it(m_slots.begin()); ci_it != m_slots.end(); ++ci_it)
		{
			eDebugCI("check Slot %d", ci_it->getSlotID());
			bool useThis=false;
			bool user_mapped=true;
			eDVBCICAManagerSession *ca_manager = ci_it->getCAManager();

			if (ca_manager)
			{
				int mask=0;
				if (!ci_it->possible_services.empty())
				{
					mask |= 1;
					serviceSet::iterator it = ci_it->possible_services.find(ref);
					if (it != ci_it->possible_services.end())
					{
						eDebug("'%s' is in service list of slot %d... so use it", ref.toString().c_str(), ci_it->getSlotID());
						useThis = true;
					}
					else // check parent
					{
						eServiceReferenceDVB parent_ref = ref.getParentServiceReference();
						if (parent_ref)
						{
							it = ci_it->possible_services.find(ref);
							if (it != ci_it->possible_services.end())
							{
								eDebug("parent '%s' of '%s' is in service list of slot %d... so use it",
									parent_ref.toString().c_str(), ref.toString().c_str(), ci_it->getSlotID());
								useThis = true;
							}
						}
					}
				}
				if (!useThis && !ci_it->possible_providers.empty())
				{
					eDVBNamespace ns = ref.getDVBNamespace();
					mask |= 2;
					if (!service) // subservice?
					{
						eServiceReferenceDVB parent_ref = ref.getParentServiceReference();
						eDVBDB::getInstance()->getService(parent_ref, service);
					}
					if (service)
					{
						providerSet::iterator it = ci_it->possible_providers.find(providerPair(service->m_provider_name, ns.get()));
						if (it != ci_it->possible_providers.end())
						{
							eDebug("'%s/%08x' is in provider list of slot %d... so use it", service->m_provider_name.c_str(), ns.get(), ci_it->getSlotID());
							useThis = true;
						}
					}
				}
				if (!useThis && !ci_it->possible_caids.empty())
				{
					mask |= 4;
					for (CAID_LIST::iterator ca(caids.begin()); ca != caids.end(); ++ca)
					{
						caidSet::iterator it = ci_it->possible_caids.find(*ca);
						if (it != ci_it->possible_caids.end())
						{
							eDebug("caid '%04x' is in caid list of slot %d... so use it", *ca, ci_it->getSlotID());
							useThis = true;
							break;
						}
					}
				}
				if (!useThis && !mask)
				{
					const std::vector<uint16_t> &ci_caids = ca_manager->getCAIDs();
					for (CAID_LIST::iterator ca(caids.begin()); ca != caids.end(); ++ca)
					{
						std::vector<uint16_t>::const_iterator z =
							std::lower_bound(ci_caids.begin(), ci_caids.end(), *ca);
						if ( z != ci_caids.end() && *z == *ca )
						{
							eDebug("The CI in Slot %d has said it can handle caid %04x... so use it", ci_it->getSlotID(), *z);
							useThis = true;
							user_mapped = false;
							break;
						}
					}
				}
			}

			if (useThis)
			{
				// check if this CI is already assigned to this pmthandler
				eDVBCISlot *tmp = it->cislot;
				while(tmp)
				{
					if (tmp == ci_it)
						break;
					tmp=tmp->linked_next;
				}
				if (tmp) // ignore already assigned cislots...
				{
					eDebugCI("already assigned!");
					continue;
				}
				eDebugCI("current slot %d usecount %d", ci_it->getSlotID(), ci_it->use_count);
				if (ci_it->use_count)  // check if this CI can descramble more than one service
				{
					bool found = false;
					useThis = false;
					PMTHandlerList::iterator tmp = m_pmt_handlers.begin();
					while (!found && tmp != m_pmt_handlers.end())
					{
						eDebugCI(".");
						eDVBCISlot *tmp_cislot = tmp->cislot;
						while (!found && tmp_cislot)
						{
							eDebugCI("..");
							eServiceReferenceDVB ref2;
							tmp->pmthandler->getServiceReference(ref2);
							if ( tmp_cislot == ci_it && it->pmthandler != tmp->pmthandler )
							{
								eDebugCI("check pmthandler %s for same service/tp", ref2.toString().c_str());
								eDVBChannelID s1, s2;
								if (ref != ref2)
								{
									eDebugCI("different services!");
									ref.getChannelID(s1);
									ref2.getChannelID(s2);
								}
								if (ref == ref2 || (s1 == s2 && canDescrambleMultipleServices(tmp_cislot->getSlotID())))
								{
									found = true;
									eDebugCI("found!");
									eDVBCISlot *tmpci = it->cislot = tmp->cislot;
									while(tmpci)
									{
										++tmpci->use_count;
										eDebug("(2)CISlot %d, usecount now %d", tmpci->getSlotID(), tmpci->use_count);
										tmpci=tmpci->linked_next;
									}
								}
							}
							tmp_cislot=tmp_cislot->linked_next;
						}
						eDebugCI("...");
						++tmp;
					}
				}

				if (useThis)
				{
					if (ci_it->user_mapped)  // we dont like to link user mapped CIs
					{
						eDebugCI("user mapped CI already in use... dont link!");
						continue;
					}

					++ci_it->use_count;
					eDebug("(1)CISlot %d, usecount now %d", ci_it->getSlotID(), ci_it->use_count);

					data_source ci_source=CI_A;
					switch(ci_it->getSlotID())
					{
						case 0: ci_source = CI_A; break;
						case 1: ci_source = CI_B; break;
						case 2: ci_source = CI_C; break;
						case 3: ci_source = CI_D; break;
						default:
							eDebug("try to get source for CI %d!!\n", ci_it->getSlotID());
							break;
					}

					if (!it->cislot)
					{
						int tunernum = -1;
						eUsePtr<iDVBChannel> channel;
						if (!pmthandler->getChannel(channel))
						{
							ePtr<iDVBFrontend> frontend;
							if (!channel->getFrontend(frontend))
							{
								eDVBFrontend *fe = (eDVBFrontend*) &(*frontend);
								tunernum = fe->getSlotID();
							}
						}
						ASSERT(tunernum != -1);
						data_source tuner_source = TUNER_A;
						switch (tunernum)
						{
							case 0: tuner_source = TUNER_A; break;
							case 1: tuner_source = TUNER_B; break;
							case 2: tuner_source = TUNER_C; break;
							case 3: tuner_source = TUNER_D; break;
							default:
								eDebug("try to get source for tuner %d!!\n", tunernum);
								break;
						}
						ci_it->current_tuner = tunernum;
#ifndef QBOXHD
						setInputSource(tunernum, ci_source);
						ci_it->setSource(tuner_source);
#endif
					}
					else
					{
						ci_it->current_tuner = it->cislot->current_tuner;
						ci_it->linked_next = it->cislot;
#ifndef QBOXHD
						ci_it->setSource(ci_it->linked_next->current_source);
						ci_it->linked_next->setSource(ci_source);
#endif
					}
					it->cislot = ci_it;
					eDebugCI("assigned!");
					gotPMT(pmthandler);
				}

				if (it->cislot && user_mapped) // CI assigned to this pmthandler in this run.. and user mapped? then we break here.. we dont like to link other CIs to user mapped CIs
				{
					eDebugCI("user mapped CI assigned... dont link CIs!");
					break;
				}
			}
		}
	}
#ifdef QBOXHD
    #ifndef QBOXHD_MINI
        int i = 0;
        unsigned char cam_slot;
        bb_channels channels;
        memset((unsigned char*)&channels, 0, sizeof(channels));

	for (PMTHandlerList::iterator it(m_pmt_handlers.begin());
		it != m_pmt_handlers.end(); ++it)
	{
		eDVBCISlot *tmp = it->cislot;
		eDVBServicePMTHandler *pmthandler = it->pmthandler;

                eUsePtr<iDVBChannel> ch;
                pmthandler->getChannel(ch);
                ePtr<iDVBFrontend> fe_tmp;
                int slotid_tmp=0;
                ch->getFrontend(fe_tmp);
                eDVBFrontend *fe2 = (eDVBFrontend*) &(*fe_tmp);
                slotid_tmp = fe2->getSlotID();
                ePtr<iDVBDemux> m_d;
                ch->getDemux(m_d);
                eDVBDemux *d2=(eDVBDemux*)&(*m_d);

                channels.ch_tuner[i] = d2->getAdapter();
                channels.ch_phys[i] = slotid_tmp;

                while(tmp)
                {
                    cam_slot = tmp->getSlotID();
                    if (cam_slot<2) channels.ch_cam[i] += (cam_slot+1);
                        tmp=tmp->linked_next;
                }
                i++;
                if (i>=2)   break;
        }
        
        channels.channels_quant = i;
        //eDebugCI("****************************************************************** BlackBoxSwitch Start");
        BlackBoxSwitch(&channels);
        //eDebugCI("****************************************************************** BlackBoxSwitch End");
	#else
		start_check_TSM();
    #endif //QBOXHD_MINI
#endif //QBOXHD
}

void eDVBCIInterfaces::addPMTHandler(eDVBServicePMTHandler *pmthandler)
{
	// check if this pmthandler is already registered
	PMTHandlerList::iterator it = m_pmt_handlers.begin();
	while (it != m_pmt_handlers.end())
	{
		if ( *it++ == pmthandler )
			return;
	}

	eServiceReferenceDVB ref;
	pmthandler->getServiceReference(ref);
	eDebug("[eDVBCIInterfaces] addPMTHandler %s", ref.toString().c_str());
#if 0
#ifdef QBOXHD
	/* For detect new channel */
	char tmp[strlen(ref.toString().c_str())+4];	/* +4: arbitrary... */
	memset(tmp,0,strlen(ref.toString().c_str())+4);
	sprintf(tmp, "s=%s",ref.toString().c_str());
	comunicate_generic_info(tmp,strlen(tmp));       // message for channel change !!!!!!!!!
	/* ******* */
#endif ///QBOXHD
#endif
	m_pmt_handlers.push_back(CIPmtHandler(pmthandler));
	recheckPMTHandlers();
}

void eDVBCIInterfaces::removePMTHandler(eDVBServicePMTHandler *pmthandler)
{
	PMTHandlerList::iterator it=std::find(m_pmt_handlers.begin(),m_pmt_handlers.end(),pmthandler);
	if (it != m_pmt_handlers.end())
	{
		eDVBCISlot *slot = it->cislot;
		eDVBCISlot *base_slot = slot;
		eDVBServicePMTHandler *pmthandler = it->pmthandler;
		m_pmt_handlers.erase(it);

		eServiceReferenceDVB service_to_remove;
		pmthandler->getServiceReference(service_to_remove);

		bool sameServiceExist=false;
		for (PMTHandlerList::iterator i=m_pmt_handlers.begin(); i != m_pmt_handlers.end(); ++i)
		{
			if (i->cislot)
			{
				eServiceReferenceDVB ref;
				i->pmthandler->getServiceReference(ref);
				if ( ref == service_to_remove )
				{
					sameServiceExist=true;
					break;
				}
			}
		}

		while(slot)
		{
			eDVBCISlot *next = slot->linked_next;
			if (!sameServiceExist)
			{
				eDebug("[eDVBCIInterfaces] remove last pmt handler for service %s send empty capmt",
					service_to_remove.toString().c_str());
				std::vector<uint16_t> caids;
				caids.push_back(0xFFFF);
				slot->sendCAPMT(pmthandler, caids);  // send a capmt without caids to remove a running service
				slot->removeService(service_to_remove.getServiceID().get());
			}

			if (!--slot->use_count)
			{
#ifndef QBOXHD
				if (slot->linked_next)
					slot->linked_next->setSource(slot->current_source);
				else
					setInputSource(slot->current_tuner, slot->current_source);
#endif
                                
				if (base_slot != slot)
				{
					eDVBCISlot *tmp = it->cislot;
					while(tmp->linked_next != slot)
						tmp = tmp->linked_next;
					ASSERT(tmp);
					if (slot->linked_next)
						tmp->linked_next = slot->linked_next;
					else
						tmp->linked_next = 0;
				}
				else // removed old base slot.. update ptr
					base_slot = slot->linked_next;
				slot->linked_next = 0;
				slot->user_mapped = false;
			}
			eDebug("(3) slot %d usecount is now %d", slot->getSlotID(), slot->use_count);
			slot = next;
		}
		// check if another service is waiting for the CI
		recheckPMTHandlers();
	}
}

void eDVBCIInterfaces::gotPMT(eDVBServicePMTHandler *pmthandler)
{
//printf("\n");
//printf("------------------------------>eDVBCIInterfaces::gotPMT\n");
//printf("\n");
	eDebug("[eDVBCIInterfaces] gotPMT");
	PMTHandlerList::iterator it=std::find(m_pmt_handlers.begin(), m_pmt_handlers.end(), pmthandler);
	if (it != m_pmt_handlers.end() && it->cislot)
	{
		eDVBCISlot *tmp = it->cislot;
		while(tmp)
		{
			eDebugCI("check slot %d %d %d", tmp->getSlotID(), tmp->running_services.empty(), canDescrambleMultipleServices(tmp->getSlotID()));
			if (tmp->running_services.empty() || canDescrambleMultipleServices(tmp->getSlotID()))
				tmp->sendCAPMT(pmthandler);
			tmp = tmp->linked_next;
		}
	}
}

int eDVBCIInterfaces::getMMIState(int slotid)
{
//printf("\n");
//printf("------------------------------>eDVBCIInterfaces::getMMIState(\n");
//printf("\n");
	eDVBCISlot *slot;

	if( (slot = getSlot(slotid)) == 0 )
		return -1;

	return slot->getMMIState();
}

int eDVBCIInterfaces::setInputSource(int tuner_no, data_source source)
{
#ifdef QBOXHD
// eDebug("\n\n------------------------------>eDVBCIInterfaces::setInputSource\n");
    eDebug("\neDVBCIInterfaces::setInputSource(%d %d)\n", tuner_no, (int)source);

    
	set_tuner_to_cam(tuner_no,source);
	return 0;
#else
//	eDebug("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
//	eDebug("eDVBCIInterfaces::setInputSource(%d %d)", tuner_no, (int)source);
	if (getNumOfSlots() > 1) // FIXME .. we force DM8000 when more than one CI Slot is avail
	{
		char buf[64];
		snprintf(buf, 64, "/proc/stb/tsmux/input%d", tuner_no);

		FILE *input=0;
		if((input = fopen(buf, "wb")) == NULL) {
			eDebug("cannot open %s", buf);
			return 0;
		}

		if (tuner_no > 3)
			eDebug("setInputSource(%d, %d) failed... dm8000 just have four inputs", tuner_no, (int)source);

		switch(source)
		{
			case CI_A:
				fprintf(input, "CI0");
				break;
			case CI_B:
				fprintf(input, "CI1");
				break;
			case CI_C:
				fprintf(input, "CI2");
			break;
			case CI_D:
				fprintf(input, "CI3");
				break;
			case TUNER_A:
				fprintf(input, "A");
				break;
			case TUNER_B:
				fprintf(input, "B");
				break;
			case TUNER_C:
				fprintf(input, "C");
				break;
			case TUNER_D:
				fprintf(input, "D");
				break;
			default:
				eDebug("setInputSource for input %d failed!!!\n", (int)source);
				break;
		}

		fclose(input);
	}
	else  // DM7025
	{
		char buf[64];
		snprintf(buf, 64, "/etc/enigma2/stb/tsmux/input%d", tuner_no);

		if (tuner_no > 1)
			eDebug("setInputSource(%d, %d) failed... dm7025 just have two inputs", tuner_no, (int)source);

		FILE *input=0;
		if((input = fopen(buf, "wb")) == NULL) {
			eDebug("cannot open %s", buf);
			return 0;
		}

		switch(source)
		{
			case CI_A:
				fprintf(input, "CI");
				break;
			case TUNER_A:
				fprintf(input, "A");
				break;
			case TUNER_B:
				fprintf(input, "B");
				break;
			default:
				eDebug("setInputSource for input %d failed!!!\n", (int)source);
				break;
		}

		fclose(input);
	}
	eDebug("eDVBCIInterfaces->setInputSource(%d, %d)", tuner_no, (int)source);
	return 0;
#endif
}

PyObject *eDVBCIInterfaces::getDescrambleRules(int slotid)
{
// printf("\n");
// printf("------------------------------>eDVBCIInterfaces::getDescrambleRules\n");
// printf("\n");
	eDVBCISlot *slot = getSlot(slotid);
	if (!slot)
	{
		char tmp[255];
		snprintf(tmp, 255, "eDVBCIInterfaces::getDescrambleRules try to get rules for CI Slot %d... but just %d slots are available", slotid, m_slots.size());
		PyErr_SetString(PyExc_StandardError, tmp);
		return 0;
	}
	ePyObject tuple = PyTuple_New(3);
	int caids = slot->possible_caids.size();
	int services = slot->possible_services.size();
	int providers = slot->possible_providers.size();
	ePyObject caid_list = PyList_New(caids);
	ePyObject service_list = PyList_New(services);
	ePyObject provider_list = PyList_New(providers);
	caidSet::iterator caid_it(slot->possible_caids.begin());
	while(caids)
	{
		--caids;
		PyList_SET_ITEM(caid_list, caids, PyLong_FromLong(*caid_it));
		++caid_it;
	}
	serviceSet::iterator ref_it(slot->possible_services.begin());
	while(services)
	{
		--services;
		PyList_SET_ITEM(service_list, services, PyString_FromString(ref_it->toString().c_str()));
		++ref_it;
	}
	providerSet::iterator provider_it(slot->possible_providers.begin());
	while(providers)
	{
		ePyObject tuple = PyTuple_New(2);
		PyTuple_SET_ITEM(tuple, 0, PyString_FromString(provider_it->first.c_str()));
		PyTuple_SET_ITEM(tuple, 1, PyLong_FromUnsignedLong(provider_it->second));
		--providers;
		PyList_SET_ITEM(provider_list, providers, tuple);
		++provider_it;
	}
	PyTuple_SET_ITEM(tuple, 0, service_list);
	PyTuple_SET_ITEM(tuple, 1, provider_list);
	PyTuple_SET_ITEM(tuple, 2, caid_list);
	return tuple;
}

const char *PyObject_TypeStr(PyObject *o)
{
//printf("\n");
//printf("------------------------------>PyObject_TypeStr\n");
//printf("\n");
	return o->ob_type && o->ob_type->tp_name ? o->ob_type->tp_name : "unknown object type";
}

RESULT eDVBCIInterfaces::setDescrambleRules(int slotid, SWIG_PYOBJECT(ePyObject) obj )
{
// printf("\n");
// printf("------------------------------>eDVBCIInterfaces::setDescrambleRules\n");
// printf("\n");
	eDVBCISlot *slot = getSlot(slotid);
	if (!slot)
	{
		char tmp[255];
		snprintf(tmp, 255, "eDVBCIInterfaces::setDescrambleRules try to set rules for CI Slot %d... but just %d slots are available", slotid, m_slots.size());
		PyErr_SetString(PyExc_StandardError, tmp);
		return -1;
	}
	if (!PyTuple_Check(obj))
	{
		char tmp[255];
		snprintf(tmp, 255, "2nd argument of setDescrambleRules is not a tuple.. it is a '%s'!!", PyObject_TypeStr(obj));
		PyErr_SetString(PyExc_StandardError, tmp);
		return -1;
	}
	if (PyTuple_Size(obj) != 3)
	{
		const char *errstr = "eDVBCIInterfaces::setDescrambleRules not enough entrys in argument tuple!!\n"
			"first argument should be a pythonlist with possible services\n"
			"second argument should be a pythonlist with possible providers/dvbnamespace tuples\n"
			"third argument should be a pythonlist with possible caids";
		PyErr_SetString(PyExc_StandardError, errstr);
		return -1;
	}
	ePyObject service_list = PyTuple_GET_ITEM(obj, 0);
	ePyObject provider_list = PyTuple_GET_ITEM(obj, 1);
	ePyObject caid_list = PyTuple_GET_ITEM(obj, 2);
	if (!PyList_Check(service_list) || !PyList_Check(provider_list) || !PyList_Check(caid_list))
	{
		char errstr[512];
		snprintf(errstr, 512, "eDVBCIInterfaces::setDescrambleRules incorrect data types in argument tuple!!\n"
			"first argument(%s) should be a pythonlist with possible services (reference strings)\n"
			"second argument(%s) should be a pythonlist with possible providers (providername strings)\n"
			"third argument(%s) should be a pythonlist with possible caids (ints)",
			PyObject_TypeStr(service_list), PyObject_TypeStr(provider_list), PyObject_TypeStr(caid_list));
		PyErr_SetString(PyExc_StandardError, errstr);
		return -1;
	}
	slot->possible_caids.clear();
	slot->possible_services.clear();
	slot->possible_providers.clear();
	int size = PyList_Size(service_list);
	while(size)
	{
		--size;
		ePyObject refstr = PyList_GET_ITEM(service_list, size);
		if (!PyString_Check(refstr))
		{
			char buf[255];
			snprintf(buf, 255, "eDVBCIInterfaces::setDescrambleRules entry in service list is not a string.. it is '%s'!!", PyObject_TypeStr(refstr));
			PyErr_SetString(PyExc_StandardError, buf);
			return -1;
		}
		char *tmpstr = PyString_AS_STRING(refstr);
		eServiceReference ref(tmpstr);
		if (ref.valid())
			slot->possible_services.insert(ref);
		else
			eDebug("eDVBCIInterfaces::setDescrambleRules '%s' is not a valid service reference... ignore!!", tmpstr);
	};
	size = PyList_Size(provider_list);
	while(size)
	{
		--size;
		ePyObject tuple = PyList_GET_ITEM(provider_list, size);
		if (!PyTuple_Check(tuple))
		{
			char buf[255];
			snprintf(buf, 255, "eDVBCIInterfaces::setDescrambleRules entry in provider list is not a tuple it is '%s'!!", PyObject_TypeStr(tuple));
			PyErr_SetString(PyExc_StandardError, buf);
			return -1;
		}
		if (PyTuple_Size(tuple) != 2)
		{
			char buf[255];
			snprintf(buf, 255, "eDVBCIInterfaces::setDescrambleRules provider tuple has %d instead of 2 entries!!", PyTuple_Size(tuple));
			PyErr_SetString(PyExc_StandardError, buf);
			return -1;
		}
		if (!PyString_Check(PyTuple_GET_ITEM(tuple, 0)))
		{
			char buf[255];
			snprintf(buf, 255, "eDVBCIInterfaces::setDescrambleRules 1st entry in provider tuple is not a string it is '%s'", PyObject_TypeStr(PyTuple_GET_ITEM(tuple, 0)));
			PyErr_SetString(PyExc_StandardError, buf);
			return -1;
		}
		if (!PyLong_Check(PyTuple_GET_ITEM(tuple, 1)))
		{
			char buf[255];
			snprintf(buf, 255, "eDVBCIInterfaces::setDescrambleRules 2nd entry in provider tuple is not a long it is '%s'", PyObject_TypeStr(PyTuple_GET_ITEM(tuple, 1)));
			PyErr_SetString(PyExc_StandardError, buf);
			return -1;
		}
		char *tmpstr = PyString_AS_STRING(PyTuple_GET_ITEM(tuple, 0));
		uint32_t orbpos = PyLong_AsUnsignedLong(PyTuple_GET_ITEM(tuple, 1));
		if (strlen(tmpstr))
			slot->possible_providers.insert(std::pair<std::string, uint32_t>(tmpstr, orbpos));
		else
			eDebug("eDVBCIInterfaces::setDescrambleRules ignore invalid entry in provider tuple (string is empty)!!");
	};
	size = PyList_Size(caid_list);
	while(size)
	{
		--size;
		ePyObject caid = PyList_GET_ITEM(caid_list, size);
		if (!PyLong_Check(caid))
		{
			char buf[255];
			snprintf(buf, 255, "eDVBCIInterfaces::setDescrambleRules entry in caid list is not a long it is '%s'!!", PyObject_TypeStr(caid));
			PyErr_SetString(PyExc_StandardError, buf);
			return -1;
		}
		int tmpcaid = PyLong_AsLong(caid);
		if (tmpcaid > 0 && tmpcaid < 0x10000)
			slot->possible_caids.insert(tmpcaid);
		else
			eDebug("eDVBCIInterfaces::setDescrambleRules %d is not a valid caid... ignore!!", tmpcaid);
	};
	return 0;
}

PyObject *eDVBCIInterfaces::readCICaIds(int slotid)
{
	eDVBCISlot *slot = getSlot(slotid);
	if (!slot)
	{
		char tmp[255];
		snprintf(tmp, 255, "eDVBCIInterfaces::readCICaIds try to get CAIds for CI Slot %d... but just %d slots are available", slotid, m_slots.size());
		PyErr_SetString(PyExc_StandardError, tmp);
	}
	else
	{
		int idx=0;
		eDVBCICAManagerSession *ca_manager = slot->getCAManager();
		const std::vector<uint16_t> *ci_caids = ca_manager ? &ca_manager->getCAIDs() : 0;
		ePyObject list = PyList_New(ci_caids ? ci_caids->size() : 0);
		if (ci_caids)
		{
			for (std::vector<uint16_t>::const_iterator it = ci_caids->begin(); it != ci_caids->end(); ++it)
				PyList_SET_ITEM(list, idx++, PyLong_FromLong(*it));
		}
		return list;
	}
	return 0;
}

int eDVBCIInterfaces::setCIClockRate(int slotid, int rate)
{
	eDVBCISlot *slot = getSlot(slotid);
	if (slot)
		return slot->setClockRate(rate);
	return -1;
}

int eDVBCISlot::send(const unsigned char *data, size_t len)
{
//printf("\n");
//printf("------------------------------>eDVBCISlot::send\n");
//printf("\n");
	int res=0;

	if (sendqueue.empty())
		res = ::write(fd, data, len);

	if (res < 0 || (unsigned int)res != len)
	{
		unsigned char *d = new unsigned char[len];
		memcpy(d, data, len);
		sendqueue.push( queueData(d, len) );
		notifier->setRequested(eSocketNotifier::Read | eSocketNotifier::Priority | eSocketNotifier::Write);
	}

	return res;
}

#ifdef QBOXHD
void eDVBCISlot::data(int what)
{
/*
if(state == stateInserted)
{
    //if(tento<0x9FF)
    if(tento<0x13FE)
    {
        tento++;
        return;
    }
    tento=0;
}
*/
//usleep(TIME_POLL_CAM);//FIXME

//  unsigned int i=0;
    unsigned char res=0xF;
    //res=IsInserted_modA();

	if(reset_state==1)
	{
		if(state==stateInserted)
		{
			eDVBCIInterfaces::getInstance()->ciRemoved(this);
			eDVBCISession::deleteSessions(this);
			notifier->setRequested(eSocketNotifier::Read|eSocketNotifier::Priority);
			eDVBCI_UI::getInstance()->setState(getSlotID(),0);
			state = stateInvalid;
		}
		return;
	}

    res=IsInserted(slotid);

    if( (res==0) && (state==stateInserted) )
    {
        //state = stateInvalid;
        eDVBCIInterfaces::getInstance()->ciRemoved(this);
        eDVBCISession::deleteSessions(this);
/**/        notifier->setRequested(eSocketNotifier::Read|eSocketNotifier::Priority);
        eDVBCI_UI::getInstance()->setState(getSlotID(),0);
        state = stateInvalid;
        return;
    }
    else if( (res==1) && (state==stateRemoved) )
    {
        //state = stateInvalid;
/**/    notifier->setRequested(eSocketNotifier::Read|eSocketNotifier::Priority);
        state = stateInvalid;
		
		reset();

        return;
    }

    if(what == eSocketNotifier::Priority) {
        if(state != stateRemoved && state != stateResetted) {
            //state = stateRemoved;

            eDebug("ci removed\n");
            while(sendqueue.size())
            {
                delete [] sendqueue.top().data;
                sendqueue.pop();
            }
            eDVBCIInterfaces::getInstance()->ciRemoved(this);
            eDVBCISession::deleteSessions(this);
            notifier->setRequested(eSocketNotifier::Read);
            eDVBCI_UI::getInstance()->setState(getSlotID(),0);
            state = stateRemoved;
        }
        return;
    }

    if (state == stateInvalid)
    {
    //  eDebug("ci invalid\n");
        //res=IsInserted_modA();
        res=IsInserted(slotid);
        if(res==0)
        {
//          state = stateRemoved;
/**/        notifier->setRequested(eSocketNotifier::Read|eSocketNotifier::Priority);
            return;
        }

            reset();
    }

    if(state != stateInserted) {
        //res=IsInserted_modA();
        res=IsInserted(slotid);
        if(res==0)
        {
            //state = stateRemoved;
            /*while(sendqueue.size())
            {
                delete [] sendqueue.top().data;
                sendqueue.pop();
            }*/
            eDVBCIInterfaces::getInstance()->ciRemoved(this);
            eDVBCISession::deleteSessions(this);
/**/            notifier->setRequested(eSocketNotifier::Read);
            eDVBCI_UI::getInstance()->setState(getSlotID(),0);
            state = stateRemoved;
            return;
        }
        eDebug("ci inserted\n");
        state = stateInserted;
        eDVBCI_UI::getInstance()->setState(getSlotID(),1);
        notifier->setRequested(eSocketNotifier::Read|eSocketNotifier::Priority);
        /* enable PRI to detect removal or errors */
    }

    if (what & eSocketNotifier::Read) {
        //__u8 data[4096];

        res=IsInserted(slotid);
        if(res==0)
        {
            return;
        }
        unsigned char data[4096];
        int r;

        r = ::read(fd, data, 4096);
        if(r > 0) {

            eDVBCISession::receiveData(this, (const unsigned char *)data, r);
            eDVBCISession::pollAll();
            return;
        }
    }
    else if (what & eSocketNotifier::Write) {
        res=IsInserted(slotid);
        if(res==0)
        {
            return;
        }
        if (!sendqueue.empty()) {
            const queueData &qe = sendqueue.top();
            int res = ::write(fd, qe.data, qe.len);
            if (res >= 0 && (unsigned int)res == qe.len)
            {
                delete [] qe.data;
                sendqueue.pop();
            }
        }
        else
        {
            notifier->setRequested(eSocketNotifier::Read|eSocketNotifier::Priority);
        }
    }

}
#else
void eDVBCISlot::data(int what)
{
	eDebugCI("CISlot %d what %d\n", getSlotID(), what);
	if(what == eSocketNotifier::Priority) {
		if(state != stateRemoved) {
			state = stateRemoved;
			while(sendqueue.size())
			{
				delete [] sendqueue.top().data;
				sendqueue.pop();
			}
			eDVBCISession::deleteSessions(this);
			eDVBCIInterfaces::getInstance()->ciRemoved(this);
			notifier->setRequested(eSocketNotifier::Read);
			eDVBCI_UI::getInstance()->setState(getSlotID(),0);
		}
		return;
	}

	if (state == stateInvalid)
		reset();

	if(state != stateInserted) {
		eDebug("ci inserted in slot %d", getSlotID());
		state = stateInserted;
		eDVBCI_UI::getInstance()->setState(getSlotID(),1);
		notifier->setRequested(eSocketNotifier::Read|eSocketNotifier::Priority);
		/* enable PRI to detect removal or errors */
	}

	if (what & eSocketNotifier::Read) {
		//__u8 data[4096];
		unsigned char data[4096];
		int r;

		r = ::read(fd, data, 4096);

		if(r > 0) {
//			int i;
//			eDebugNoNewLine("> ");
//			for(i=0;i<r;i++)
//				eDebugNoNewLine("%02x ",data[i]);
//			eDebug("");
			eDVBCISession::receiveData(this, (const unsigned char *)data, r);
			eDVBCISession::pollAll();
			return;
		}
	}
	else if (what & eSocketNotifier::Write) {
		if (!sendqueue.empty()) {
			const queueData &qe = sendqueue.top();

			int res = ::write(fd, qe.data, qe.len);

			if (res >= 0 && (unsigned int)res == qe.len)
			{
				delete [] qe.data;
				sendqueue.pop();
			}
		}
		else
			notifier->setRequested(eSocketNotifier::Read|eSocketNotifier::Priority);
	}
}
#endif

DEFINE_REF(eDVBCISlot);

eDVBCISlot::eDVBCISlot(eMainloop *context, int nr)
{
//printf("\n");
//printf("------------------------------>eDVBCISlot::eDVBCISlot\n");
//printf("\n");
	char filename[128];

	application_manager = 0;
	mmi_session = 0;
	ca_manager = 0;
	use_count = 0;
	linked_next = 0;
	user_mapped = false;
	plugged = true;

	slotid = nr;

	sprintf(filename, "/dev/ci%d", nr);

//	possible_caids.insert(0x1702);
//	possible_providers.insert(providerPair("PREMIERE", 0xC00000));
//	possible_services.insert(eServiceReference("1:0:1:2A:4:85:C00000:0:0:0:"));

	fd = ::open(filename, O_RDWR | O_NONBLOCK);

#ifdef QBOXHD
    /* FIXME */
    ci_info[nr].fd_ci=fd;

    eDebugCI("CI Slot %d has fd %d", getSlotID(), fd);
    state = stateRemoved;
#else
    eDebugCI("CI Slot %d has fd %d", getSlotID(), fd);
	state = stateInvalid;
#endif
	if (fd >= 0)
	{
		notifier = eSocketNotifier::create(context, fd, eSocketNotifier::Read | eSocketNotifier::Priority | eSocketNotifier::Write);
		CONNECT(notifier->activated, eDVBCISlot::data);
	} else
	{
		perror(filename);
	}
}

eDVBCISlot::~eDVBCISlot()
{
	eDVBCISession::deleteSessions(this);
}

void eDVBCISlot::setAppManager( eDVBCIApplicationManagerSession *session )
{
	application_manager=session;
}

void eDVBCISlot::setMMIManager( eDVBCIMMISession *session )
{
	mmi_session = session;
}

void eDVBCISlot::setCAManager( eDVBCICAManagerSession *session )
{
	ca_manager = session;
}

int eDVBCISlot::getSlotID()
{
	return slotid;
}

int eDVBCISlot::reset()
{
	eDebug("CI Slot %d: reset requested", getSlotID());

#ifdef QBOXHD
	if (state == stateInvalid)
#endif
	{
		unsigned char buf[256];
		eDebug("ci flush");
		while(::read(fd, buf, 256)>0);
		state = stateResetted;
	}

	while(sendqueue.size())
	{
		delete [] sendqueue.top().data;
		sendqueue.pop();
	}

#ifdef QBOXHD
    if(isUsed==0)
    {
        isUsed=1;
        ioctl(fd, 0);
        isUsed=0;
    }
#else
	ioctl(fd, 0);
#endif

	return 0;
}

int eDVBCISlot::startMMI()
{
	eDebug("CI Slot %d: startMMI()", getSlotID());

	if(application_manager)
		application_manager->startMMI();

	return 0;
}

int eDVBCISlot::stopMMI()
{
	eDebug("CI Slot %d: stopMMI()", getSlotID());

	if(mmi_session)
		mmi_session->stopMMI();

	return 0;
}

int eDVBCISlot::answerText(int answer)
{
	eDebug("CI Slot %d: answerText(%d)", getSlotID(), answer);

	if(mmi_session)
		mmi_session->answerText(answer);

	return 0;
}

int eDVBCISlot::getMMIState()
{
//printf("\n");
//printf("------------------------------>eDVBCISlot::getMMIState\n");
//printf("\n");
	if(mmi_session)
		return 1;

	return 0;
}

int eDVBCISlot::answerEnq(char *value)
{
	eDebug("CI Slot %d: answerENQ(%s)", getSlotID(), value);

	if(mmi_session)
		mmi_session->answerEnq(value);

	return 0;
}

int eDVBCISlot::cancelEnq()
{
	eDebug("CI Slot %d: cancelENQ", getSlotID());

	if(mmi_session)
		mmi_session->cancelEnq();

	return 0;
}

int eDVBCISlot::sendCAPMT(eDVBServicePMTHandler *pmthandler, const std::vector<uint16_t> &ids)
{
//printf("\n");
//printf("------------------------------>eDVBCISlot::sendCAPMT\n");
//printf("\n");
	if (!ca_manager)
	{
		eDebug("no ca_manager (no CI plugged?)");
		return -1;
	}
	const std::vector<uint16_t> &caids = ids.empty() ? ca_manager->getCAIDs() : ids;
	ePtr<eTable<ProgramMapSection> > ptr;
	if (pmthandler->getPMT(ptr))
		return -1;
	else
	{
		eDVBTableSpec table_spec;
		ptr->getSpec(table_spec);
		int pmt_version = table_spec.version & 0x1F; // just 5 bits

		eServiceReferenceDVB ref;
		pmthandler->getServiceReference(ref);
		uint16_t program_number = ref.getServiceID().get();
		std::map<uint16_t, uint8_t>::iterator it =
			running_services.find(program_number);
		bool sendEmpty = caids.size() == 1 && caids[0] == 0xFFFF;

		if ( it != running_services.end() &&
			(pmt_version == it->second) &&
			!sendEmpty )
		{
			eDebug("[eDVBCISlot] dont send self capmt version twice");
			return -1;
		}

		std::vector<ProgramMapSection*>::const_iterator i=ptr->getSections().begin();
		if ( i == ptr->getSections().end() )
			return -1;
		else
		{
			unsigned char raw_data[2048];

//			eDebug("send %s capmt for service %04x to slot %d",
//				it != running_services.end() ? "UPDATE" : running_services.empty() ? "ONLY" : "ADD",
//				program_number, slotid);

			CaProgramMapSection capmt(*i++,
				it != running_services.end() ? 0x05 /*update*/ : running_services.empty() ? 0x03 /*only*/ : 0x04 /*add*/, 0x01, caids );
			while( i != ptr->getSections().end() )
			{
		//			eDebug("append");
				capmt.append(*i++);
			}
			capmt.writeToBuffer(raw_data);

// begin calc capmt length
			int wp=0;
			int hlen;
			if ( raw_data[3] & 0x80 )
			{
				int i=0;
				int lenbytes = raw_data[3] & ~0x80;
				while(i < lenbytes)
					wp = (wp << 8) | raw_data[4 + i++];
				wp+=4;
				wp+=lenbytes;
				hlen = 4 + lenbytes;
			}
			else
			{
				wp = raw_data[3];
				wp+=4;
				hlen = 4;
			}
// end calc capmt length

			if (sendEmpty)
			{
//				eDebugNoNewLine("SEND EMPTY CAPMT.. old version is %02x", raw_data[hlen+3]);
				if (sendEmpty && running_services.size() == 1)  // check if this is the capmt for the last running service
					raw_data[hlen] = 0x03; // send only instead of update... because of strange effects with alphacrypt
				raw_data[hlen+3] &= ~0x3E;
				raw_data[hlen+3] |= ((pmt_version+1) & 0x1F) << 1;
//				eDebug(" new version is %02x", raw_data[hlen+3]);
			}

//			eDebug("ca_manager %p dump capmt:", ca_manager);
//			for(int i=0;i<wp;i++)
//				eDebugNoNewLine("%02x ", raw_data[i]);
//			eDebug("");

			//dont need tag and lenfield
			ca_manager->sendCAPMT(raw_data + hlen, wp - hlen);
			running_services[program_number] = pmt_version;
		}
	}
	return 0;
}

void eDVBCISlot::removeService(uint16_t program_number)
{
//printf("\n");
//printf("------------------------------>eDVBCISlot::removeService\n");
//printf("\n");
	if (program_number == 0xFFFF)
		running_services.clear();  // remove all
	else
		running_services.erase(program_number);  // remove single service
}

int eDVBCISlot::setSource(data_source source)
{
#ifndef QBOXHD
	current_source = source;
	if (eDVBCIInterfaces::getInstance()->getNumOfSlots() > 1) // FIXME .. we force DM8000 when more than one CI Slot is avail
	{
		char buf[64];
		snprintf(buf, 64, "/proc/stb/tsmux/ci%d_input", slotid);
		FILE *ci = fopen(buf, "wb");
		switch(source)
		{
			case CI_A:
				fprintf(ci, "CI0");
				break;
			case CI_B:
				fprintf(ci, "CI1");
				break;
			case CI_C:
				fprintf(ci, "CI2");
				break;
			case CI_D:
				fprintf(ci, "CI3");
				break;
			case TUNER_A:
				fprintf(ci, "A");
				break;
			case TUNER_B:
				fprintf(ci, "B");
				break;
			case TUNER_C:
				fprintf(ci, "C");
				break;
				case TUNER_D:
				fprintf(ci, "D");
				break;
			default:
				eDebug("CI Slot %d: setSource %d failed!!!\n", getSlotID(), (int)source);
				break;
		}
		fclose(ci);
	}
	else // DM7025
	{
//		eDebug("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
//		eDebug("eDVBCISlot::enableTS(%d %d)", enable, (int)source);
		FILE *ci = fopen("/proc/stb/tsmux/input2", "wb");
		if(ci == NULL) {
			eDebug("cannot open /proc/stb/tsmux/input2");
			return 0;
		}
		if (source != TUNER_A && source != TUNER_B)
			eDebug("CI Slot %d: setSource %d failed!!!\n", getSlotID(), (int)source);
		else
			fprintf(ci, "%s", source==TUNER_A ? "A" : "B");  // configure CI data source (TunerA, TunerB)
		fclose(ci);
	}
	eDebug("CI Slot %d setSource(%d)", getSlotID(), (int)source);
#endif
	return 0;
}

int eDVBCISlot::setClockRate(int rate)
{
	char buf[64];
	snprintf(buf, 64, "/proc/stb/tsmux/ci%d_tsclk", slotid);
	FILE *ci = fopen(buf, "wb");
	if (ci)
	{
		if (rate)
			fprintf(ci, "high");
		else
			fprintf(ci, "normal");
		fclose(ci);
		return 0;
	}
	return -1;
}

eAutoInitP0<eDVBCIInterfaces> init_eDVBCIInterfaces(eAutoInitNumbers::dvb, "CI Slots");
