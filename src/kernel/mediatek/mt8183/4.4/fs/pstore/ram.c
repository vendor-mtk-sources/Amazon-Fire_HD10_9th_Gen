/*
 * RAM Oops/Panic logger
 *
 * Copyright (C) 2010 Marco Stornelli <marco.stornelli@gmail.com>
 * Copyright (C) 2011 Kees Cook <keescook@chromium.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/pstore.h>
#include <linux/time.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/compiler.h>
#include <linux/pstore_ram.h>
#include <linux/of.h>
#include <linux/of_address.h>

#define RAMOOPS_KERNMSG_HDR "===="
#define MIN_MEM_SIZE 4096UL
#ifdef __aarch64__
#ifdef memcpy
#undef memcpy
#endif
#define memcpy memcpy_toio
#endif

static ulong record_size = MIN_MEM_SIZE;
module_param(record_size, ulong, 0400);
MODULE_PARM_DESC(record_size,
		"size of each dump done on oops/panic");

#ifdef CONFIG_PSTORE_CONSOLE_SIZE
static ulong ramoops_console_size = CONFIG_PSTORE_CONSOLE_SIZE;
#else
static ulong ramoops_console_size = MIN_MEM_SIZE;
#endif
module_param_named(console_size, ramoops_console_size, ulong, 0400);
MODULE_PARM_DESC(console_size, "size of kernel console log");

static ulong ramoops_ftrace_size = MIN_MEM_SIZE;
module_param_named(ftrace_size, ramoops_ftrace_size, ulong, 0400);
MODULE_PARM_DESC(ftrace_size, "size of ftrace log");

#ifdef CONFIG_PSTORE_PMSG_SIZE
static ulong ramoops_pmsg_size = CONFIG_PSTORE_PMSG_SIZE;
#else
static ulong ramoops_pmsg_size = MIN_MEM_SIZE;
#endif
module_param_named(pmsg_size, ramoops_pmsg_size, ulong, 0400);
MODULE_PARM_DESC(pmsg_size, "size of user space message log");

#ifdef CONFIG_PSTORE_MEM_ADDR
static ulong mem_address = CONFIG_PSTORE_MEM_ADDR;
#else
static ulong mem_address;
#endif
module_param(mem_address, ulong, 0400);
MODULE_PARM_DESC(mem_address,
		"start of reserved RAM used to store oops/panic logs");

#ifdef CONFIG_PSTORE_MEM_SIZE
static ulong mem_size = CONFIG_PSTORE_MEM_SIZE;
#else
static ulong mem_size;
#endif
module_param(mem_size, ulong, 0400);
MODULE_PARM_DESC(mem_size,
		"size of reserved RAM used to store oops/panic logs");

static unsigned int mem_type;
module_param(mem_type, uint, 0600);
MODULE_PARM_DESC(mem_type,
		"set to 1 to try to use unbuffered memory (default 0)");

static int dump_oops = 1;
module_param(dump_oops, int, 0600);
MODULE_PARM_DESC(dump_oops,
		"set to 1 to dump oopses, 0 to only dump panics (default 1)");

static int ramoops_ecc;
module_param_named(ecc, ramoops_ecc, int, 0600);
MODULE_PARM_DESC(ramoops_ecc,
		"if non-zero, the option enables ECC support and specifies "
		"ECC buffer size in bytes (1 is a special value, means 16 "
		"bytes ECC)");

struct ramoops_context {
	struct persistent_ram_zone **przs;
	struct persistent_ram_zone *cprz;
	struct persistent_ram_zone *fprz;
	struct persistent_ram_zone *mprz;
	struct persistent_ram_zone *bprz; /* simple lockless buffer console */
	phys_addr_t phys_addr;
	unsigned long size;
	unsigned int memtype;
	size_t record_size;
	size_t console_size;
	size_t ftrace_size;
	size_t pmsg_size;
	int dump_oops;
	struct persistent_ram_ecc_info ecc_info;
	unsigned int max_dump_cnt;
	unsigned int dump_write_cnt;
	/* _read_cnt need clear on ramoops_pstore_open */
	unsigned int dump_read_cnt;
	unsigned int console_read_cnt;
	unsigned int ftrace_read_cnt;
	unsigned int pmsg_read_cnt;
	unsigned int bconsole_read_cnt;
	struct pstore_info pstore;
};

