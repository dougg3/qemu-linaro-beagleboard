/*
 * TI OMAP L4 interconnect emulation.
 *
 * Copyright (C) 2007-2009 Nokia Corporation
 * Written by Andrzej Zaborowski <andrew@openedhand.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 or
 * (at your option) any later version of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#include "hw.h"
#include "omap.h"

/* Define to prevent registering L4 TA regions for OMAP3 */
#define OMAP3_REDUCE_IOREGIONS

/* Define to group all L4 regions together as one I/O region */
/*# define L4_MUX_HACK*/

/* L4 Interconnect */
#ifdef L4_MUX_HACK
static int omap_l4_io_entries;
static int omap_cpu_io_entry;
static struct omap_l4_entry {
    CPUReadMemoryFunc * const *mem_read;
    CPUWriteMemoryFunc * const *mem_write;
    void *opaque;
} *omap_l4_io_entry;
static CPUReadMemoryFunc * const *omap_l4_io_readb_fn;
static CPUReadMemoryFunc * const *omap_l4_io_readh_fn;
static CPUReadMemoryFunc * const *omap_l4_io_readw_fn;
static CPUWriteMemoryFunc * const *omap_l4_io_writeb_fn;
static CPUWriteMemoryFunc * const *omap_l4_io_writeh_fn;
static CPUWriteMemoryFunc * const *omap_l4_io_writew_fn;
static void **omap_l4_io_opaque;

int l4_register_io_memory(CPUReadMemoryFunc * const *mem_read,
                          CPUWriteMemoryFunc * const *mem_write,
                          void *opaque)
{
    omap_l4_io_entry[omap_l4_io_entries].mem_read = mem_read;
    omap_l4_io_entry[omap_l4_io_entries].mem_write = mem_write;
    omap_l4_io_entry[omap_l4_io_entries].opaque = opaque;

    return omap_l4_io_entries ++;
}

static uint32_t omap_l4_io_readb(void *opaque, target_phys_addr_t addr)
{
    unsigned int i = (addr - OMAP2_L4_BASE) >> TARGET_PAGE_BITS;

    return omap_l4_io_readb_fn[i](omap_l4_io_opaque[i], addr);
}

static uint32_t omap_l4_io_readh(void *opaque, target_phys_addr_t addr)
{
    unsigned int i = (addr - OMAP2_L4_BASE) >> TARGET_PAGE_BITS;

    return omap_l4_io_readh_fn[i](omap_l4_io_opaque[i], addr);
}

static uint32_t omap_l4_io_readw(void *opaque, target_phys_addr_t addr)
{
    unsigned int i = (addr - OMAP2_L4_BASE) >> TARGET_PAGE_BITS;

    return omap_l4_io_readw_fn[i](omap_l4_io_opaque[i], addr);
}

static void omap_l4_io_writeb(void *opaque, target_phys_addr_t addr,
                              uint32_t value)
{
    unsigned int i = (addr - OMAP2_L4_BASE) >> TARGET_PAGE_BITS;

    return omap_l4_io_writeb_fn[i](omap_l4_io_opaque[i], addr, value);
}

static void omap_l4_io_writeh(void *opaque, target_phys_addr_t addr,
                              uint32_t value)
{
    unsigned int i = (addr - OMAP2_L4_BASE) >> TARGET_PAGE_BITS;

    return omap_l4_io_writeh_fn[i](omap_l4_io_opaque[i], addr, value);
}

static void omap_l4_io_writew(void *opaque, target_phys_addr_t addr,
                              uint32_t value)
{
    unsigned int i = (addr - OMAP2_L4_BASE) >> TARGET_PAGE_BITS;

    return omap_l4_io_writew_fn[i](omap_l4_io_opaque[i], addr, value);
}

static CPUReadMemoryFunc * const omap_l4_io_readfn[] = {
    omap_l4_io_readb,
    omap_l4_io_readh,
    omap_l4_io_readw,
};

