/*      $OpenBSD: src/sys/dev/pcmcia/if_malo.c,v 1.27 2007/07/31 23:00:38 claudio Exp $ */

/*
 * Copyright (c) 2007 Marcus Glocker <mglocker@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/timeout.h>
#include <sys/socket.h>
#include <sys/tree.h>
#include <sys/malloc.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_llc.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/if_ether.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_radiotap.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

#include <dev/pcmcia/if_malovar.h>
#include <dev/pcmcia/if_maloreg.h>

/*
 * Driver for the Marvell 88W8385 CF chip.
 */

#ifdef CMALO_DEBUG
int cmalo_d = 2;
#define DPRINTF(l, x...)	do { if ((l) <= cmalo_d) printf(x); } while (0)
#else
#define DPRINTF(l, x...)
#endif

int	malo_pcmcia_match(struct device *, void *, void *);
void	malo_pcmcia_attach(struct device *, struct device *, void *);
int	malo_pcmcia_detach(struct device *, int);
int	malo_pcmcia_activate(struct device *, enum devact);

void	cmalo_attach(void *);
int	cmalo_ioctl(struct ifnet *, u_long, caddr_t);
int	cmalo_fw_load_helper(struct malo_softc *);
int	cmalo_fw_load_main(struct malo_softc *);
int	cmalo_init(struct ifnet *);
void	cmalo_stop(struct malo_softc *);
int	cmalo_media_change(struct ifnet *);
int	cmalo_newstate(struct ieee80211com *, enum ieee80211_state, int);
void	cmalo_detach(void *);
int	cmalo_intr(void *);
void	cmalo_intr_mask(struct malo_softc *, int);
void	cmalo_rx(struct malo_softc *);
void	cmalo_start(struct ifnet *);
int	cmalo_tx(struct malo_softc *, struct mbuf *);
void	cmalo_tx_done(struct malo_softc *);

void	cmalo_hexdump(void *, int);
int	cmalo_cmd_get_hwspec(struct malo_softc *);
int	cmalo_cmd_rsp_hwspec(struct malo_softc *);
int	cmalo_cmd_set_reset(struct malo_softc *);
int	cmalo_cmd_set_scan(struct malo_softc *);
int	cmalo_cmd_rsp_scan(struct malo_softc *);
int	cmalo_parse_elements(struct malo_softc *, void *, int, int);
int	cmalo_cmd_set_auth(struct malo_softc *);
int	cmalo_cmd_set_snmp(struct malo_softc *, uint16_t);
int	cmalo_cmd_set_radio(struct malo_softc *, uint16_t);
int	cmalo_cmd_set_channel(struct malo_softc *, uint16_t);
int	cmalo_cmd_set_txpower(struct malo_softc *, int16_t);
int	cmalo_cmd_set_antenna(struct malo_softc *, uint16_t);
int	cmalo_cmd_set_macctrl(struct malo_softc *);
int	cmalo_cmd_set_assoc(struct malo_softc *);
int	cmalo_cmd_set_80211d(struct malo_softc *);
int	cmalo_cmd_set_bgscan_config(struct malo_softc *);
int	cmalo_cmd_set_bgscan_query(struct malo_softc *);
int	cmalo_cmd_set_rate(struct malo_softc *);
int	cmalo_cmd_request(struct malo_softc *, uint16_t, int);
int	cmalo_cmd_response(struct malo_softc *);

/*
 * PCMCIA bus.
 */
struct malo_pcmcia_softc {
	struct malo_softc	 sc_malo;

	struct pcmcia_function	*sc_pf;
	struct pcmcia_io_handle	 sc_pcioh;
	int			 sc_io_window;
	void			*sc_ih;
};

struct cfattach malo_pcmcia_ca = {
	sizeof(struct malo_pcmcia_softc),
	    malo_pcmcia_match,
	    malo_pcmcia_attach,
	    malo_pcmcia_detach,
	    malo_pcmcia_activate
};

int
malo_pcmcia_match(struct device *parent, void *match, void *aux)
{
	struct pcmcia_attach_args *pa = aux;

	if (pa->manufacturer == PCMCIA_VENDOR_AMBICOM &&
	    pa->product == PCMCIA_PRODUCT_AMBICOM_WL54CF)
		return (1);

	return (0);
}

void
malo_pcmcia_attach(struct device *parent, struct device *self, void *aux)
{
	struct malo_pcmcia_softc *psc = (struct malo_pcmcia_softc *)self;
	struct malo_softc *sc = &psc->sc_malo;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	const char *intrstr = NULL;

	psc->sc_pf = pa->pf;
	cfe = SIMPLEQ_FIRST(&pa->pf->cfe_head);

	/* enable card */
	pcmcia_function_init(psc->sc_pf, cfe);
	if (pcmcia_function_enable(psc->sc_pf)) {
		printf(": can't enable function!\n");
		return;
	}

	/* allocate I/O space */
	if (pcmcia_io_alloc(psc->sc_pf, 0,
	    cfe->iospace[0].length, cfe->iospace[0].length, &psc->sc_pcioh)) {
		printf(": can't allocate i/o space!\n");
		pcmcia_function_disable(psc->sc_pf);
		return;
	}

	/* map I/O space */
	if (pcmcia_io_map(psc->sc_pf, PCMCIA_WIDTH_IO16, 0,
	    cfe->iospace[0].length, &psc->sc_pcioh, &psc->sc_io_window)) {
		printf(": can't map i/o space!\n");
		pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);
		pcmcia_function_disable(psc->sc_pf);
		return;
	}
	sc->sc_iot = psc->sc_pcioh.iot;
	sc->sc_ioh = psc->sc_pcioh.ioh;

	printf(" port 0x%x/%d", psc->sc_pcioh.addr, psc->sc_pcioh.size);

	/* establish interrupt */
	psc->sc_ih = pcmcia_intr_establish(psc->sc_pf, IPL_NET, cmalo_intr, sc,
	    sc->sc_dev.dv_xname);
	if (psc->sc_ih == NULL) {
		printf(": can't establish interrupt!\n");
		return;
	}
	intrstr = pcmcia_intr_string(psc->sc_pf, psc->sc_ih);
	if (intrstr != NULL) {
		if (*intrstr != NULL)
			printf(", %s", intrstr);
	}
	printf("\n");

	/* attach device */
	if (rootvp == NULL)
		mountroothook_establish(cmalo_attach, sc);
	else
		cmalo_attach(sc);
}

int
malo_pcmcia_detach(struct device *dev, int flags)
{
	struct malo_pcmcia_softc *psc = (struct malo_pcmcia_softc *)dev;
	struct malo_softc *sc = &psc->sc_malo;

	cmalo_detach(sc);

	pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);
	pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);

	return (0);
}

int
malo_pcmcia_activate(struct device *dev, enum devact act)
{
	struct malo_pcmcia_softc *psc = (struct malo_pcmcia_softc *)dev;
	struct malo_softc *sc = &psc->sc_malo;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &ic->ic_if;
	int s;

	s = splnet();
	switch (act) {
	case DVACT_ACTIVATE:
		pcmcia_function_enable(psc->sc_pf);
		psc->sc_ih = pcmcia_intr_establish(psc->sc_pf, IPL_NET,
		    cmalo_intr, sc, sc->sc_dev.dv_xname);
		cmalo_init(ifp);
		break;
	case DVACT_DEACTIVATE:
		ifp->if_timer = 0;
		if (ifp->if_flags & IFF_RUNNING)
			cmalo_stop(sc);
		if (psc->sc_ih != NULL)
			pcmcia_intr_disestablish(psc->sc_pf, psc->sc_ih);
		pcmcia_function_disable(psc->sc_pf);
		break;
	}
	splx(s);

	return (0);
}