static struct platform_device *dummy;
static struct ramoops_platform_data *dummy_data;

static int ramoops_pstore_open(struct pstore_info *psi)
{
	struct ramoops_context *cxt = psi->data;

	cxt->dump_read_cnt = 0;
	cxt->console_read_cnt = 0;
	cxt->ftrace_read_cnt = 0;
	cxt->pmsg_read_cnt = 0;
	cxt->bconsole_read_cnt = 0;
	return 0;
}

static struct persistent_ram_zone *
ramoops_get_next_prz(struct persistent_ram_zone *przs[], uint *c, uint max,
		     u64 *id,
		     enum pstore_type_id *typep, enum pstore_type_id type,
		     bool update)
{
	struct persistent_ram_zone *prz;
	int i = (*c)++;

	if (i >= max)
		return NULL;

	prz = przs[i];
	if (!prz)
		return NULL;

	/* Update old/shadowed buffer. */
	if (update)
		persistent_ram_save_old(prz);

	if (!persistent_ram_old_size(prz))
		return NULL;

	*typep = type;
	*id = i;

	return prz;
}

static int ramoops_read_kmsg_hdr(char *buffer, struct timespec *time,
				  bool *compressed)
{
	char data_type;
	int header_length = 0;

	if (sscanf(buffer, RAMOOPS_KERNMSG_HDR "%lu.%lu-%c\n%n", &time->tv_sec,
			&time->tv_nsec, &data_type, &header_length) == 3) {
		if (data_type == 'C')
			*compressed = true;
		else
			*compressed = false;
	} else if (sscanf(buffer, RAMOOPS_KERNMSG_HDR "%lu.%lu\n%n",
			&time->tv_sec, &time->tv_nsec, &header_length) == 2) {
			*compressed = false;
	} else {
		time->tv_sec = 0;
		time->tv_nsec = 0;
		*compressed = false;
	}
	return header_length;
}

static bool prz_ok(struct persistent_ram_zone *prz)
{
	return !!prz && !!(persistent_ram_old_size(prz) +
			   persistent_ram_ecc_string(prz, NULL, 0));
}

#define PLAT_LOG_BUFFER_SIZE 4096
static char plat_log_buf[PLAT_LOG_BUFFER_SIZE + 1];
static int plat_log_size;
void ramoops_append_plat_log(const char *fmt, ...)
{
	va_list ap;

	if (PLAT_LOG_BUFFER_SIZE - plat_log_size > 0) {
		va_start(ap, fmt);
		plat_log_size += vscnprintf(plat_log_buf + plat_log_size, PLAT_LOG_BUFFER_SIZE - plat_log_size, fmt, ap);
		va_end(ap);
	}
}

#define FALLBACK_ORDER (PAGE_ALLOC_COSTLY_ORDER + 1) /* tuning number */

/**
 * kvmalloc - attempt to allocate physically contiguous memory, but upon
 * failure, fall back to non-contiguous (vmalloc) allocation.
 * @size: size of the request.
 *
 * Uses kmalloc to get the memory but if the allocation fails then falls back
 * to the vmalloc allocator. Use kvfree for freeing the memory.
 *
 * Return: pointer to the allocated memory of %NULL in case of failure
 */
