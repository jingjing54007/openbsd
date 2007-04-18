/*	$OpenBSD: src/sys/dev/pci/if_tht.c,v 1.19 2007/04/18 02:12:30 dlg Exp $ */

/*
 * Copyright (c) 2007 David Gwynne <dlg@openbsd.org>
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

/*
 * Driver for the Tehuti TN30xx multi port 10Gb Ethernet chipsets,
 * see http://www.tehutinetworks.net/.
 *
 * This driver was made possible because Tehuti networks provided
 * hardware and documentation. Thanks!
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <machine/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_ether.h>
#endif

/* registers */

#define THT_PCI_BAR		0x10

#define _Q(_q)			((_q) * 4)

/* General Configuration */
#define THT_REG_END_SEL		0x5448 /* PCI Endian Select */
#define THT_REG_CLKPLL		0x5000
/* Descriptors and FIFO Registers */
#define THT_REG_TXT_CFG0(_q)	(0x4040 + _Q(_q)) /* CFG0 TX Task queues */
#define THT_REG_RXF_CFG0(_q)	(0x4050 + _Q(_q)) /* CFG0 RX Free queues */
#define THT_REG_RXD_CFG0(_q)	(0x4060 + _Q(_q)) /* CFG0 RX DSC queues */
#define THT_REG_TXF_CFG0(_q)	(0x4070 + _Q(_q)) /* CFG0 TX Free queues */
#define THT_REG_TXT_CFG1(_q)	(0x4000 + _Q(_q)) /* CFG1 TX Task queues */
#define THT_REG_RXF_CFG1(_q)	(0x4010 + _Q(_q)) /* CFG1 RX Free queues */
#define THT_REG_RXD_CFG1(_q)	(0x4020 + _Q(_q)) /* CFG1 RX DSC queues */
#define THT_REG_TXF_CFG1(_q)	(0x4030 + _Q(_q)) /* CFG1 TX Free queues */
#define THT_REG_TXT_RPTR(_q)	(0x40c0 + _Q(_q)) /* TX Task read ptr */
#define THT_REG_RXF_RPTR(_q)	(0x40d0 + _Q(_q)) /* RX Free read ptr */
#define THT_REG_RXD_RPTR(_q)	(0x40e0 + _Q(_q)) /* RX DSC read ptr */
#define THT_REG_TXF_RPTR(_q)	(0x40f0 + _Q(_q)) /* TX Free read ptr */
#define THT_REG_TXT_WPTR(_q)	(0x4080 + _Q(_q)) /* TX Task write ptr */
#define THT_REG_RXF_WPTR(_q)	(0x4090 + _Q(_q)) /* RX Free write ptr */
#define THT_REG_RXD_WPTR(_q)	(0x40a0 + _Q(_q)) /* RX DSC write ptr */
#define THT_REG_TXF_WPTR(_q)	(0x40b0 + _Q(_q)) /* TX Free write ptr */
#define THT_REG_HTB_ADDR	0x4100 /* HTB Addressing Mechanism enable */
#define THT_REG_HTB_ADDR_HI	0x4110 /* High HTB Address */
#define THT_REG_HTB_ST_TMR	0x3290 /* HTB Timer */
#define THT_REG_RDINTCM(_q)	(0x0120 + _Q(_q)) /* RX DSC Intr Coalescing */
#define THT_REG_TDINTCM(_q)	(0x0120 + _Q(_q)) /* TX DSC Intr Coalescing */
/* 10G Ethernet MAC */
#define THT_REG_10G_REV		0x6000 /* Revision */
#define THT_REG_10G_SCR		0x6004 /* Scratch */
#define THT_REG_10G_CTL		0x6008 /* Control/Status */
#define THT_REG_10G_FRM_LEN	0x6014 /* Fram Length */
#define THT_REG_10G_PAUSE	0x6018 /* Pause Quanta */
#define THT_REG_10G_RX_SEC	0x601c /* RX Section */
#define THT_REG_10G_TX_SEC	0x6020 /* TX Section */
#define THT_REG_10G_RFIFO_AEF	0x6024 /* RX FIFO Almost Empty/Full */
#define THT_REG_10G_TFIFO_AEF	0x6028 /* TX FIFO Almost Empty/Full */
#define THT_REG_10G_SM_STAT	0x6030 /* MDIO Status */
#define THT_REG_10G_SM_CMD	0x6034 /* MDIO Command */
#define THT_REG_10G_SM_DAT	0x6038 /* MDIO Data */
#define THT_REG_10G_SM_ADD	0x603c /* MDIO Address */
#define THT_REG_10G_STAT	0x6040 /* Status */
/* Statistic Counters */
/* XXX todo */
/* Status Registers */
#define THT_REG_MAC_LNK_STAT	0x0200 /* Link Status */
/* Interrupt Registers */
#define THT_REG_ISR		0x5100 /* Interrupt Status */
#define THT_REG_ISR_GTI		0x5080 /* GTI Interrupt Status */
#define THT_REG_IMR		0x5110 /* Interrupt Mask */
#define THT_REG_IMR_GTI		0x5090 /* GTI Interrupt Mask */
#define THT_REG_ISR_MSK		0x5140 /* ISR Masked */
/* Global Counters */
/* XXX todo */
/* DDR2 SDRAM Controller Registers */
/* XXX TBD */
/* EEPROM Registers */
/* XXX todo */
/* Init arbitration and status registers */
#define THT_REG_INIT_SEMAPHORE	0x5170 /* Init Semaphore */
#define THT_REG_INIT_STATUS	0x5180 /* Init Status */
/* PCI Credits Registers */
/* XXX todo */
/* TX Arbitration Registers */
#define THT_REG_TXTSK_PR(_q)	(0x41b0 + _Q(_q)) /* TX Queue Priority */
/* RX Part Registers */
#define THT_REG_RX_FLT		0x1240 /* RX Filter Configuration */
#define  THT_REG_RX_FLT_ATXER		(1<<15)
#define  THT_REG_RX_FLT_ATRM		(1<<14)
#define  THT_REG_RX_FLT_AFTSQ		(1<<13)
#define  THT_REG_RX_FLT_OSEN		(1<<12)
#define  THT_REG_RX_FLT_APHER		(1<<11)
#define  THT_REG_RX_FLT_TXFC		(1<<10) /* TX flow control */
#define  THT_REG_RX_FLT_FDA		(1<<8) /* filter direct address */
#define  THT_REG_RX_FLT_AOF		(1<<7) /* accept overflow frame */
#define  THT_REG_RX_FLT_ACF		(1<<6) /* accept control frame */
#define  THT_REG_RX_FLT_ARUNT		(1<<5) /* accept runt */
#define  THT_REG_RX_FLT_ACRC		(1<<4) /* accept crc error */
#define  THT_REG_RX_FLT_AM		(1<<3) /* accept multicast */
#define  THT_REG_RX_FLT_AB		(1<<2) /* accept broadcast */
#define  THT_REG_RX_FLT_PRM_MASK	0x3 /* promiscuous mode */
#define  THT_REG_RX_FLT_PRM_NORMAL	0x0 /* normal mode */
#define  THT_REG_RX_FLT_PRM_ALL		0x1 /* pass all incoming frames */
#define THT_REG_RX_MAX_FRAME	0x12c0 /* Max Frame Size */
#define THT_REG_RX_UNC_MAC0	0x1250 /* MAC Address low word */
#define THT_REG_RX_UNC_MAC1	0x1260 /* MAC Address mid word */
#define THT_REG_RX_UNC_MAC2	0x1270 /* MAC Address high word */
/* SW Reset Registers */
#define THT_REG_RST_PRT		0x7000 /* Reset Port */
#define THT_REG_DIS_PRT		0x7010 /* Disable Port */
#define THT_REG_RST_QU_0	0x7020 /* Reset Queue 0 */
#define THT_REG_RST_QU_1	0x7028 /* Reset Queue 1 */
#define THT_REG_DIS_QU_0	0x7030 /* Disable Queue 0 */
#define THT_REG_DIS_QU_1	0x7038 /* Disable Queue 1 */