/*
 * Driver.
 */

/* XXX experimental */
#if 0
uint8_t ap[] = { 0x00, 0x17, 0x9a, 0x44, 0xda, 0x83 };
uint8_t chan[] = { 0x01 };
uint8_t ssid[] = "nazgul";

uint8_t ap[] = { 0x00, 0x15, 0xe9, 0xa4, 0x6e, 0xd1 };
uint8_t chan[] = { 0x04 };
uint8_t ssid[] = "foobar";
#endif

uint8_t ap[] = { 0x00, 0x04, 0x75, 0x88, 0xa5, 0xb4 };
uint8_t chan[] = { 0x04 };
uint8_t ssid[] = "foobar";

uint8_t rates[] = { 0x01, 0x04, 0x82, 0x84, 0x0b, 0x16 };

void
cmalo_attach(void *arg)
{
	struct malo_softc *sc = arg;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	int i;

	/* disable interrupts */
	cmalo_intr_mask(sc, 0);

	/* load firmware */
	if (cmalo_fw_load_helper(sc) != 0)
		return;
	if (cmalo_fw_load_main(sc) != 0)
		return;
	sc->sc_flags |= MALO_FW_LOADED;

	/* allocate command buffer */
	sc->sc_cmd = malloc(MALO_CMD_BUFFER_SIZE, M_DEVBUF, M_NOWAIT);

	/* allocate data buffer */
	sc->sc_data = malloc(MCLBYTES, M_DEVBUF, M_NOWAIT);

	/* enable interrupts */
	cmalo_intr_mask(sc, 1);

	/* get hardware specs */
	cmalo_cmd_get_hwspec(sc);

	/* setup interface */
	ifp->if_softc = sc;
	ifp->if_ioctl = cmalo_ioctl;
	ifp->if_init = cmalo_init;
	ifp->if_start = cmalo_start;
	//ifp->if_watchdog = cmalo_watchdog;
	ifp->if_flags = IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST;
	strlcpy(ifp->if_xname, sc->sc_dev.dv_xname, IFNAMSIZ);
	IFQ_SET_MAXLEN(&ifp->if_snd, IFQ_MAXLEN);
	IFQ_SET_READY(&ifp->if_snd);

	ic->ic_opmode = IEEE80211_M_STA;
	ic->ic_state = IEEE80211_S_INIT;
	ic->ic_caps =
	    IEEE80211_C_MONITOR |
	    IEEE80211_C_IBSS |
	    IEEE80211_C_WEP;

	ic->ic_sup_rates[IEEE80211_MODE_11B] = ieee80211_std_rateset_11b;
	ic->ic_sup_rates[IEEE80211_MODE_11G] = ieee80211_std_rateset_11g;

	for (i = 0; i <= 14; i++) {
		ic->ic_channels[i].ic_freq =
		    ieee80211_ieee2mhz(i, IEEE80211_CHAN_2GHZ);
		ic->ic_channels[i].ic_flags =
		    IEEE80211_CHAN_B |
		    IEEE80211_CHAN_G;
	}

	/* attach interface */
	if_attach(ifp);
	ieee80211_ifattach(ifp);

	sc->sc_newstate = ic->ic_newstate;
	ic->ic_newstate = cmalo_newstate;
	ieee80211_media_init(ifp, cmalo_media_change, ieee80211_media_status);

	/* second attach line */
	printf("%s: address %s\n",
	    sc->sc_dev.dv_xname, ether_sprintf(ic->ic_myaddr));

	/* device attached */
	sc->sc_flags |= MALO_DEVICE_ATTACHED;
}

int
cmalo_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct malo_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_nodereq_all *na;
	struct ieee80211_nodereq *nr;
	struct ifaddr *ifa;
	int i, j, s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFADDR:
		ifa = (struct ifaddr *)data;
		ifp->if_flags |= IFF_UP;
#ifdef INET
		if (ifa->ifa_addr->sa_family == AF_INET)
			arp_ifinit(&ic->ic_ac, ifa);
#endif
		/* FALLTHROUGH */
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if ((ifp->if_flags & IFF_RUNNING) == 0)
				cmalo_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				cmalo_stop(sc);
		}
		break;
	case SIOCS80211SCAN:
		cmalo_cmd_set_scan(sc);
		break;
	case SIOCG80211ALLNODES:
		nr = NULL;
		na = (struct ieee80211_nodereq_all *)data;

		if ((nr = malloc(sizeof(*nr), M_DEVBUF, M_WAITOK)) == NULL)
			break;

		for (na->na_nodes = i = j = 0; i < sc->sc_networks_num &&
		    (na->na_size >= j + sizeof(struct ieee80211_nodereq));
		    i++) {
			bzero(nr, sizeof(*nr));

			IEEE80211_ADDR_COPY(nr->nr_macaddr,
			    sc->sc_networks[i].bssid);
			IEEE80211_ADDR_COPY(nr->nr_bssid,
			    sc->sc_networks[i].bssid);
			nr->nr_channel = sc->sc_networks[i].channel;
			nr->nr_chan_flags = IEEE80211_CHAN_B; /* XXX */
			nr->nr_rssi = sc->sc_networks[i].rssi;
			nr->nr_max_rssi = 0; /* XXX */
			nr->nr_nwid_len = strlen(sc->sc_networks[i].ssid);
			bcopy(sc->sc_networks[i].ssid, nr->nr_nwid,
			    nr->nr_nwid_len);
			nr->nr_intval = sc->sc_networks[i].beaconintvl;
			nr->nr_capinfo = sc->sc_networks[i].capinfo;
			nr->nr_flags |= IEEE80211_NODEREQ_AP;

			bcopy(nr, (caddr_t)na->na_node + j,
			    sizeof(struct ieee80211_nodereq));
			j += sizeof(struct ieee80211_nodereq);
			na->na_nodes++;
		}

		if (nr)
			free(nr, M_DEVBUF);
		break;
	default:
		error = ieee80211_ioctl(ifp, cmd, data);
		break;
	}

	if (error == ENETRESET) {
		if (ifp->if_flags & (IFF_UP | IFF_RUNNING))
			cmalo_init(ifp);
		error = 0;
	}

	splx(s);

	return (error);
}