static inline void *kvmalloc(size_t size)
{
	gfp_t kmalloc_flags = GFP_KERNEL;
	void *ret;
	/*
	 * We want to attempt a large physically contiguous block first because
	 * it is less likely to fragment multiple larger blocks and therefore
	 * contribute to a long term fragmentation less than vmalloc fallback.
	 * However make sure that larger requests are not too disruptive - no
	 * OOM killer and no allocation failure warnings as we have a fallback.
	 */
	if (get_order(size) >= FALLBACK_ORDER) {
		kmalloc_flags |= __GFP_NOWARN;
	}

	ret = kmalloc(size, kmalloc_flags);

	/*
	 * won't fallback to vmalloc if order < FALLBACK_ORDER
	 */
	if (ret || get_order(size) < FALLBACK_ORDER)
		return ret;

	pr_err("Can't allocate memory by kmalloc, fallback to vmalloc.\n");

	return vmalloc(size);

}

static ssize_t ramoops_pstore_read(u64 *id, enum pstore_type_id *type,
				   int *count, struct timespec *time,
				   char **buf, bool *compressed,
				   struct pstore_info *psi)
{
	ssize_t size, extra_size = 0;
	ssize_t ecc_notice_size;
	struct ramoops_context *cxt = psi->data;
	struct persistent_ram_zone *prz = NULL;
	int header_length = 0;

	/* Ramoops headers provide time stamps for PSTORE_TYPE_DMESG, but
	 * PSTORE_TYPE_CONSOLE and PSTORE_TYPE_FTRACE don't currently have
	 * valid time stamps, so it is initialized to zero.
	 */
	time->tv_sec = 0;
	time->tv_nsec = 0;
	*compressed = false;

	/* Find the next valid persistent_ram_zone for DMESG */
	while (cxt->dump_read_cnt < cxt->max_dump_cnt && !prz) {
		prz = ramoops_get_next_prz(cxt->przs, &cxt->dump_read_cnt,
					   cxt->max_dump_cnt, id, type,
					   PSTORE_TYPE_DMESG, 1);
		if (!prz_ok(prz))
			continue;
		header_length = ramoops_read_kmsg_hdr(persistent_ram_old(prz),
						      time, compressed);
		/* Clear and skip this DMESG record if it has no valid header */
		if (!header_length) {
			persistent_ram_free_old(prz);
			persistent_ram_zap(prz);
			prz = NULL;
		}
	}

	if (!prz_ok(prz))
		prz = ramoops_get_next_prz(&cxt->cprz, &cxt->console_read_cnt,
					   1, id, type, PSTORE_TYPE_CONSOLE, 0);
	if (!prz_ok(prz)) {
		prz = ramoops_get_next_prz(&cxt->bprz, &cxt->bconsole_read_cnt,
					   1, id, type, PSTORE_TYPE_CONSOLE, 0);
		/* pr_notice("pstore: pstore_read bprz type: %d count %d id %llx\n",
		* *type, cxt->bconsole_read_cnt, *id);
		*/
		*id = 2;
	}
	if (!prz_ok(prz))
		prz = ramoops_get_next_prz(&cxt->fprz, &cxt->ftrace_read_cnt,
					   1, id, type, PSTORE_TYPE_FTRACE, 0);
	if (!prz_ok(prz))
		prz = ramoops_get_next_prz(&cxt->mprz, &cxt->pmsg_read_cnt,
					   1, id, type, PSTORE_TYPE_PMSG, 0);
	if (!prz_ok(prz))
		return 0;

	size = persistent_ram_old_size(prz) - header_length;

	/* ECC correction notice */
	ecc_notice_size = persistent_ram_ecc_string(prz, NULL, 0);

	if (*type == PSTORE_TYPE_CONSOLE) {
		extra_size += plat_log_size;
	}

	*buf = kvmalloc(size + ecc_notice_size + extra_size + 1);

	if (*buf == NULL) {
		pr_err("ramoops_pstore_read memory allocation failure, order = %d\n",
			get_order(size + ecc_notice_size + extra_size + 1));
		return -ENOMEM;
	}