static CPUWriteMemoryFunc * const omap_l4_io_writefn[] = {
    omap_l4_io_writeb,
    omap_l4_io_writeh,
    omap_l4_io_writew,
};
#else
int l4_register_io_memory(CPUReadMemoryFunc * const *mem_read,
                          CPUWriteMemoryFunc * const *mem_write,
                          void *opaque)
{
    return cpu_register_io_memory(mem_read, mem_write, opaque,
                                  DEVICE_NATIVE_ENDIAN);
}
#endif

struct omap_l4_s {
    target_phys_addr_t base;
    int ta_num;
    struct omap_target_agent_s ta[0];
};

struct omap_l4_s *omap_l4_init(target_phys_addr_t base, int ta_num,
                               int region_count)
{
    struct omap_l4_s *bus = qemu_mallocz(sizeof(*bus) +
                                         ta_num * sizeof(*bus->ta));

    bus->ta_num = ta_num;
    bus->base = base;

#ifdef L4_MUX_HACK
    omap_l4_io_entries = 1;
    omap_l4_io_entry = qemu_mallocz(region_count * sizeof(*omap_l4_io_entry));
    omap_cpu_io_entry = cpu_register_io_memory(omap_l4_io_readfn,
                                               omap_l4_io_writefn, bus,
                                               DEVICE_NATIVE_ENDIAN);
# define L4_PAGES	(0xb4000 / TARGET_PAGE_SIZE)
    omap_l4_io_readb_fn = qemu_mallocz(sizeof(void *) * L4_PAGES);
    omap_l4_io_readh_fn = qemu_mallocz(sizeof(void *) * L4_PAGES);
    omap_l4_io_readw_fn = qemu_mallocz(sizeof(void *) * L4_PAGES);
    omap_l4_io_writeb_fn = qemu_mallocz(sizeof(void *) * L4_PAGES);
    omap_l4_io_writeh_fn = qemu_mallocz(sizeof(void *) * L4_PAGES);
    omap_l4_io_writew_fn = qemu_mallocz(sizeof(void *) * L4_PAGES);
    omap_l4_io_opaque = qemu_mallocz(sizeof(void *) * L4_PAGES);
#endif

    return bus;
}

target_phys_addr_t omap_l4_attach(struct omap_target_agent_s *ta, int region,
                                  int iotype)
{
    target_phys_addr_t base;
    ssize_t size;
#ifdef L4_MUX_HACK
    int i;
#endif

    if (region < 0 || region >= ta->regions) {
        fprintf(stderr, "%s: bad io region (%i)\n", __FUNCTION__, region);
        exit(-1);
    }

    base = ta->bus->base + ta->start[region].offset;
    size = ta->start[region].size;
    if (iotype) {
#ifndef L4_MUX_HACK
        cpu_register_physical_memory(base, size, iotype);
#else
        cpu_register_physical_memory(base, size, omap_cpu_io_entry);
        i = (base - ta->bus->base) / TARGET_PAGE_SIZE;
        for (; size > 0; size -= TARGET_PAGE_SIZE, i ++) {
            omap_l4_io_readb_fn[i] = omap_l4_io_entry[iotype].mem_read[0];
            omap_l4_io_readh_fn[i] = omap_l4_io_entry[iotype].mem_read[1];
            omap_l4_io_readw_fn[i] = omap_l4_io_entry[iotype].mem_read[2];
            omap_l4_io_writeb_fn[i] = omap_l4_io_entry[iotype].mem_write[0];
            omap_l4_io_writeh_fn[i] = omap_l4_io_entry[iotype].mem_write[1];
            omap_l4_io_writew_fn[i] = omap_l4_io_entry[iotype].mem_write[2];
            omap_l4_io_opaque[i] = omap_l4_io_entry[iotype].opaque;
        }
#endif
    }

    return base;
}

target_phys_addr_t omap_l4_base(struct omap_target_agent_s *ta, int region)
{
    return ta->bus->base + ta->start[region].offset;
}

uint32_t omap_l4_size(struct omap_target_agent_s *ta, int region)
{
    return ta->start[region].size;
}