int
cmalo_fw_load_helper(struct malo_softc *sc)
{
	const char *name = "malo8385-h";
	size_t usize;
	uint8_t val8, *ucode;
	uint16_t bsize, *uc;
	int error, offset, i;

	/* verify if the card is ready for firmware download */
	val8 = MALO_READ_1(sc, MALO_REG_SCRATCH);
	if (val8 == MALO_VAL_SCRATCH_FW_LOADED)
		/* firmware already loaded */
		return (0);
	if (val8 != MALO_VAL_SCRATCH_READY) {
		/* bad register value */
		printf("%s: device not ready for FW download!\n",
		    sc->sc_dev.dv_xname);
		return (EIO);
	}

	/* read helper firmware image */
	if ((error = loadfirmware(name, &ucode, &usize)) != 0) {
		printf("%s: can't read microcode %s (error %d)!\n",
		    sc->sc_dev.dv_xname, name, error);
		return (EIO);
	}

	/* download the helper firmware */
	for (offset = 0; offset < usize; offset += bsize) {
		if (usize - offset >= MALO_FW_HELPER_BSIZE)
			bsize = MALO_FW_HELPER_BSIZE;
		else
			bsize = usize - offset;

		/* send a block in words and confirm it */
		DPRINTF(3, "%s: download helper FW block (%d bytes, %d off)\n",
		    sc->sc_dev.dv_xname, bsize, offset);
		MALO_WRITE_2(sc, MALO_REG_CMD_WRITE_LEN, bsize);
		uc = (uint16_t *)(ucode + offset);
		for (i = 0; i < bsize / 2; i++)
			MALO_WRITE_2(sc, MALO_REG_CMD_WRITE, htole16(uc[i]));
		MALO_WRITE_1(sc, MALO_REG_HOST_STATUS, MALO_VAL_CMD_DL_OVER);
		MALO_WRITE_2(sc, MALO_REG_CARD_INTR_CAUSE,
		    MALO_VAL_CMD_DL_OVER);

		/* poll for an acknowledgement */
		for (i = 0; i < 50; i++) {
			if (MALO_READ_1(sc, MALO_REG_CARD_STATUS) ==
			    MALO_VAL_CMD_DL_OVER)
				break;
			delay(1000);
		}
		if (i == 50) {
			printf("%s: timeout while helper FW block download!\n",
			    sc->sc_dev.dv_xname);
			free(ucode, M_DEVBUF);
			return (EIO);
		}
	}
	free(ucode, M_DEVBUF);

	/* helper firmware download done */
	MALO_WRITE_2(sc, MALO_REG_CMD_WRITE_LEN, 0);
	MALO_WRITE_1(sc, MALO_REG_HOST_STATUS, MALO_VAL_CMD_DL_OVER);
	MALO_WRITE_2(sc, MALO_REG_CARD_INTR_CAUSE, MALO_VAL_CMD_DL_OVER);
	DPRINTF(1, "%s: helper FW downloaded\n", sc->sc_dev.dv_xname);

	return (0);
}

int
cmalo_fw_load_main(struct malo_softc *sc)
{
	const char *name = "malo8385-m";
	size_t usize;
	uint8_t *ucode;
	uint16_t val16, bsize, *uc;
	int error, offset, i, retry;

	/* read main firmware image */
	if ((error = loadfirmware(name, &ucode, &usize)) != 0) {
		printf("%s: can't read microcode %s (error %d)!\n",
		    sc->sc_dev.dv_xname, name, error);
		return (EIO);
	}

	/* verify if the helper firmware has been loaded correctly */
	for (i = 0; i < 10; i++) {
		if (MALO_READ_1(sc, MALO_REG_RBAL) == MALO_FW_HELPER_LOADED)
			break;
		delay(1000);
	}
	if (i == 10) {
		printf("%s: helper FW not loaded!\n", sc->sc_dev.dv_xname);
		free(ucode, M_DEVBUF);
		return (EIO);
	}
	DPRINTF(1, "%s: helper FW loaded successfully\n", sc->sc_dev.dv_xname);

	/* download the main firmware */
	for (offset = 0; offset < usize; offset += bsize) {
		val16 = MALO_READ_2(sc, MALO_REG_RBAL);
		/*
		 * If the helper firmware serves us an odd integer then
		 * something went wrong and we retry to download the last
		 * block until we receive a good integer again, or give up.
		 */
		if (val16 & 0x0001) {
			if (retry > MALO_FW_MAIN_MAXRETRY) {
				printf("%s: main FW download failed!\n",
				    sc->sc_dev.dv_xname);
				free(ucode, M_DEVBUF);
				return (EIO);
			}
			retry++;
			offset -= bsize;
		} else {
			retry = 0;
			bsize = val16;
		}

		/* send a block in words and confirm it */
		DPRINTF(3, "%s: download main FW block (%d bytes, %d off)\n",
		    sc->sc_dev.dv_xname, bsize, offset);
		MALO_WRITE_2(sc, MALO_REG_CMD_WRITE_LEN, bsize);
		uc = (uint16_t *)(ucode + offset);
		for (i = 0; i < bsize / 2; i++)
			MALO_WRITE_2(sc, MALO_REG_CMD_WRITE, htole16(uc[i]));
		MALO_WRITE_1(sc, MALO_REG_HOST_STATUS, MALO_VAL_CMD_DL_OVER);
                MALO_WRITE_2(sc, MALO_REG_CARD_INTR_CAUSE,
		    MALO_VAL_CMD_DL_OVER);

		/* poll for an acknowledgement */
		for (i = 0; i < 5000; i++) {
			if (MALO_READ_1(sc, MALO_REG_CARD_STATUS) ==
			    MALO_VAL_CMD_DL_OVER)
				break;
		}
		if (i == 5000) {
			printf("%s: timeout while main FW block download!\n",
			    sc->sc_dev.dv_xname);
			free(ucode, M_DEVBUF);
			return (EIO);
		}
	}
	free(ucode, M_DEVBUF);

	DPRINTF(1, "%s: main FW downloaded\n", sc->sc_dev.dv_xname);

	/* verify if the main firmware has been loaded correctly */
	for (i = 0; i < 50; i++) {
		if (MALO_READ_1(sc, MALO_REG_SCRATCH) ==
		    MALO_VAL_SCRATCH_FW_LOADED)
			break;
		delay(1000);
	}
	if (i == 50) {
		printf("%s: main FW not loaded!\n", sc->sc_dev.dv_xname);
		return (EIO);
	}

	DPRINTF(1, "%s: main FW loaded successfully\n", sc->sc_dev.dv_xname);

	return (0);
}

int
cmalo_init(struct ifnet *ifp)
{
	struct malo_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;

	/* reload the firmware if necessary */
	if (!(sc->sc_flags & MALO_FW_LOADED)) {
		/* disable interrupts */
		cmalo_intr_mask(sc, 0);

		/* load firmware */
		if (cmalo_fw_load_helper(sc) != 0)
			return (EIO);
		if (cmalo_fw_load_main(sc) != 0)
			return (EIO);
		sc->sc_flags |= MALO_FW_LOADED;

		/* enable interrupts */
		cmalo_intr_mask(sc, 1);
	}

	/* get current channel */
        ic->ic_bss->ni_chan = ic->ic_ibss_chan;
        sc->sc_curchan = ieee80211_chan2ieee(ic, ic->ic_bss->ni_chan);

	DPRINTF(1, "%s: current channel is %d\n",
	    sc->sc_dev.dv_xname, sc->sc_curchan);

	/* setup device */
	if (cmalo_cmd_set_macctrl(sc) != 0)
		return (EIO);
	if (cmalo_cmd_set_txpower(sc, 15) != 0)
		return (EIO);
	if (cmalo_cmd_set_antenna(sc, 1) != 0)
		return (EIO);
	if (cmalo_cmd_set_antenna(sc, 2) != 0)
		return (EIO);
	if (cmalo_cmd_set_radio(sc, 1) != 0)
		return (EIO);
	if (cmalo_cmd_set_channel(sc, sc->sc_curchan) != 0)
		return (EIO);

	cmalo_cmd_set_rate(sc);

	cmalo_cmd_set_snmp(sc, MALO_OID_RTSTRESH);
	cmalo_cmd_set_snmp(sc, MALO_OID_SHORTRETRY);
	cmalo_cmd_set_snmp(sc, MALO_OID_FRAGTRESH);

	cmalo_cmd_set_assoc(sc);

	/* device up */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	ieee80211_new_state(ic, IEEE80211_S_RUN, -1);

	return (0);
}