	memcpy(*buf, (char *)persistent_ram_old(prz) + header_length, size);
	persistent_ram_ecc_string(prz, *buf + size, ecc_notice_size + 1);

	if (*type == PSTORE_TYPE_CONSOLE && extra_size) {
		memcpy(*buf + size + ecc_notice_size, plat_log_buf, extra_size);
		*(char *)(*buf + size + ecc_notice_size + extra_size) = '\0';
	}

	return size + ecc_notice_size + extra_size;
}

static size_t ramoops_write_kmsg_hdr(struct persistent_ram_zone *prz,
				     bool compressed)
{
	char *hdr;
	struct timespec timestamp;
	size_t len;

	/* Report zeroed timestamp if called before timekeeping has resumed. */
	if (__getnstimeofday(&timestamp)) {
		timestamp.tv_sec = 0;
		timestamp.tv_nsec = 0;
	}
	hdr = kasprintf(GFP_ATOMIC, RAMOOPS_KERNMSG_HDR "%lu.%lu-%c\n",
		(long)timestamp.tv_sec, (long)(timestamp.tv_nsec / 1000),
		compressed ? 'C' : 'D');
	WARN_ON_ONCE(!hdr);
	len = hdr ? strlen(hdr) : 0;
	persistent_ram_write(prz, hdr, len);
	kfree(hdr);

	return len;
}

static int notrace ramoops_pstore_write_buf(enum pstore_type_id type,
					    enum kmsg_dump_reason reason,
					    u64 *id, unsigned int part,
					    const char *buf,
					    bool compressed, size_t size,
					    struct pstore_info *psi)
{
	struct ramoops_context *cxt = psi->data;
	struct persistent_ram_zone *prz;
	size_t hlen;

	if (type == PSTORE_TYPE_CONSOLE) {
		if (reason == 0) {
			if (!cxt->cprz)
				return -ENOMEM;
			persistent_ram_write(cxt->cprz, buf, size);
		} else {
			if (!cxt->bprz)
				return -ENOMEM;
			persistent_ram_write(cxt->bprz, buf, size);
		}
		return 0;
	} else if (type == PSTORE_TYPE_FTRACE) {
		if (!cxt->fprz)
			return -ENOMEM;
		persistent_ram_write(cxt->fprz, buf, size);
		return 0;
	} else if (type == PSTORE_TYPE_PMSG) {
		if (!cxt->mprz)
			return -ENOMEM;
		persistent_ram_write(cxt->mprz, buf, size);
		return 0;
	}

	if (type != PSTORE_TYPE_DMESG)
		return -EINVAL;

	/* Out of the various dmesg dump types, ramoops is currently designed
	 * to only store crash logs, rather than storing general kernel logs.
	 */
	if (reason != KMSG_DUMP_OOPS &&
	    reason != KMSG_DUMP_PANIC)
		return -EINVAL;

	/* Skip Oopes when configured to do so. */
	if (reason == KMSG_DUMP_OOPS && !cxt->dump_oops)
		return -EINVAL;

	/* Explicitly only take the first part of any new crash.
	 * If our buffer is larger than kmsg_bytes, this can never happen,
	 * and if our buffer is smaller than kmsg_bytes, we don't want the
	 * report split across multiple records.
	 */
	if (part != 1)
		return -ENOSPC;

	if (!cxt->przs)
		return -ENOSPC;

	prz = cxt->przs[cxt->dump_write_cnt];

	hlen = ramoops_write_kmsg_hdr(prz, compressed);
	if (size + hlen > prz->buffer_size)
		size = prz->buffer_size - hlen;
	persistent_ram_write(prz, buf, size);

	cxt->dump_write_cnt = (cxt->dump_write_cnt + 1) % cxt->max_dump_cnt;

	return 0;
}