static uint32_t omap2_l4ta_read(void *opaque, target_phys_addr_t addr)
{
    struct omap_target_agent_s *s = (struct omap_target_agent_s *) opaque;

    switch (addr) {
    case 0x00:  /* COMPONENT */
        return s->component;

    case 0x20:  /* AGENT_CONTROL */
        return s->control;

    case 0x28:  /* AGENT_STATUS */
        return s->status;
    }

    OMAP_BAD_REG(addr);
    return 0;
}

static void omap2_l4ta_write(void *opaque, target_phys_addr_t addr,
                             uint32_t value)
{
    struct omap_target_agent_s *s = (struct omap_target_agent_s *) opaque;

    switch (addr) {
    case 0x00:  /* COMPONENT */
    case 0x28:  /* AGENT_STATUS */
        OMAP_RO_REG(addr);
        break;

    case 0x20:  /* AGENT_CONTROL */
        s->control = value & 0x01000700;
        if (value & 1)        /* OCP_RESET */
            s->status &= ~1;  /* REQ_TIMEOUT */
        break;

    default:
        OMAP_BAD_REG(addr);
    }
}

static CPUReadMemoryFunc * const omap2_l4ta_readfn[] = {
    omap_badwidth_read16,
    omap2_l4ta_read,
    omap_badwidth_read16,
};

static CPUWriteMemoryFunc * const omap2_l4ta_writefn[] = {
    omap_badwidth_write32,
    omap_badwidth_write32,
    omap2_l4ta_write,
};

struct omap_target_agent_s *omap2_l4ta_init(
    struct omap_l4_s *bus,
    const struct omap_l4_region_s *regions,
    const struct omap2_l4_agent_info_s *agents,
    int cs)
{
    int i, iomemtype;
    struct omap_target_agent_s *ta = NULL;
    const struct omap2_l4_agent_info_s *info = NULL;

    for (i = 0; i < bus->ta_num; i ++) {
        if (agents[i].ta == cs) {
            ta = &bus->ta[i];
            info = &agents[i];
            break;
        }
    }
    if (!ta) {
        fprintf(stderr, "%s: bad target agent (%i)\n", __FUNCTION__, cs);
        exit(-1);
    }

    ta->bus = bus;
    ta->start = &regions[info->region];
    ta->regions = info->regions;

    ta->component = ('Q' << 24) | ('E' << 16) | ('M' << 8) | ('U' << 0);
    ta->status = 0x00000000;
    ta->control = 0x00000200;	/* XXX 01000200 for L4TAO */

    iomemtype = l4_register_io_memory(omap2_l4ta_readfn,
                                      omap2_l4ta_writefn, ta);
    ta->base = omap_l4_attach(ta, info->ta_region, iomemtype);

    return ta;
}

#if defined(L4_MUX_HACK) || !defined(OMAP3_REDUCE_IOREGIONS)
static uint32_t omap3_l4ta_read(void *opaque, target_phys_addr_t addr)
{
    struct omap_target_agent_s *s = (struct omap_target_agent_s *)opaque;

    switch (addr) {
    case 0x00: /* COMPONENT_L */
        return s->component;
    case 0x04: /* COMPONENT_H */
        return 0;
    case 0x18: /* CORE_L */
        return s->component;
    case 0x1c: /* CORE_H */
        return (s->component >> 16);
    case 0x20: /* AGENT_CONTROL_L */
        return s->control;
    case 0x24: /* AGENT_CONTROL_H */
        return s->control_h;
    case 0x28: /* AGENT_STATUS_L */
        return s->status;
    case 0x2c: /* AGENT_STATUS_H */
        return 0;
    default:
        break;
    }

    OMAP_BAD_REG(s->base + addr);
    return 0;
}