void
cmalo_stop(struct malo_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
        struct ifnet *ifp = &ic->ic_if;

	/* device down */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	/* change device back to initial state */
	ieee80211_new_state(ic, IEEE80211_S_INIT, -1);

	/* reset device */
	cmalo_cmd_set_reset(sc);
	sc->sc_flags &= ~MALO_FW_LOADED;

	DPRINTF(1, "%s: device down\n", sc->sc_dev.dv_xname);
}

int
cmalo_media_change(struct ifnet *ifp)
{
	int error;

	if ((error = ieee80211_media_change(ifp) != ENETRESET))
		return (error);

	if (ifp->if_flags & (IFF_UP | IFF_RUNNING))
		cmalo_init(ifp);

	return (0);
}

int
cmalo_newstate(struct ieee80211com *ic, enum ieee80211_state nstate, int arg)
{
	struct malo_softc *sc = ic->ic_if.if_softc;

	switch (nstate) {
		case IEEE80211_S_INIT:
			DPRINTF(1, "%s: newstate is IEEE80211_S_INIT\n",
			    sc->sc_dev.dv_xname);
			break;
		case IEEE80211_S_SCAN:
			DPRINTF(1, "%s: newstate is IEEE80211_S_SCAN\n",
			    sc->sc_dev.dv_xname);
			break;
		case IEEE80211_S_AUTH:
			DPRINTF(1, "%s: newstate is IEEE80211_S_AUTH\n",
			    sc->sc_dev.dv_xname);
			break;
		case IEEE80211_S_ASSOC:
			DPRINTF(1, "%s: newstate is IEEE80211_S_ASSOC\n",
			    sc->sc_dev.dv_xname);
			break;
		case IEEE80211_S_RUN:
			DPRINTF(1, "%s: newstate is IEEE80211_S_RUN\n",
			    sc->sc_dev.dv_xname);
			break;
		default:
			break;
	}

	return (sc->sc_newstate(ic, nstate, arg));
}

void
cmalo_detach(void *arg)
{
	struct malo_softc *sc = arg;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &ic->ic_if;

	if (!(sc->sc_flags & MALO_DEVICE_ATTACHED))
		/* device was not properly attached */
		return;

	/* free command buffer */
	if (sc->sc_cmd != NULL)
		free(sc->sc_cmd, M_DEVBUF);

	/* free data buffer */
	if (sc->sc_data != NULL)
		free(sc->sc_data, M_DEVBUF);

	/* detach inferface */
	ieee80211_ifdetach(ifp);
	if_detach(ifp);
}

int
cmalo_intr(void *arg)
{
	struct malo_softc *sc = arg;
	uint16_t intr;

	intr = MALO_READ_2(sc, MALO_REG_HOST_INTR_CAUSE);

	if (intr == 0) {
		/* interrupt not for us */
		return (0);
	}
	if (intr == 0xffff) {
		/* card has been detached */
		return (0);
	}

	DPRINTF(2, "%s: interrupt handler called (intr = 0x%04x)\n",
	    sc->sc_dev.dv_xname, intr);

	if (intr & MALO_VAL_HOST_INTR_TX)
		/* TX frame sent */
		cmalo_tx_done(sc);
	if (intr & MALO_VAL_HOST_INTR_RX)
		/* RX frame received */
		cmalo_rx(sc);
	if (intr & MALO_VAL_HOST_INTR_CMD)
		/* command response */
		sc->sc_cmd_running = 0;

	/* acknowledge interrupt */
	intr &= MALO_VAL_HOST_INTR_MASK_ON;
	MALO_WRITE_2(sc, MALO_REG_HOST_INTR_CAUSE, intr);

	return (1);
}

void
cmalo_intr_mask(struct malo_softc *sc, int enable)
{
	uint16_t val16;

	val16 = MALO_READ_2(sc, MALO_REG_HOST_INTR_MASK);

	DPRINTF(1, "%s: intr mask changed from 0x%04x ",
	    sc->sc_dev.dv_xname, val16);

	if (enable)
		MALO_WRITE_2(sc, MALO_REG_HOST_INTR_MASK,
		    val16 & ~MALO_VAL_HOST_INTR_MASK_ON);
	else
		MALO_WRITE_2(sc, MALO_REG_HOST_INTR_MASK,
		    val16 | MALO_VAL_HOST_INTR_MASK_ON);

	val16 = MALO_READ_2(sc, MALO_REG_HOST_INTR_MASK);

	DPRINTF(1, "to 0x%04x\n", val16);
}

void
cmalo_rx(struct malo_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &ic->ic_if;
	struct malo_rx_desc *rxdesc;
	struct mbuf *m;
	uint8_t *data;
	uint16_t psize, *uc;
	int i, s;

	s = splnet();

	/* read the whole RX packet which is always 802.3 */
	psize = MALO_READ_2(sc, MALO_REG_DATA_READ_LEN);
	uc = (uint16_t *)sc->sc_data;
	for (i = 0; i < psize / 2; i++)
		uc[i] = htole16(MALO_READ_2(sc, MALO_REG_DATA_READ));
	if (psize & 0x0001)
		uc[i] = MALO_READ_1(sc, MALO_REG_DATA_READ);
	MALO_WRITE_1(sc, MALO_REG_HOST_STATUS, MALO_VAL_RX_DL_OVER);
	MALO_WRITE_2(sc, MALO_REG_CARD_INTR_CAUSE, MALO_VAL_RX_DL_OVER);

	/* access RX packet descriptor */
	rxdesc = (struct malo_rx_desc *)sc->sc_data;
	rxdesc->status = letoh16(rxdesc->status);
	rxdesc->pkglen = letoh16(rxdesc->pkglen);
	rxdesc->pkgoffset = letoh32(rxdesc->pkgoffset);

	DPRINTF(2, "RX status=%d, pkglen=%d, pkgoffset=%d\n",
	    rxdesc->status, rxdesc->pkglen, rxdesc->pkgoffset);

	if (rxdesc->status != MALO_RX_STATUS_OK) {
		/* RX packet is not OK */
		splx(s);
		return;
	}

	/* remove the LLC / SNAP header */
	data = sc->sc_data + rxdesc->pkgoffset;
	i = (ETHER_ADDR_LEN * 2) + sizeof(struct llc);
	bcopy(data + i, data + (ETHER_ADDR_LEN * 2), rxdesc->pkglen - i);
	rxdesc->pkglen -= sizeof(struct llc);

	/* prepare mbuf */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		ifp->if_ierrors++;
		splx(s);
		return;
	}
	MCLGET(m, M_DONTWAIT);
	if (!(m->m_flags & M_EXT)) {
		ifp->if_ierrors++;
		m_freem(m);
		splx(s);
		return;
	}
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = m->m_len = rxdesc->pkglen;
	data = mtod(m, uint8_t *);
	bcopy(sc->sc_data + rxdesc->pkgoffset, data, m->m_pkthdr.len);