static int notrace ramoops_pstore_write_buf_user(enum pstore_type_id type,
						 enum kmsg_dump_reason reason,
						 u64 *id, unsigned int part,
						 const char __user *buf,
						 bool compressed, size_t size,
						 struct pstore_info *psi)
{
	if (type == PSTORE_TYPE_PMSG) {
		struct ramoops_context *cxt = psi->data;

		if (!cxt->mprz)
			return -ENOMEM;
		return persistent_ram_write_user(cxt->mprz, buf, size);
	}

	return -EINVAL;
}

static int ramoops_pstore_erase(enum pstore_type_id type, u64 id, int count,
				struct timespec time, struct pstore_info *psi)
{
	struct ramoops_context *cxt = psi->data;
	struct persistent_ram_zone *prz;

	switch (type) {
	case PSTORE_TYPE_DMESG:
		if (id >= cxt->max_dump_cnt)
			return -EINVAL;
		prz = cxt->przs[id];
		break;
	case PSTORE_TYPE_CONSOLE:
		prz = cxt->cprz;
		break;
	case PSTORE_TYPE_FTRACE:
		prz = cxt->fprz;
		break;
	case PSTORE_TYPE_PMSG:
		prz = cxt->mprz;
		break;
	default:
		return -EINVAL;
	}

	persistent_ram_free_old(prz);
	persistent_ram_zap(prz);

	return 0;
}

static struct ramoops_context oops_cxt = {
	.pstore = {
		.owner	= THIS_MODULE,
		.name	= "ramoops",
		.open	= ramoops_pstore_open,
		.read	= ramoops_pstore_read,
		.write_buf	= ramoops_pstore_write_buf,
		.write_buf_user	= ramoops_pstore_write_buf_user,
		.erase	= ramoops_pstore_erase,
	},
};

static void ramoops_free_przs(struct ramoops_context *cxt)
{
	int i;

	if (!cxt->przs)
		return;

	for (i = 0; i < cxt->max_dump_cnt; i++)
		persistent_ram_free(cxt->przs[i]);

	kfree(cxt->przs);
	cxt->max_dump_cnt = 0;
}

static int ramoops_init_przs(struct device *dev, struct ramoops_context *cxt,
			     phys_addr_t *paddr, size_t dump_mem_sz)
{
	int err = -ENOMEM;
	int i;

	if (!cxt->record_size)
		return 0;

	if (*paddr + dump_mem_sz - cxt->phys_addr > cxt->size) {
		dev_err(dev, "no room for dumps\n");
		return -ENOMEM;
	}

	cxt->max_dump_cnt = dump_mem_sz / cxt->record_size;
	if (!cxt->max_dump_cnt)
		return -ENOMEM;

	cxt->przs = kzalloc(sizeof(*cxt->przs) * cxt->max_dump_cnt,
			     GFP_KERNEL);
	if (!cxt->przs) {
		dev_err(dev, "failed to initialize a prz array for dumps\n");
		goto fail_mem;
	}

	for (i = 0; i < cxt->max_dump_cnt; i++) {
		cxt->przs[i] = persistent_ram_new(*paddr, cxt->record_size, 0,
						  &cxt->ecc_info,
						  cxt->memtype, 0);
		if (IS_ERR(cxt->przs[i])) {
			err = PTR_ERR(cxt->przs[i]);
			dev_err(dev, "failed to request mem region (0x%zx@0x%llx): %d\n",
				cxt->record_size, (unsigned long long)*paddr, err);

			while (i > 0) {
				i--;
				persistent_ram_free(cxt->przs[i]);
			}
			goto fail_prz;
		}
		*paddr += cxt->record_size;
	}

	return 0;
fail_prz:
	kfree(cxt->przs);
fail_mem:
	cxt->max_dump_cnt = 0;
	return err;
}