#define THT_PORT_SIZE		0x8000
#define THT_PORT_REGION(_p)	((_p) * THT_PORT_SIZE)

/* hardware structures (we're using the 64 bit variants) */

/* physical buffer descriptor */
struct tht_pbd {
	u_int32_t		addr_lo;
	u_int32_t		addr_hi;
	u_int32_t		len;
} __packed;

/* rx free fifo */
struct tht_rxf {
	u_int32_t		flags;

	u_int32_t		uid_lo;
	u_int32_t		uid_hi;

	/* followed by a pdb list */
} __packed;

/* rx descriptor type 2 */
struct tht_rx_desc {
	u_int32_t		flags;
	u_int16_t		len;
	u_int16_t		vlan;

	u_int32_t		uid_lo;
	u_int32_t		uid_hi;
} __packed;

/* rx decriptor type 3: data chain instruction */
struct tht_rx_desc_dc {
	/* preceded by tht_rx_desc */

	u_int16_t		cd_offset;
	u_int16_t		flags;

	u_int8_t		data[4];
} __packed;

/* rx descriptor type 4: rss information */
struct tht_rx_desc_rss {
	/* preceded by tht_rx_desc */

	u_int8_t		rss_hft;
	u_int8_t		rss_type;
	u_int8_t		rss_tcpu;
	u_int8_t		reserved;