#if NBPFILTER > 0
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_IN);
#endif

	/* push the frame up to the network stack if not in monitor mode */
	if (ic->ic_opmode != IEEE80211_M_MONITOR) {
		ether_input_mbuf(ifp, m);
		ifp->if_ipackets++;
	}

	splx(s);
}

void
cmalo_start(struct ifnet *ifp)
{
	struct malo_softc *sc = ifp->if_softc;
	struct mbuf *m;

	/* don't transmit packets if interface is busy or down */
	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	IFQ_POLL(&ifp->if_snd, m);
	if (m == NULL)
		return;

	IFQ_DEQUEUE(&ifp->if_snd, m);

#if NBPFILTER > 0
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_OUT);
#endif

	if (cmalo_tx(sc, m) != 0)
		ifp->if_oerrors++;
}

int
cmalo_tx(struct malo_softc *sc, struct mbuf *m)
{
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	struct malo_tx_desc *txdesc = sc->sc_data;
	uint8_t *data;
	uint16_t psize, *uc;
	int i;

	bzero(sc->sc_data, sizeof(*txdesc));
	psize = sizeof(*txdesc) + m->m_pkthdr.len;
	data = mtod(m, uint8_t *);

	/* prepare TX descriptor */
	txdesc->pkgoffset = htole32(sizeof(*txdesc));
	txdesc->pkglen = htole16(m->m_pkthdr.len);
	bcopy(data, txdesc->dstaddrhigh, ETHER_ADDR_LEN);

	/* copy mbuf data to the buffer */
	m_copydata(m, 0, m->m_pkthdr.len, sc->sc_data + sizeof(*txdesc));
	m_freem(m);

	/* send TX packet to the device */
	MALO_WRITE_2(sc, MALO_REG_DATA_WRITE_LEN, psize);
	uc = (uint16_t *)sc->sc_data;
	for (i = 0; i < psize / 2; i++)
		MALO_WRITE_2(sc, MALO_REG_DATA_WRITE, uc[i]);
	if (psize & 0x0001)
		MALO_WRITE_1(sc, MALO_REG_DATA_WRITE, uc[i]);
	MALO_WRITE_1(sc, MALO_REG_HOST_STATUS, MALO_VAL_TX_DL_OVER);
	MALO_WRITE_2(sc, MALO_REG_CARD_INTR_CAUSE, MALO_VAL_TX_DL_OVER);

	ifp->if_flags |= IFF_OACTIVE;

	DPRINTF(2, "%s: TX status=%d, pkglen=%d, pkgoffset=%d\n",
	    sc->sc_dev.dv_xname, txdesc->status, letoh16(txdesc->pkglen),
	    sizeof(*txdesc));

	return (0);
}

void
cmalo_tx_done(struct malo_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ic.ic_if;

	DPRINTF(2, "%s: TX done\n", sc->sc_dev.dv_xname);

	ifp->if_opackets++;
	ifp->if_flags &= ~IFF_OACTIVE;
	cmalo_start(ifp);
}

void
cmalo_hexdump(void *buf, int len)
{
#ifdef CMALO_DEBUG
	int i;

	if (cmalo_d <= 2) {
		for (i = 0; i < len; i++) {
			if (i % 16 == 0)
				printf("%s%5i:", i ? "\n" : "", i);
			if (i % 4 == 0)
				printf(" ");
			printf("%02x", (int)*((u_char *)buf + i));
		}
		printf("\n");
	}
#endif
}

int
cmalo_cmd_get_hwspec(struct malo_softc *sc)
{
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_spec *body;
	uint16_t psize;

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);
	psize = sizeof(*hdr) + sizeof(*body);

	hdr->cmd = htole16(MALO_CMD_HWSPEC);
	hdr->size = htole16(sizeof(*body));
	hdr->seqnum = htole16(1);
	hdr->result = 0;
	body = (struct malo_cmd_body_spec *)(hdr + 1);

	/* set all bits for MAC address, otherwise we won't get one back */
	memset(body->macaddr, 0xff, ETHER_ADDR_LEN);

	/* process command request */
	if (cmalo_cmd_request(sc, psize, 0) != 0)
		return (EIO);

	/* process command repsonse */
	cmalo_cmd_response(sc);

	return (0);
}

int
cmalo_cmd_rsp_hwspec(struct malo_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_spec *body;
	int i;

	body = (struct malo_cmd_body_spec *)(hdr + 1);

	/* get our MAC address */
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		ic->ic_myaddr[i] = body->macaddr[i];

	return (0);
}

int
cmalo_cmd_set_reset(struct malo_softc *sc)
{
	struct malo_cmd_header *hdr = sc->sc_cmd;
	uint16_t psize;

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);
	psize = sizeof(*hdr);

	hdr->cmd = htole16(MALO_CMD_RESET);
	hdr->size = 0;
	hdr->seqnum = htole16(1);
	hdr->result = 0;

	/* process command request */
	if (cmalo_cmd_request(sc, psize, 1) != 0)
		return (EIO);

	return (0);
}

int
cmalo_cmd_set_scan(struct malo_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_scan *body;
	struct malo_cmd_tlv_ssid *body_ssid;
	struct malo_cmd_tlv_chanlist *body_chanlist;
	struct malo_cmd_tlv_rates *body_rates;
	//struct malo_cmd_tlv_numprobes *body_numprobes;
	uint16_t psize;
	int i;

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);
	psize = sizeof(*hdr) + sizeof(*body);

	hdr->cmd = htole16(MALO_CMD_SCAN);
	hdr->seqnum = htole16(1);
	hdr->result = 0;
	body = (struct malo_cmd_body_scan *)(hdr + 1);

	body->bsstype = 0x03; /* any BSS */
	memset(body->bssid, 0xff, ETHER_ADDR_LEN);

	body_ssid = sc->sc_cmd + psize;
	body_ssid->type = htole16(MALO_TLV_TYPE_SSID);
	body_ssid->size = htole16(0);
	psize += (sizeof(*body_ssid) - 1);

	body_chanlist = sc->sc_cmd + psize;
	body_chanlist->type = htole16(MALO_TLV_TYPE_CHANLIST);
	body_chanlist->size = htole16(sizeof(body_chanlist->data));
	for (i = 0; i < CHANNELS; i++) {
		body_chanlist->data[i].radiotype = 0x00;
		body_chanlist->data[i].channumber = (i + 1);
		body_chanlist->data[i].scantype = 0x00; /* active */
		body_chanlist->data[i].minscantime = htole16(0);
		body_chanlist->data[i].maxscantime = htole16(100);
	}
	psize += sizeof(*body_chanlist);

	body_rates = sc->sc_cmd + psize;
	body_rates->type = htole16(MALO_TLV_TYPE_RATES);
	body_rates->size =
	    htole16(ic->ic_sup_rates[IEEE80211_MODE_11B].rs_nrates);
	bcopy(ic->ic_sup_rates[IEEE80211_MODE_11B].rs_rates, body_rates->data,
	    ic->ic_sup_rates[IEEE80211_MODE_11B].rs_nrates);
	psize += (sizeof(*body_rates) - 1) + body_rates->size;
#if 0
	body_numprobes = sc->sc_cmd + psize;
	body_numprobes->type = htole16(MALO_TLV_TYPE_NUMPROBES);
	body_numprobes->size = htole16(2);
	body_numprobes->numprobes = htole16(1);
	psize += sizeof(*body_numprobes);