static int ramoops_init_prz(struct device *dev, struct ramoops_context *cxt,
			    struct persistent_ram_zone **prz,
			    phys_addr_t *paddr, size_t sz, u32 sig)
{
	if (!sz)
		return 0;

	if (*paddr + sz - cxt->phys_addr > cxt->size) {
		dev_err(dev, "no room for mem region (0x%zx@0x%llx) in (0x%lx@0x%llx)\n",
			sz, (unsigned long long)*paddr,
			cxt->size, (unsigned long long)cxt->phys_addr);
		return -ENOMEM;
	}

	*prz = persistent_ram_new(*paddr, sz, sig, &cxt->ecc_info,
				  cxt->memtype, 0);
	if (IS_ERR(*prz)) {
		int err = PTR_ERR(*prz);

		dev_err(dev, "failed to request mem region (0x%zx@0x%llx): %d\n",
			sz, (unsigned long long)*paddr, err);
		return err;
	}

	persistent_ram_zap(*prz);

	*paddr += sz;

	return 0;
}

void notrace ramoops_console_write_buf(const char *buf, size_t size)
{
	struct ramoops_context *cxt = &oops_cxt;
	persistent_ram_write(cxt->cprz, buf, size);
}

static int ramoops_parse_dt_size(struct platform_device *pdev,
		const char *propname, unsigned long *val)
{
	u64 val64;
	int ret;

	ret = of_property_read_u64(pdev->dev.of_node, propname, &val64);
	if (ret == -EINVAL) {
		*val = 0;
		return 0;
	} else if (ret != 0) {
		dev_err(&pdev->dev, "failed to parse property %s: %d\n",
				propname, ret);
		return ret;
	}

	if (val64 > ULONG_MAX) {
		dev_err(&pdev->dev, "invalid %s %llu\n", propname, val64);
		return -EOVERFLOW;
	}

	*val = val64;
	return 0;
}

static int ramoops_parse_dt(struct platform_device *pdev,
		struct ramoops_platform_data *pdata)
{
	struct device_node *of_node = pdev->dev.of_node;
	struct device_node *mem_region;
	struct resource res;
	u32 ecc_size;
	int ret;

	dev_dbg(&pdev->dev, "using Device Tree\n");

	mem_region = of_parse_phandle(of_node, "memory-region", 0);
	if (!mem_region) {
		dev_err(&pdev->dev, "no memory-region phandle\n");
		return -ENODEV;
	}

	ret = of_address_to_resource(mem_region, 0, &res);
	of_node_put(mem_region);
	if (ret) {
		dev_err(&pdev->dev, "failed to translate memory-region to resource: %d\n",
				ret);
		return ret;
	}

	pdata->mem_size = resource_size(&res);
	pdata->mem_address = res.start;
	pdata->mem_type = of_property_read_bool(of_node, "unbuffered");
	pdata->dump_oops = !of_property_read_bool(of_node, "no-dump-oops");

	ret = ramoops_parse_dt_size(pdev, "record-size", &pdata->record_size);
	if (ret < 0)
		return ret;

	ret = ramoops_parse_dt_size(pdev, "console-size", &pdata->console_size);
	if (ret < 0)
		return ret;

	ret = ramoops_parse_dt_size(pdev, "ftrace-size", &pdata->ftrace_size);
	if (ret < 0)
		return ret;

	ret = ramoops_parse_dt_size(pdev, "pmsg-size", &pdata->pmsg_size);
	if (ret < 0)
		return ret;

	ret = of_property_read_u32(of_node, "ecc-size", &ecc_size);
	if (ret == 0) {
		if (ecc_size > INT_MAX) {
			dev_err(&pdev->dev, "invalid ecc-size %u\n", ecc_size);
			return -EOVERFLOW;
		}
		pdata->ecc_info.ecc_size = ecc_size;
	} else if (ret != -EINVAL) {
		return ret;
	}

	return 0;
}