	u_int32_t		rss_hash;
} __packed;

/* TX_TASK FIFO */
struct tht_tx_task {
	u_int32_t		flags;
	u_int16_t		mss_mtu;
	u_int16_t		len;

	u_int32_t		uid_lo;
	u_int32_t		uid_hi;

	/* followed by a pbd list */
} __packed;

/* TX_FREE FIFO */
struct tht_tx_free {
	u_int32_t		status;

	u_int32_t		uid_lo;
	u_int32_t		uid_hi;

	u_int32_t		pad;
} __packed;

/* pci controller autoconf glue */

struct thtc_softc {
	struct device		sc_dev;

	bus_dma_tag_t		sc_dmat;

	bus_space_tag_t		sc_memt;
	bus_space_handle_t	sc_memh;
	bus_size_t		sc_mems;
};

int			thtc_match(struct device *, void *, void *);
void			thtc_attach(struct device *, struct device *, void *);
int			thtc_print(void *, const char *);

struct cfattach thtc_ca = {
	sizeof(struct thtc_softc), thtc_match, thtc_attach
};

struct cfdriver thtc_cd = {
	NULL, "thtc", DV_DULL
};

/* port autoconf glue */

struct tht_softc {
	struct device		sc_dev;
	struct thtc_softc	*sc_thtc;

	void			*sc_ih;

	bus_space_handle_t	sc_memh;

	struct arpcom		sc_ac;
	struct ifmedia		sc_media;

	u_int16_t		sc_lladdr[3];
};

int			tht_match(struct device *, void *, void *);
void			tht_attach(struct device *, struct device *, void *);
int			tht_intr(void *);

struct cfattach tht_ca = {
	sizeof(struct tht_softc), tht_match, tht_attach
};

struct cfdriver tht_cd = {
	NULL, "tht", DV_IFNET
};

/* glue between the controller and the port */

struct tht_attach_args {
	int			taa_port;

	struct pci_attach_args	*taa_pa;
	pci_intr_handle_t	taa_ih;
};

/* port operations */
int			tht_ioctl(struct ifnet *, u_long, caddr_t);
void			tht_start(struct ifnet *);
void			tht_watchdog(struct ifnet *);
int			tht_media_change(struct ifnet *);
void			tht_media_status(struct ifnet *, struct ifmediareq *);
void			tht_read_lladdr(struct tht_softc *);