#endif
	hdr->size = htole16(psize - sizeof(*hdr));

	/* process command request */
	if (cmalo_cmd_request(sc, psize, 0) != 0)
		return (EIO);

	/* process command repsonse */
	cmalo_cmd_response(sc);

	return (0);
}

int
cmalo_cmd_rsp_scan(struct malo_softc *sc)
{
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_rsp_scan *body;
	struct malo_cmd_body_rsp_scan_set *set;
	uint16_t psize;
	int i;

	bzero(sc->sc_networks, sizeof(sc->sc_networks));
	psize = sizeof(*hdr) + sizeof(*body);

	body = (struct malo_cmd_body_rsp_scan *)(hdr + 1);

	DPRINTF(1, "bufsize=%d, APs=%d\n", body->bufsize, body->numofset);
	sc->sc_networks_num = body->numofset;

	/* cycle through found networks */
	for (i = 0; i < body->numofset; i++) {
		set = (struct malo_cmd_body_rsp_scan_set *)(sc->sc_cmd + psize);

		DPRINTF(1, "size=%d, bssid=%s, rssi=%d, beaconintvl=%d, "
		    "capinfo=0x%04x\n",
		    set->size, ether_sprintf(set->bssid), set->rssi,
		    set->beaconintvl, set->capinfo);

		/* save scan results */
		bcopy(set->bssid, sc->sc_networks[i].bssid, sizeof(set->bssid));
		bcopy(set->timestamp, sc->sc_networks[i].timestamp,
		    sizeof(set->timestamp));
		sc->sc_networks[i].rssi = set->rssi;
		sc->sc_networks[i].beaconintvl = set->beaconintvl;
		sc->sc_networks[i].capinfo = set->capinfo;
		cmalo_parse_elements(sc, (set + 1),
		    set->size - (sizeof(*set) - sizeof(set->size)), i);

		psize += (set->size + sizeof(set->size));
	}

	return (0);
}

int
cmalo_parse_elements(struct malo_softc *sc, void *buf, int size, int pos)
{
	uint8_t eid, len;
	int i;

	DPRINTF(2, "element_size=%d, element_pos=%d\n", size, pos);

	for (i = 0; i < size; ) {
		eid = *(uint8_t *)(buf + i);
		i++;
		len = *(uint8_t *)(buf + i);
		i++;
		DPRINTF(2, "eid=%d, len=%d, ", eid, len);

		switch (eid) {
		case IEEE80211_ELEMID_SSID:
			bcopy(buf + i, sc->sc_networks[pos].ssid, len);
			DPRINTF(2, "ssid=%s\n", sc->sc_networks[pos].ssid);
			break;
		case IEEE80211_ELEMID_RATES:
			bcopy(buf + i, sc->sc_networks[pos].rates, len);
			DPRINTF(2, "rates\n");
			break;
		case IEEE80211_ELEMID_DSPARMS:
			sc->sc_networks[pos].channel = *(uint8_t *)(buf + i);
			DPRINTF(2, "chnl=%d\n", sc->sc_networks[pos].channel);
			break;
		default:
			DPRINTF(2, "unknown\n");
			break;
		}

		i += len;
	}

	return (0);
}

int
cmalo_cmd_set_auth(struct malo_softc *sc)
{
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_auth *body;
	uint16_t psize;

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);
	psize = sizeof(*hdr) + sizeof(*body);

	hdr->cmd = htole16(MALO_CMD_AUTH);
	hdr->size = htole16(sizeof(*body));
	hdr->seqnum = htole16(1);
	hdr->result = 0;
	body = (struct malo_cmd_body_auth *)(hdr + 1);

	bcopy(ap, body->peermac, ETHER_ADDR_LEN);
	body->authtype = 0;

	/* process command request */
	if (cmalo_cmd_request(sc, psize, 0) != 0)
		return (EIO);

	/* process command repsonse */
	cmalo_cmd_response(sc);

	return (0);
}

int
cmalo_cmd_set_snmp(struct malo_softc *sc, uint16_t oid)
{
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_snmp *body;
	uint16_t psize;

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);
	psize = sizeof(*hdr) + sizeof(*body);

	hdr->cmd = htole16(MALO_CMD_SNMP);
	hdr->size = htole16(sizeof(*body));
	hdr->seqnum = htole16(1);
	hdr->result = 0;
	body = (struct malo_cmd_body_snmp *)(hdr + 1);

	body->action = htole16(1);

	switch (oid) {
	case MALO_OID_RTSTRESH:
		body->oid = htole16(MALO_OID_RTSTRESH);
		body->size = htole16(2);
		*(uint16_t *)body->data = htole16(2347);
		break;
	case MALO_OID_SHORTRETRY:
		body->oid = htole16(MALO_OID_SHORTRETRY);
		body->size = htole16(2);
		*(uint16_t *)body->data = htole16(4);
		break;
	case MALO_OID_FRAGTRESH:
		body->oid = htole16(MALO_OID_FRAGTRESH);
		body->size = htole16(2);
		*(uint16_t *)body->data = htole16(2346);
		break;
	case MALO_OID_80211D:
		body->oid = htole16(MALO_OID_80211D);
		body->size = htole16(2);
		*(uint16_t *)body->data = htole16(1);
		break;
	default:
		break;
	}

	/* process command request */
	if (cmalo_cmd_request(sc, psize, 0) != 0)
		return (EIO);

	/* process command repsonse */
	cmalo_cmd_response(sc);

	return (0);
}

int
cmalo_cmd_set_radio(struct malo_softc *sc, uint16_t control)
{
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_radio *body;
	uint16_t psize;

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);
	psize = sizeof(*hdr) + sizeof(*body);

	hdr->cmd = htole16(MALO_CMD_RADIO);
	hdr->size = htole16(sizeof(*body));
	hdr->seqnum = htole16(1);
	hdr->result = 0;
	body = (struct malo_cmd_body_radio *)(hdr + 1);

	body->action = htole16(1);

	if (control) {
		body->control  = htole16(MALO_CMD_RADIO_ON);
		body->control |= htole16(MALO_CMD_RADIO_AUTO_P);
	}

	/* process command request */
	if (cmalo_cmd_request(sc, psize, 0) != 0)
		return (EIO);

	/* process command repsonse */
	cmalo_cmd_response(sc);

	return (0);
}

int
cmalo_cmd_set_channel(struct malo_softc *sc, uint16_t channel)
{
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_channel *body;
	uint16_t psize;

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);
	psize = sizeof(*hdr) + sizeof(*body);

	hdr->cmd = htole16(MALO_CMD_CHANNEL);
	hdr->size = htole16(sizeof(*body));
	hdr->seqnum = htole16(1);
	hdr->result = 0;
	body = (struct malo_cmd_body_channel *)(hdr + 1);

	body->action = htole16(1);
	body->channel = htole16(channel);

	/* process command request */
	if (cmalo_cmd_request(sc, psize, 0) != 0)
		return (EIO);

	/* process command repsonse */
	cmalo_cmd_response(sc);

	return (0);
}