static int ramoops_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ramoops_platform_data *pdata = pdev->dev.platform_data;
	struct ramoops_context *cxt = &oops_cxt;
	size_t dump_mem_sz;
	phys_addr_t paddr;
	int err = -EINVAL;

	if (!pdata)
		pdata = pdev->dev.platform_data;

	if (dev->of_node && !pdata) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			err = -ENOMEM;
			goto fail_out;
		}

		err = ramoops_parse_dt(pdev, pdata);
		if (err < 0)
			goto fail_out;
	}

	/* Only a single ramoops area allowed at a time, so fail extra
	 * probes.
	 */
	if (cxt->max_dump_cnt)
		goto fail_out;

	if (!pdata->mem_size || (!pdata->record_size && !pdata->console_size &&
			!pdata->ftrace_size && !pdata->pmsg_size)) {
		pr_err("The memory size and the record/console size must be "
			"non-zero\n");
		goto fail_out;
	}

	if (pdata->record_size && !is_power_of_2(pdata->record_size))
		pdata->record_size = rounddown_pow_of_two(pdata->record_size);
	if (pdata->console_size && !is_power_of_2(pdata->console_size))
		pdata->console_size = rounddown_pow_of_two(pdata->console_size);
	if (pdata->ftrace_size && !is_power_of_2(pdata->ftrace_size))
		pdata->ftrace_size = rounddown_pow_of_two(pdata->ftrace_size);
	if (pdata->pmsg_size && !is_power_of_2(pdata->pmsg_size))
		pdata->pmsg_size = rounddown_pow_of_two(pdata->pmsg_size);

	cxt->size = pdata->mem_size;
	cxt->phys_addr = pdata->mem_address;
	cxt->memtype = pdata->mem_type;
	cxt->record_size = pdata->record_size;
	cxt->console_size = pdata->console_size;
	cxt->ftrace_size = pdata->ftrace_size;
	cxt->pmsg_size = pdata->pmsg_size;
	cxt->dump_oops = pdata->dump_oops;
	cxt->ecc_info = pdata->ecc_info;

	pr_err("pstore:address is 0x%lx, size is 0x%lx, console_size is 0x%zx, pmsg_size is 0x%zx\n",
			(unsigned long)cxt->phys_addr, cxt->size, cxt->console_size, cxt->pmsg_size);
	paddr = cxt->phys_addr;

	dump_mem_sz = cxt->size - cxt->console_size * 2 - cxt->ftrace_size
			- cxt->pmsg_size;
	err = ramoops_init_przs(dev, cxt, &paddr, dump_mem_sz);
	if (err)
		goto fail_out;

	err = ramoops_init_prz(dev, cxt, &cxt->cprz, &paddr,
			       cxt->console_size, 0);
	if (err)
		goto fail_init_cprz;

	err = ramoops_init_prz(dev, cxt, &cxt->bprz, &paddr,
			       cxt->console_size, 0);
	if (err)
		goto fail_init_bprz;

	err = ramoops_init_prz(dev, cxt, &cxt->fprz, &paddr, cxt->ftrace_size,
			       LINUX_VERSION_CODE);
	if (err)
		goto fail_init_fprz;

	err = ramoops_init_prz(dev, cxt, &cxt->mprz, &paddr, cxt->pmsg_size, 0);
	if (err)
		goto fail_init_mprz;

	cxt->pstore.data = cxt;
	/*
	 * Console can handle any buffer size, so prefer LOG_LINE_MAX. If we
	 * have to handle dumps, we must have at least record_size buffer. And
	 * for ftrace, bufsize is irrelevant (if bufsize is 0, buf will be
	 * ZERO_SIZE_PTR).
	 */
	if (cxt->console_size)
		cxt->pstore.bufsize = 1024; /* LOG_LINE_MAX */
	cxt->pstore.bufsize = max(cxt->record_size, cxt->pstore.bufsize);
	cxt->pstore.buf = kmalloc(cxt->pstore.bufsize, GFP_KERNEL);
	spin_lock_init(&cxt->pstore.buf_lock);
	if (!cxt->pstore.buf) {
		pr_err("cannot allocate pstore buffer\n");
		err = -ENOMEM;
		goto fail_clear;
	}

	err = pstore_register(&cxt->pstore);
	if (err) {
		pr_err("registering with pstore failed\n");
		goto fail_buf;
	}

	/*
	 * Update the module parameter variables as well so they are visible
	 * through /sys/module/ramoops/parameters/
	 */
	mem_size = pdata->mem_size;
	mem_address = pdata->mem_address;
	record_size = pdata->record_size;
	dump_oops = pdata->dump_oops;
	ramoops_console_size = pdata->console_size;
	ramoops_pmsg_size = pdata->pmsg_size;
	ramoops_ftrace_size = pdata->ftrace_size;

	pr_info("attached 0x%lx@0x%llx, ecc: %d/%d\n",
		cxt->size, (unsigned long long)cxt->phys_addr,
		cxt->ecc_info.ecc_size, cxt->ecc_info.block_size);

	return 0;