static void omap3_l4ta_write(void *opaque, target_phys_addr_t addr,
                             uint32_t value)
{
    struct omap_target_agent_s *s = (struct omap_target_agent_s *)opaque;

    switch (addr) {
    case 0x00: /* COMPONENT_L */
    case 0x04: /* COMPONENT_H */
    case 0x18: /* CORE_L */
    case 0x1c: /* CORE_H */
        OMAP_RO_REG(s->base + addr);
        break;
    case 0x20: /* AGENT_CONTROL_L */
        s->control = value & 0x00000701;
        break;
    case 0x24: /* AGENT_CONTROL_H */
        s->control_h = value & 0x100; /* TODO: shouldn't this be read-only? */
        break;
    case 0x28: /* AGENT_STATUS_L */
        if (value & 0x100)
            s->status &= ~0x100; /* REQ_TIMEOUT */
        break;
    case 0x2c: /* AGENT_STATUS_H */
        /* no writable bits although the register is listed as RW */
        break;
    default:
        OMAP_BAD_REG(s->base + addr);
        break;
    }
}

static void omap3_l4ta_save_state(QEMUFile *f, void *opaque)
{
    struct omap_target_agent_s *s = (struct omap_target_agent_s *)opaque;

    qemu_put_be32(f, s->control);
    qemu_put_be32(f, s->control_h);
    qemu_put_be32(f, s->status);
}

static int omap3_l4ta_load_state(QEMUFile *f, void *opaque, int version_id)
{
    struct omap_target_agent_s *s = (struct omap_target_agent_s *)opaque;

    if (version_id)
        return -EINVAL;

    s->control = qemu_get_be32(f);
    s->control_h = qemu_get_be32(f);
    s->status = qemu_get_be32(f);

    return 0;
}

static CPUReadMemoryFunc *omap3_l4ta_readfn[] = {
    omap_badwidth_read32,
    omap_badwidth_read32,
    omap3_l4ta_read,
};

static CPUWriteMemoryFunc *omap3_l4ta_writefn[] = {
    omap_badwidth_write32,
    omap_badwidth_write32,
    omap3_l4ta_write,
};
#endif

struct omap_target_agent_s *omap3_l4ta_init(
    struct omap_l4_s *bus,
    const struct omap_l4_region_s *regions,
    const struct omap3_l4_agent_info_s *agents,
    int cs)
{
#if defined(L4_MUX_HACK) || !defined(OMAP3_REDUCE_IOREGIONS)
    int iomemtype;
#endif
    int i;
    struct omap_target_agent_s *ta = NULL;
    const struct omap3_l4_agent_info_s *info = NULL;

    for (i = 0; i < bus->ta_num; i++)
        if (agents[i].agent_id == cs) {
            ta = &bus->ta[i];
            info = &agents[i];
            break;
        }
    if (!ta) {
        hw_error("%s: invalid agent id (%i)", __FUNCTION__, cs);
    }
    if (ta->bus) {
        hw_error("%s: target agent (%d) already initialized", __FUNCTION__,
                 cs);
    }

    ta->bus = bus;
    ta->start = &regions[info->first_region_id];
    ta->regions = info->region_count;

    ta->component = ('Q' << 24) | ('E' << 16) | ('M' << 8) | ('U' << 0);
    ta->status = 0x00000000;
    ta->control = 0x00000200;

    for (i = 0; i < info->region_count; i++)
        if (regions[info->first_region_id + i].access == L4TYPE_TA)
            break;
    if (i >= info->region_count) {
        hw_error("%s: specified agent (%d) has no TA region", __FUNCTION__,
                 cs);
    }

#if defined(L4_MUX_HACK) || !defined(OMAP3_REDUCE_IOREGIONS)
    iomemtype = l4_register_io_memory(0, omap3_l4ta_readfn,
                                      omap3_l4ta_writefn, ta);
    ta->base = omap_l4_attach(ta, i, iomemtype);
    
    register_savevm("omap3_l4ta", ta->base >> 8, 0,
                    omap3_l4ta_save_state, omap3_l4ta_load_state, ta);
#else
    ta->base = ta->bus->base + ta->start[i].offset;
#endif

    return ta;
}