/* bus space operations */
u_int32_t		tht_read(struct tht_softc *, bus_size_t);
void			tht_write(struct tht_softc *, bus_size_t, u_int32_t);

/* misc */
#define DEVNAME(_sc)	((_sc)->sc_dev.dv_xname)
#define sizeofa(_a)	(sizeof(_a) / sizeof((_a)[0]))

struct thtc_device {
	pci_vendor_id_t		td_vendor;
	pci_vendor_id_t		td_product;
	u_int			td_nports;
};

const struct thtc_device *thtc_lookup(struct pci_attach_args *);

static const struct thtc_device thtc_devices[] = {
	{ PCI_VENDOR_TEHUTI,	PCI_PRODUCT_TEHUTI_TN3009, 1 },
	{ PCI_VENDOR_TEHUTI,	PCI_PRODUCT_TEHUTI_TN3010, 1 },
	{ PCI_VENDOR_TEHUTI,	PCI_PRODUCT_TEHUTI_TN3014, 2 }
};

const struct thtc_device *
thtc_lookup(struct pci_attach_args *pa)
{
	int				i;
	const struct thtc_device	*td;

	for (i = 0; i < sizeofa(thtc_devices); i++) {
		td = &thtc_devices[i];
		if (td->td_vendor == PCI_VENDOR(pa->pa_id) &&
		    td->td_product == PCI_PRODUCT(pa->pa_id))
			return (td);
	}

	return (NULL);
}

int
thtc_match(struct device *parent, void *match, void *aux)
{
	struct pci_attach_args		*pa = aux;

	if (thtc_lookup(pa) != NULL)
		return (1);

	return (0);
}

void
thtc_attach(struct device *parent, struct device *self, void *aux)
{
	struct thtc_softc		*sc = (struct thtc_softc *)self;
	struct pci_attach_args		*pa = aux;
	pcireg_t			memtype;
	const struct thtc_device	*td;
	struct tht_attach_args		taa;
	int				i;

	bzero(&taa, sizeof(taa));
	td = thtc_lookup(pa);

	sc->sc_dmat = pa->pa_dmat;

	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, THT_PCI_BAR);
	if (pci_mapreg_map(pa, THT_PCI_BAR, memtype, 0, &sc->sc_memt,
	    &sc->sc_memh, NULL, &sc->sc_mems, 0) != 0) {
		printf(": unable to map host registers\n");
		return;
	}

	if (pci_intr_map(pa, &taa.taa_ih) != 0) {
		printf(": unable to map interrupt\n");
		goto unmap;
	}
	printf(": %s\n", pci_intr_string(pa->pa_pc, taa.taa_ih));

	taa.taa_pa = pa;
	for (i = 0; i < td->td_nports; i++) {
		taa.taa_port = i;

		config_found(self, &taa, thtc_print);
	}

	return;

unmap:
	bus_space_unmap(sc->sc_memt, sc->sc_memh, sc->sc_mems);
	sc->sc_mems = 0;
}

int
thtc_print(void *aux, const char *pnp)
{
	struct tht_attach_args		*taa = aux;

	if (pnp != NULL)
		printf("\"%s\" at %s", tht_cd.cd_name, pnp);

	printf(" port %d", taa->taa_port);

	return (UNCONF);
}

int
tht_match(struct device *parent, void *match, void *aux)
{
	return (1);
}