fail_buf:
	kfree(cxt->pstore.buf);
fail_clear:
	cxt->pstore.bufsize = 0;
	kfree(cxt->mprz);
fail_init_mprz:
	kfree(cxt->fprz);
fail_init_fprz:
	kfree(cxt->bprz);
fail_init_bprz:
	kfree(cxt->cprz);
fail_init_cprz:
	ramoops_free_przs(cxt);
fail_out:
	return err;
}

static int ramoops_remove(struct platform_device *pdev)
{
	struct ramoops_context *cxt = &oops_cxt;

	pstore_unregister(&cxt->pstore);

	kfree(cxt->pstore.buf);
	cxt->pstore.bufsize = 0;

	persistent_ram_free(cxt->mprz);
	persistent_ram_free(cxt->fprz);
	persistent_ram_free(cxt->cprz);
	ramoops_free_przs(cxt);

	return 0;
}

static const struct of_device_id dt_match[] = {
	{ .compatible = "ramoops" },
	{}
};

static struct platform_driver ramoops_driver = {
	.probe		= ramoops_probe,
	.remove		= ramoops_remove,
	.driver		= {
		.name		= "ramoops",
		.of_match_table	= dt_match,
	},
};

static void ramoops_register_dummy(void)
{
	if (!mem_size)
		return;

	pr_info("using module parameters\n");

	dummy_data = kzalloc(sizeof(*dummy_data), GFP_KERNEL);
	if (!dummy_data) {
		pr_info("could not allocate pdata\n");
		return;
	}

	dummy_data->mem_size = mem_size;
	dummy_data->mem_address = mem_address;
	dummy_data->mem_type = mem_type;
	dummy_data->record_size = record_size;
	dummy_data->console_size = ramoops_console_size;
	dummy_data->ftrace_size = ramoops_ftrace_size;
	dummy_data->pmsg_size = ramoops_pmsg_size;
	dummy_data->dump_oops = dump_oops;
	/*
	 * For backwards compatibility ramoops.ecc=1 means 16 bytes ECC
	 * (using 1 byte for ECC isn't much of use anyway).
	 */
	dummy_data->ecc_info.ecc_size = ramoops_ecc == 1 ? 16 : ramoops_ecc;

	dummy = platform_device_register_data(NULL, "ramoops", -1,
			dummy_data, sizeof(struct ramoops_platform_data));
	if (IS_ERR(dummy)) {
		pr_info("could not create platform device: %ld\n",
			PTR_ERR(dummy));
	}
}

static int __init ramoops_init(void)
{
	ramoops_register_dummy();
	return platform_driver_register(&ramoops_driver);
}
postcore_initcall(ramoops_init);

static void __exit ramoops_exit(void)
{
	platform_driver_unregister(&ramoops_driver);
	platform_device_unregister(dummy);
	kfree(dummy_data);
}
module_exit(ramoops_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marco Stornelli <marco.stornelli@gmail.com>");
MODULE_DESCRIPTION("RAM Oops/Panic logger/driver");