int
cmalo_cmd_set_txpower(struct malo_softc *sc, int16_t txpower)
{
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_txpower *body;
	uint16_t psize;

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);
	psize = sizeof(*hdr) + sizeof(*body);

	hdr->cmd = htole16(MALO_CMD_TXPOWER);
	hdr->size = htole16(sizeof(*body));
	hdr->seqnum = htole16(1);
	hdr->result = 0;
	body = (struct malo_cmd_body_txpower *)(hdr + 1);

	body->action = htole16(1);
	body->txpower = htole16(txpower);

	/* process command request */
	if (cmalo_cmd_request(sc, psize, 0) != 0)
		return (EIO);

	/* process command repsonse */
	cmalo_cmd_response(sc);

	return (0);
}

int
cmalo_cmd_set_antenna(struct malo_softc *sc, uint16_t action)
{
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_antenna *body;
	uint16_t psize;

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);
	psize = sizeof(*hdr) + sizeof(*body);

	hdr->cmd = htole16(MALO_CMD_ANTENNA);
	hdr->size = htole16(sizeof(*body));
	hdr->seqnum = htole16(1);
	hdr->result = 0;
	body = (struct malo_cmd_body_antenna *)(hdr + 1);

	/* 1 = set RX, 2 = set TX */
	body->action = htole16(action);

	if (action == 1)
		/* set RX antenna */
		body->antenna_mode = htole16(0xffff);
	if (action == 2)
		/* set TX antenna */
		body->antenna_mode = htole16(2);

	/* process command request */
	if (cmalo_cmd_request(sc, psize, 0) != 0)
		return (EIO);

	/* process command repsonse */
	cmalo_cmd_response(sc);

	return (0);
}

int
cmalo_cmd_set_macctrl(struct malo_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_macctrl *body;
	uint16_t psize;

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);
	psize = sizeof(*hdr) + sizeof(*body);

	hdr->cmd = htole16(MALO_CMD_MACCTRL);
	hdr->size = htole16(sizeof(*body));
	hdr->seqnum = htole16(1);
	hdr->result = 0;
	body = (struct malo_cmd_body_macctrl *)(hdr + 1);

	body->action  = htole16(MALO_CMD_MACCTRL_RX_ON);
	body->action |= htole16(MALO_CMD_MACCTRL_TX_ON);
	if (ic->ic_opmode == IEEE80211_M_MONITOR)
		body->action |= htole16(MALO_CMD_MACCTRL_PROMISC_ON);

	/* process command request */
	if (cmalo_cmd_request(sc, psize, 0) != 0)
		return (EIO);

	/* process command repsonse */
	cmalo_cmd_response(sc);

	return (0);
}

int
cmalo_cmd_set_assoc(struct malo_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_assoc *body;
	struct malo_cmd_tlv_ssid *body_ssid;
	struct malo_cmd_tlv_phy *body_phy;
	struct malo_cmd_tlv_cf *body_cf;
	struct malo_cmd_tlv_rates *body_rates;
	struct malo_cmd_tlv_passeid *body_passeid;
	uint16_t psize;

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);
	psize = sizeof(*hdr) + sizeof(*body);

	hdr->cmd = htole16(MALO_CMD_ASSOC);
	hdr->seqnum = htole16(1);
	hdr->result = 0;
	body = (struct malo_cmd_body_assoc *)(hdr + 1);

	bcopy(ap, body->peermac, ETHER_ADDR_LEN);
	body->capinfo = htole16(IEEE80211_CAPINFO_ESS);
	body->listenintrv = htole16(10);

	body_ssid = sc->sc_cmd + psize;
	body_ssid->type = htole16(MALO_TLV_TYPE_SSID);
	body_ssid->size = htole16(6);
	bcopy(ssid, body_ssid->data, 6);
	psize += (sizeof(*body_ssid) - 1) + body_ssid->size;

	body_phy = sc->sc_cmd + psize;
	body_phy->type = htole16(MALO_TLV_TYPE_PHY);
	body_phy->size = htole16(1);
	bcopy(chan, body_phy->data, 1);
	psize += sizeof(*body_phy);

	body_cf = sc->sc_cmd + psize;
	body_cf->type = htole16(MALO_TLV_TYPE_CF);
	body_cf->size = htole16(0);
	psize += (sizeof(*body_cf) - 1);

	body_rates = sc->sc_cmd + psize;
	body_rates->type = htole16(MALO_TLV_TYPE_RATES);
	body_rates->size =
	    htole16(ic->ic_sup_rates[IEEE80211_MODE_11B].rs_nrates);
	bcopy(ic->ic_sup_rates[IEEE80211_MODE_11B].rs_rates, body_rates->data,
	    ic->ic_sup_rates[IEEE80211_MODE_11B].rs_nrates);
	psize += (sizeof(*body_rates) - 1) + body_rates->size;

	/* hack to correct FW's wrong generated rates-element-id */
	body_passeid = sc->sc_cmd + psize;
	body_passeid->type = htole16(MALO_TLV_TYPE_PASSEID);
	body_passeid->size = htole16(6);
	bcopy(rates, body_passeid->data, 6);
	psize += (sizeof(*body_passeid) - 1) + body_passeid->size;

	hdr->size = htole16(psize - sizeof(*hdr));

	/* process command request */
	if (cmalo_cmd_request(sc, psize, 0) != 0)
		return (EIO);

	/* process command repsonse */
	cmalo_cmd_response(sc);

	return (0);
}

int
cmalo_cmd_set_80211d(struct malo_softc *sc)
{
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_80211d *body;
	struct malo_cmd_tlv_80211d *body_80211d;
	uint16_t psize;
	int i;

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);
	psize = sizeof(*hdr) + sizeof(*body);

	hdr->cmd = htole16(MALO_CMD_80211D);
	hdr->seqnum = htole16(1);
	hdr->result = 0;
	body = (struct malo_cmd_body_80211d *)(hdr + 1);

	body->action = htole16(1);

	body_80211d = sc->sc_cmd + psize;
	body_80211d->type = htole16(MALO_TLV_TYPE_80211D);
	body_80211d->size = htole16(sizeof(body_80211d->data) +
	    sizeof(body_80211d->countrycode));
	bcopy("EU ", body_80211d->countrycode,
	    sizeof(body_80211d->countrycode));
	for (i = 0; i < CHANNELS; i++) {
		body_80211d->data[i].firstchannel = 1;
		body_80211d->data[i].numchannels = 12;
		body_80211d->data[i].maxtxpower = 10;
	}
	psize += sizeof(*body_80211d);

	hdr->size = htole16(psize - sizeof(*hdr));
	
	/* process command request */
	if (cmalo_cmd_request(sc, psize, 0) != 0)
		return (EIO);

	/* process command repsonse */
	cmalo_cmd_response(sc);

	return (0);
}

int
cmalo_cmd_set_bgscan_config(struct malo_softc *sc)
{
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_bgscan_config *body;
	uint16_t psize;

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);
	psize = sizeof(*hdr) + sizeof(*body);

	hdr->cmd = htole16(MALO_CMD_BGSCAN_CONFIG);
	hdr->size = htole16(sizeof(*body));
	hdr->seqnum = htole16(1);
	hdr->result = 0;
	body = (struct malo_cmd_body_bgscan_config *)(hdr + 1);

	body->action = htole16(1);
	body->enable = 1;
	body->bsstype = 0x03;
	body->chperscan = 12;
	body->scanintvl = htole32(100);
	body->maxscanres = htole16(12);

	/* process command request */
	if (cmalo_cmd_request(sc, psize, 0) != 0)
		return (EIO);

	/* process command repsonse */
	cmalo_cmd_response(sc);

	return (0);
}