void
tht_attach(struct device *parent, struct device *self, void *aux)
{
	struct thtc_softc		*csc = (struct thtc_softc *)parent;
	struct tht_softc		*sc = (struct tht_softc *)self;
	struct tht_attach_args		*taa = aux;
	struct ifnet			*ifp;

	sc->sc_thtc = csc;

	if (bus_space_subregion(csc->sc_memt, csc->sc_memh,
	    THT_PORT_REGION(taa->taa_port), THT_PORT_SIZE,
	    &sc->sc_memh) != 0) {
		printf(": unable to map port registers\n");
		return;
	}

	sc->sc_ih = pci_intr_establish(taa->taa_pa->pa_pc, taa->taa_ih,
	    IPL_NET, tht_intr, sc, DEVNAME(sc));
	if (sc->sc_ih == NULL) {
		printf(": unable to establish interrupt\n");
		/* bus_space(9) says we dont have to free subregions */
		return;
	}

	tht_read_lladdr(sc);
	bcopy(sc->sc_lladdr, sc->sc_ac.ac_enaddr, ETHER_ADDR_LEN);

	ifp = &sc->sc_ac.ac_if;
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_capabilities = IFCAP_VLAN_MTU;
	ifp->if_ioctl = tht_ioctl;
	ifp->if_start = tht_start;
	ifp->if_watchdog = tht_watchdog;
	ifp->if_hardmtu = 1500; /* XXX */
	strlcpy(ifp->if_xname, DEVNAME(sc), IFNAMSIZ);
	IFQ_SET_MAXLEN(&ifp->if_snd, 400);
	IFQ_SET_READY(&ifp->if_snd);

	ifmedia_init(&sc->sc_media, 0, tht_media_change, tht_media_status);
	ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_AUTO, 0, NULL);
	ifmedia_set(&sc->sc_media, IFM_ETHER|IFM_AUTO);

	if_attach(ifp);
	ether_ifattach(ifp);

	printf(" address %s\n", ether_sprintf(sc->sc_ac.ac_enaddr));
}

int
tht_intr(void *arg)
{
	return (0);
}

int
tht_ioctl(struct ifnet *ifp, u_long cmd, caddr_t addr)
{
	struct tht_softc		*sc = ifp->if_softc;
	struct ifreq			*ifr = (struct ifreq *)addr;
	int				error;
	int				s;

	s = splnet();

	error = ether_ioctl(ifp, &sc->sc_ac, cmd, addr);
	if (error > 0) {
		splx(s);
		return (error);
	}

	switch (cmd) {
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, cmd);
		break;

	default:
		error = ENOTTY;
		break;
	}

	splx(s);

	return (error);
}

void
tht_start(struct ifnet *ifp)
{
	/* do nothing */
}

void
tht_watchdog(struct ifnet *ifp)
{
	/* do nothing */
}

int
tht_media_change(struct ifnet *ifp)
{
	/* ignore */
	return (0);
}

void
tht_media_status(struct ifnet *ifp, struct ifmediareq *imr)
{
	imr->ifm_active = IFM_ETHER | IFM_AUTO;
	imr->ifm_status = IFM_AVALID;

	/* TODO: check link state */
}

void
tht_read_lladdr(struct tht_softc *sc)
{
	const static bus_size_t		r[3] = {
	    THT_REG_RX_UNC_MAC2, THT_REG_RX_UNC_MAC1, THT_REG_RX_UNC_MAC0
	};
	int				i;

	for (i = 0; i < sizeofa(r); i++)
		sc->sc_lladdr[i] = swap16(tht_read(sc, r[i]));
}

u_int32_t
tht_read(struct tht_softc *sc, bus_size_t r)
{
	bus_space_barrier(sc->sc_thtc->sc_memt, sc->sc_memh, r, 4,
	    BUS_SPACE_BARRIER_READ);
	return (bus_space_read_4(sc->sc_thtc->sc_memt, sc->sc_memh, r));
}

void
tht_write(struct tht_softc *sc, bus_size_t r, u_int32_t v)
{
	bus_space_write_4(sc->sc_thtc->sc_memt, sc->sc_memh, r, v);
	bus_space_barrier(sc->sc_thtc->sc_memt, sc->sc_memh, r, 4,
	    BUS_SPACE_BARRIER_WRITE);
}