int
cmalo_cmd_set_bgscan_query(struct malo_softc *sc)
{
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_bgscan_query *body;
	uint16_t psize;

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);
	psize = sizeof(*hdr) + sizeof(*body);

	hdr->cmd = htole16(MALO_CMD_BGSCAN_QUERY);
	hdr->size = htole16(sizeof(*body));
	hdr->seqnum = htole16(1);
	hdr->result = 0;
	body = (struct malo_cmd_body_bgscan_query *)(hdr + 1);

	body->flush = 0;

	/* process command request */
	if (cmalo_cmd_request(sc, psize, 0) != 0)
		return (EIO);

	/* process command repsonse */
	cmalo_cmd_response(sc);

	return (0);
}

int
cmalo_cmd_set_rate(struct malo_softc *sc)
{
	struct malo_cmd_header *hdr = sc->sc_cmd;
	struct malo_cmd_body_rate *body;
	uint16_t psize;

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);
	psize = sizeof(*hdr) + sizeof(*body);

	hdr->cmd = htole16(MALO_CMD_RATE);
	hdr->size = htole16(sizeof(*body));
	hdr->seqnum = htole16(1);
	hdr->result = 0;
	body = (struct malo_cmd_body_rate *)(hdr + 1);

	body->action = htole16(1);
	body->hwauto = htole16(1);
	body->ratebitmap = htole16(0x1fff);

	/* process command request */
	if (cmalo_cmd_request(sc, psize, 0) != 0)
		return (EIO);

	/* process command repsonse */
	cmalo_cmd_response(sc);

	return (0);
}

int
cmalo_cmd_request(struct malo_softc *sc, uint16_t psize, int no_response)
{
	uint16_t *uc;
	int i;

	cmalo_hexdump(sc->sc_cmd, psize);

	/* send command request */
	MALO_WRITE_2(sc, MALO_REG_CMD_WRITE_LEN, psize);
	uc = (uint16_t *)sc->sc_cmd;
	for (i = 0; i < psize / 2; i++)
		MALO_WRITE_2(sc, MALO_REG_CMD_WRITE, htole16(uc[i]));
	if (psize & 0x0001)
		MALO_WRITE_1(sc, MALO_REG_CMD_WRITE, htole16(uc[i]));
	MALO_WRITE_1(sc, MALO_REG_HOST_STATUS, MALO_VAL_CMD_DL_OVER);
	MALO_WRITE_2(sc, MALO_REG_CARD_INTR_CAUSE, MALO_VAL_CMD_DL_OVER);

	if (no_response)
		/* we don't expect a response */
		return (0);

	/* wait for the command response */
	sc->sc_cmd_running = 1;
	for (i = 0; i < 1000; i++) {
		if (sc->sc_cmd_running == 0)
			break;
		tsleep(sc, 0, "malocmd", 1);
	}
	if (sc->sc_cmd_running) {
		printf("%s: timeout while waiting for cmd response!\n",
		    sc->sc_dev.dv_xname);
		return (EIO);
	}

	return (0);
}

int
cmalo_cmd_response(struct malo_softc *sc)
{
	struct malo_cmd_header *hdr = sc->sc_cmd;
	uint16_t psize, *uc;
	int i, s;

	s = splnet(); /* XXX */

	bzero(sc->sc_cmd, MALO_CMD_BUFFER_SIZE);

	/* read the whole command response */
	psize = MALO_READ_2(sc, MALO_REG_CMD_READ_LEN);
	uc = (uint16_t *)sc->sc_cmd;
	for (i = 0; i < psize / 2; i++)
		uc[i] = htole16(MALO_READ_2(sc, MALO_REG_CMD_READ));

	cmalo_hexdump(sc->sc_cmd, psize);

	/*
	 * We convert the header values into the machines correct endianess,
	 * so we don't have to letoh16() all over the code.  The body is
	 * kept in the cards order, little endian.  We need to take care
	 * about the body endianess in the corresponding response routines.
	 */
	hdr->cmd = letoh16(hdr->cmd);
	hdr->size = letoh16(hdr->size);
	hdr->seqnum = letoh16(hdr->seqnum);
	hdr->result = letoh16(hdr->result);

	/* check for a valid command response */
	if (!(hdr->cmd & MALO_CMD_RESP)) {
		printf("%s: got invalid command response (0x%04x)!\n",
		    sc->sc_dev.dv_xname, hdr->cmd);
		return (EIO);
	}
	hdr->cmd &= ~MALO_CMD_RESP;

	/* to which command does the response belong */
	switch (hdr->cmd) {
	case MALO_CMD_HWSPEC:
		DPRINTF(1, "%s: got hwspec cmd response\n",
		    sc->sc_dev.dv_xname);
		cmalo_cmd_rsp_hwspec(sc);
		break;
	case MALO_CMD_RESET:
		/* reset will not send back a response */
		break;
	case MALO_CMD_SCAN:
		DPRINTF(1, "%s: got scan cmd response\n",
		    sc->sc_dev.dv_xname);
		cmalo_cmd_rsp_scan(sc);
		break;
	case MALO_CMD_AUTH:
		/* do nothing */
		DPRINTF(1, "%s: got auth cmd response\n",
		    sc->sc_dev.dv_xname);
		break;
	case MALO_CMD_SNMP:
		/* do nothing */
		DPRINTF(1, "%s: got snmp cmd response\n",
		    sc->sc_dev.dv_xname);
		break;
	case MALO_CMD_RADIO:
		/* do nothing */
		DPRINTF(1, "%s: got radio cmd response\n",
		    sc->sc_dev.dv_xname);
		break;
	case MALO_CMD_CHANNEL:
		/* do nothing */
		DPRINTF(1, "%s: got channel cmd response\n",
		    sc->sc_dev.dv_xname);
		break;
	case MALO_CMD_TXPOWER:
		/* do nothing */
		DPRINTF(1, "%s: got txpower cmd response\n",
		    sc->sc_dev.dv_xname);
		break;
	case MALO_CMD_ANTENNA:
		/* do nothing */
		DPRINTF(1, "%s: got antenna cmd response\n",
		    sc->sc_dev.dv_xname);
		break;
	case MALO_CMD_MACCTRL:
		/* do nothing */
		DPRINTF(1, "%s: got macctrl cmd response\n",
		    sc->sc_dev.dv_xname);
		break;
	case MALO_CMD_80211D:
		/* do nothing */
		DPRINTF(1, "%s: got 80211d cmd response\n",
		    sc->sc_dev.dv_xname);
		break;
	case MALO_CMD_BGSCAN_CONFIG:
		/* do nothing */
		DPRINTF(1, "%s: got bgscan config cmd response\n",
		    sc->sc_dev.dv_xname);
		break;
	case MALO_CMD_BGSCAN_QUERY:
		/* do nothing */
		DPRINTF(1, "%s: got bgscan query cmd response\n",
		    sc->sc_dev.dv_xname);
		break;
	case MALO_CMD_RATE:
		/* do nothing */
		DPRINTF(1, "%s: got rate cmd response\n",
		    sc->sc_dev.dv_xname);
		break;
	default:
		printf("%s: got unknown cmd response (0x%04x)!\n",
		    sc->sc_dev.dv_xname, hdr->cmd);
		break;
	}

	splx(s);

	return (0);
}
