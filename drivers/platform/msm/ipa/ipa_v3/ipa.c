/* Copyright (c) 2012-2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/compat.h>
#include <linux/device.h>
#include <linux/dmapool.h>
#include <linux/fs.h>
#include <linux/genalloc.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/rbtree.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/msm-bus.h>
#include <linux/msm-bus-board.h>
#include <linux/netdevice.h>
#include <linux/delay.h>
#include <linux/msm_gsi.h>
#include <linux/qcom_iommu.h>
#include <linux/time.h>
#include <linux/hashtable.h>
#include <linux/hash.h>
#include "ipa_i.h"
#include "ipa_rm_i.h"

#define CREATE_TRACE_POINTS
#include "ipa_trace.h"

#define IPA_SUMMING_THRESHOLD (0x10)
#define IPA_PIPE_MEM_START_OFST (0x0)
#define IPA_PIPE_MEM_SIZE (0x0)
#define IPA_MOBILE_AP_MODE(x) (x == IPA_MODE_MOBILE_AP_ETH || \
			       x == IPA_MODE_MOBILE_AP_WAN || \
			       x == IPA_MODE_MOBILE_AP_WLAN)
#define IPA_CNOC_CLK_RATE (75 * 1000 * 1000UL)
#define IPA_A5_MUX_HEADER_LENGTH (8)

#define IPA_AGGR_MAX_STR_LENGTH (10)

#define CLEANUP_TAG_PROCESS_TIMEOUT 150

#define IPA_AGGR_STR_IN_BYTES(str) \
	(strnlen((str), IPA_AGGR_MAX_STR_LENGTH - 1) + 1)

#define IPA_TRANSPORT_PROD_TIMEOUT_MSEC 100

#define IPA3_ACTIVE_CLIENTS_TABLE_BUF_SIZE 2048

#define IPA3_ACTIVE_CLIENT_LOG_TYPE_EP 0
#define IPA3_ACTIVE_CLIENT_LOG_TYPE_SIMPLE 1
#define IPA3_ACTIVE_CLIENT_LOG_TYPE_RESOURCE 2
#define IPA3_ACTIVE_CLIENT_LOG_TYPE_SPECIAL 3

#define IPA_FWS_PATH "ipa/ipa_fws.elf"

#ifdef CONFIG_COMPAT
#define IPA_IOC_ADD_HDR32 _IOWR(IPA_IOC_MAGIC, \
					IPA_IOCTL_ADD_HDR, \
					compat_uptr_t)
#define IPA_IOC_DEL_HDR32 _IOWR(IPA_IOC_MAGIC, \
					IPA_IOCTL_DEL_HDR, \
					compat_uptr_t)
#define IPA_IOC_ADD_RT_RULE32 _IOWR(IPA_IOC_MAGIC, \
					IPA_IOCTL_ADD_RT_RULE, \
					compat_uptr_t)
#define IPA_IOC_DEL_RT_RULE32 _IOWR(IPA_IOC_MAGIC, \
					IPA_IOCTL_DEL_RT_RULE, \
					compat_uptr_t)
#define IPA_IOC_ADD_FLT_RULE32 _IOWR(IPA_IOC_MAGIC, \
					IPA_IOCTL_ADD_FLT_RULE, \
					compat_uptr_t)
#define IPA_IOC_DEL_FLT_RULE32 _IOWR(IPA_IOC_MAGIC, \
					IPA_IOCTL_DEL_FLT_RULE, \
					compat_uptr_t)
#define IPA_IOC_GET_RT_TBL32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_GET_RT_TBL, \
				compat_uptr_t)
#define IPA_IOC_COPY_HDR32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_COPY_HDR, \
				compat_uptr_t)
#define IPA_IOC_QUERY_INTF32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_QUERY_INTF, \
				compat_uptr_t)
#define IPA_IOC_QUERY_INTF_TX_PROPS32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_QUERY_INTF_TX_PROPS, \
				compat_uptr_t)
#define IPA_IOC_QUERY_INTF_RX_PROPS32 _IOWR(IPA_IOC_MAGIC, \
					IPA_IOCTL_QUERY_INTF_RX_PROPS, \
					compat_uptr_t)
#define IPA_IOC_QUERY_INTF_EXT_PROPS32 _IOWR(IPA_IOC_MAGIC, \
					IPA_IOCTL_QUERY_INTF_EXT_PROPS, \
					compat_uptr_t)
#define IPA_IOC_GET_HDR32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_GET_HDR, \
				compat_uptr_t)
#define IPA_IOC_ALLOC_NAT_MEM32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_ALLOC_NAT_MEM, \
				compat_uptr_t)
#define IPA_IOC_V4_INIT_NAT32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_V4_INIT_NAT, \
				compat_uptr_t)
#define IPA_IOC_NAT_DMA32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_NAT_DMA, \
				compat_uptr_t)
#define IPA_IOC_V4_DEL_NAT32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_V4_DEL_NAT, \
				compat_uptr_t)
#define IPA_IOC_GET_NAT_OFFSET32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_GET_NAT_OFFSET, \
				compat_uptr_t)
#define IPA_IOC_PULL_MSG32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_PULL_MSG, \
				compat_uptr_t)
#define IPA_IOC_RM_ADD_DEPENDENCY32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_RM_ADD_DEPENDENCY, \
				compat_uptr_t)
#define IPA_IOC_RM_DEL_DEPENDENCY32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_RM_DEL_DEPENDENCY, \
				compat_uptr_t)
#define IPA_IOC_GENERATE_FLT_EQ32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_GENERATE_FLT_EQ, \
				compat_uptr_t)
#define IPA_IOC_QUERY_RT_TBL_INDEX32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_QUERY_RT_TBL_INDEX, \
				compat_uptr_t)
#define IPA_IOC_WRITE_QMAPID32  _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_WRITE_QMAPID, \
				compat_uptr_t)
#define IPA_IOC_MDFY_FLT_RULE32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_MDFY_FLT_RULE, \
				compat_uptr_t)
#define IPA_IOC_NOTIFY_WAN_UPSTREAM_ROUTE_ADD32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_NOTIFY_WAN_UPSTREAM_ROUTE_ADD, \
				compat_uptr_t)
#define IPA_IOC_NOTIFY_WAN_UPSTREAM_ROUTE_DEL32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_NOTIFY_WAN_UPSTREAM_ROUTE_DEL, \
				compat_uptr_t)
#define IPA_IOC_NOTIFY_WAN_EMBMS_CONNECTED32 _IOWR(IPA_IOC_MAGIC, \
					IPA_IOCTL_NOTIFY_WAN_EMBMS_CONNECTED, \
					compat_uptr_t)
#define IPA_IOC_ADD_HDR_PROC_CTX32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_ADD_HDR_PROC_CTX, \
				compat_uptr_t)
#define IPA_IOC_DEL_HDR_PROC_CTX32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_DEL_HDR_PROC_CTX, \
				compat_uptr_t)
#define IPA_IOC_MDFY_RT_RULE32 _IOWR(IPA_IOC_MAGIC, \
				IPA_IOCTL_MDFY_RT_RULE, \
				compat_uptr_t)

struct ipa3_ioc_nat_alloc_mem32 {
	char dev_name[IPA_RESOURCE_NAME_MAX];
	compat_size_t size;
	compat_off_t offset;
};
#endif

static void ipa3_start_tag_process(struct work_struct *work);
static DECLARE_WORK(ipa3_tag_work, ipa3_start_tag_process);

static void ipa3_sps_release_resource(struct work_struct *work);
static DECLARE_DELAYED_WORK(ipa3_sps_release_resource_work,
	ipa3_sps_release_resource);
static void ipa_gsi_notify_cb(struct gsi_per_notify *notify);

static void ipa_gsi_request_resource(struct work_struct *work);
static DECLARE_WORK(ipa_gsi_request_resource_work,
	ipa_gsi_request_resource);

static void ipa_gsi_release_resource(struct work_struct *work);
static DECLARE_DELAYED_WORK(ipa_gsi_release_resource_work,
	ipa_gsi_release_resource);

static struct ipa3_plat_drv_res ipa3_res = {0, };
struct msm_bus_scale_pdata *ipa3_bus_scale_table;

static struct clk *ipa3_clk;
static struct clk *smmu_clk;

struct ipa3_context *ipa3_ctx;
static struct device *master_dev;
struct platform_device *ipa3_pdev;
static bool smmu_present;
static bool arm_smmu;
static bool smmu_disable_htw;

static char *active_clients_table_buf;

const char *ipa3_clients_strings[IPA_CLIENT_MAX] = {
	__stringify(IPA_CLIENT_HSIC1_PROD),
	__stringify(IPA_CLIENT_WLAN1_PROD),
	__stringify(IPA_CLIENT_USB2_PROD),
	__stringify(IPA_CLIENT_HSIC3_PROD),
	__stringify(IPA_CLIENT_HSIC2_PROD),
	__stringify(IPA_CLIENT_USB3_PROD),
	__stringify(IPA_CLIENT_HSIC4_PROD),
	__stringify(IPA_CLIENT_USB4_PROD),
	__stringify(IPA_CLIENT_HSIC5_PROD),
	__stringify(IPA_CLIENT_USB_PROD),
	__stringify(IPA_CLIENT_A5_WLAN_AMPDU_PROD),
	__stringify(IPA_CLIENT_A2_EMBEDDED_PROD),
	__stringify(IPA_CLIENT_A2_TETHERED_PROD),
	__stringify(IPA_CLIENT_APPS_LAN_WAN_PROD),
	__stringify(IPA_CLIENT_APPS_CMD_PROD),
	__stringify(IPA_CLIENT_ODU_PROD),
	__stringify(IPA_CLIENT_MHI_PROD),
	__stringify(IPA_CLIENT_Q6_LAN_PROD),
	__stringify(IPA_CLIENT_Q6_WAN_PROD),
	__stringify(IPA_CLIENT_Q6_CMD_PROD),
	__stringify(IPA_CLIENT_MEMCPY_DMA_SYNC_PROD),
	__stringify(IPA_CLIENT_MEMCPY_DMA_ASYNC_PROD),
	__stringify(IPA_CLIENT_Q6_DECOMP_PROD),
	__stringify(IPA_CLIENT_Q6_DECOMP2_PROD),
	__stringify(IPA_CLIENT_UC_USB_PROD),

	
	__stringify(IPA_CLIENT_TEST_PROD),
	__stringify(IPA_CLIENT_TEST1_PROD),
	__stringify(IPA_CLIENT_TEST2_PROD),
	__stringify(IPA_CLIENT_TEST3_PROD),
	__stringify(IPA_CLIENT_TEST4_PROD),

	__stringify(IPA_CLIENT_HSIC1_CONS),
	__stringify(IPA_CLIENT_WLAN1_CONS),
	__stringify(IPA_CLIENT_HSIC2_CONS),
	__stringify(IPA_CLIENT_USB2_CONS),
	__stringify(IPA_CLIENT_WLAN2_CONS),
	__stringify(IPA_CLIENT_HSIC3_CONS),
	__stringify(IPA_CLIENT_USB3_CONS),
	__stringify(IPA_CLIENT_WLAN3_CONS),
	__stringify(IPA_CLIENT_HSIC4_CONS),
	__stringify(IPA_CLIENT_USB4_CONS),
	__stringify(IPA_CLIENT_WLAN4_CONS),
	__stringify(IPA_CLIENT_HSIC5_CONS),
	__stringify(IPA_CLIENT_USB_CONS),
	__stringify(IPA_CLIENT_USB_DPL_CONS),
	__stringify(IPA_CLIENT_A2_EMBEDDED_CONS),
	__stringify(IPA_CLIENT_A2_TETHERED_CONS),
	__stringify(IPA_CLIENT_A5_LAN_WAN_CONS),
	__stringify(IPA_CLIENT_APPS_LAN_CONS),
	__stringify(IPA_CLIENT_APPS_WAN_CONS),
	__stringify(IPA_CLIENT_ODU_EMB_CONS),
	__stringify(IPA_CLIENT_ODU_TETH_CONS),
	__stringify(IPA_CLIENT_MHI_CONS),
	__stringify(IPA_CLIENT_Q6_LAN_CONS),
	__stringify(IPA_CLIENT_Q6_WAN_CONS),
	__stringify(IPA_CLIENT_Q6_DUN_CONS),
	__stringify(IPA_CLIENT_MEMCPY_DMA_SYNC_CONS),
	__stringify(IPA_CLIENT_MEMCPY_DMA_ASYNC_CONS),
	__stringify(IPA_CLIENT_Q6_DECOMP_CONS),
	__stringify(IPA_CLIENT_Q6_DECOMP2_CONS),
	__stringify(IPA_CLIENT_Q6_LTE_WIFI_AGGR_CONS),
	
	__stringify(IPA_CLIENT_TEST_CONS),
	__stringify(IPA_CLIENT_TEST1_CONS),
	__stringify(IPA_CLIENT_TEST2_CONS),
	__stringify(IPA_CLIENT_TEST3_CONS),
	__stringify(IPA_CLIENT_TEST4_CONS),
};

int ipa3_active_clients_log_print_buffer(char *buf, int size)
{
	int i;
	int nbytes;
	int cnt = 0;
	int start_idx;
	int end_idx;

	start_idx = (ipa3_ctx->ipa3_active_clients_logging.log_tail + 1) %
			IPA3_ACTIVE_CLIENTS_LOG_BUFFER_SIZE_LINES;
	end_idx = ipa3_ctx->ipa3_active_clients_logging.log_head;
	for (i = start_idx; i != end_idx;
		i = (i + 1) % IPA3_ACTIVE_CLIENTS_LOG_BUFFER_SIZE_LINES) {
		nbytes = scnprintf(buf + cnt, size - cnt, "%s\n",
				ipa3_ctx->ipa3_active_clients_logging
				.log_buffer[i]);
		cnt += nbytes;
	}

	return cnt;
}

int ipa3_active_clients_log_print_table(char *buf, int size)
{
	int i;
	struct ipa3_active_client_htable_entry *iterator;
	int cnt = 0;

	cnt = scnprintf(buf, size, "\n---- Active Clients Table ----\n");
	hash_for_each(ipa3_ctx->ipa3_active_clients_logging.htable, i,
			iterator, list) {
		switch (iterator->type) {
		case IPA3_ACTIVE_CLIENT_LOG_TYPE_EP:
			cnt += scnprintf(buf + cnt, size - cnt,
					"%-40s %-3d ENDPOINT\n",
					iterator->id_string, iterator->count);
			break;
		case IPA3_ACTIVE_CLIENT_LOG_TYPE_SIMPLE:
			cnt += scnprintf(buf + cnt, size - cnt,
					"%-40s %-3d SIMPLE\n",
					iterator->id_string, iterator->count);
			break;
		case IPA3_ACTIVE_CLIENT_LOG_TYPE_RESOURCE:
			cnt += scnprintf(buf + cnt, size - cnt,
					"%-40s %-3d RESOURCE\n",
					iterator->id_string, iterator->count);
			break;
		case IPA3_ACTIVE_CLIENT_LOG_TYPE_SPECIAL:
			cnt += scnprintf(buf + cnt, size - cnt,
					"%-40s %-3d SPECIAL\n",
					iterator->id_string, iterator->count);
			break;
		default:
			IPAERR("Trying to print illegal active_clients type");
			break;
		}
	}
	cnt += scnprintf(buf + cnt, size - cnt,
			"\nTotal active clients count: %d\n",
			ipa3_ctx->ipa3_active_clients.cnt);

	return cnt;
}

static int ipa3_active_clients_panic_notifier(struct notifier_block *this,
		unsigned long event, void *ptr)
{
	ipa3_active_clients_lock();
	ipa3_active_clients_log_print_table(active_clients_table_buf,
			IPA3_ACTIVE_CLIENTS_TABLE_BUF_SIZE);
	IPAERR("%s", active_clients_table_buf);
	ipa3_active_clients_unlock();

	return NOTIFY_DONE;
}

static struct notifier_block ipa3_active_clients_panic_blk = {
	.notifier_call  = ipa3_active_clients_panic_notifier,
};

static int ipa3_active_clients_log_insert(const char *string)
{
	int head;
	int tail;

	if (!ipa3_ctx->ipa3_active_clients_logging.log_rdy)
		return -EPERM;

	head = ipa3_ctx->ipa3_active_clients_logging.log_head;
	tail = ipa3_ctx->ipa3_active_clients_logging.log_tail;

	memset(ipa3_ctx->ipa3_active_clients_logging.log_buffer[head], '_',
			IPA3_ACTIVE_CLIENTS_LOG_LINE_LEN);
	strlcpy(ipa3_ctx->ipa3_active_clients_logging.log_buffer[head], string,
			(size_t)IPA3_ACTIVE_CLIENTS_LOG_LINE_LEN);
	head = (head + 1) % IPA3_ACTIVE_CLIENTS_LOG_BUFFER_SIZE_LINES;
	if (tail == head)
		tail = (tail + 1) % IPA3_ACTIVE_CLIENTS_LOG_BUFFER_SIZE_LINES;

	ipa3_ctx->ipa3_active_clients_logging.log_tail = tail;
	ipa3_ctx->ipa3_active_clients_logging.log_head = head;

	return 0;
}

static int ipa3_active_clients_log_init(void)
{
	int i;

	ipa3_ctx->ipa3_active_clients_logging.log_buffer[0] = kzalloc(
			IPA3_ACTIVE_CLIENTS_LOG_BUFFER_SIZE_LINES *
			sizeof(char[IPA3_ACTIVE_CLIENTS_LOG_LINE_LEN]),
			GFP_KERNEL);
	active_clients_table_buf = kzalloc(sizeof(
			char[IPA3_ACTIVE_CLIENTS_TABLE_BUF_SIZE]), GFP_KERNEL);
	if (ipa3_ctx->ipa3_active_clients_logging.log_buffer == NULL) {
		pr_err("Active Clients Logging memory allocation failed");
		goto bail;
	}
	for (i = 0; i < IPA3_ACTIVE_CLIENTS_LOG_BUFFER_SIZE_LINES; i++) {
		ipa3_ctx->ipa3_active_clients_logging.log_buffer[i] =
			ipa3_ctx->ipa3_active_clients_logging.log_buffer[0] +
			(IPA3_ACTIVE_CLIENTS_LOG_LINE_LEN * i);
	}
	ipa3_ctx->ipa3_active_clients_logging.log_head = 0;
	ipa3_ctx->ipa3_active_clients_logging.log_tail =
			IPA3_ACTIVE_CLIENTS_LOG_BUFFER_SIZE_LINES - 1;
	hash_init(ipa3_ctx->ipa3_active_clients_logging.htable);
	atomic_notifier_chain_register(&panic_notifier_list,
			&ipa3_active_clients_panic_blk);
	ipa3_ctx->ipa3_active_clients_logging.log_rdy = 1;

	return 0;

bail:
	return -ENOMEM;
}

void ipa3_active_clients_log_clear(void)
{
	ipa3_active_clients_lock();
	ipa3_ctx->ipa3_active_clients_logging.log_head = 0;
	ipa3_ctx->ipa3_active_clients_logging.log_tail =
			IPA3_ACTIVE_CLIENTS_LOG_BUFFER_SIZE_LINES - 1;
	ipa3_active_clients_unlock();
}

static void ipa3_active_clients_log_destroy(void)
{
	ipa3_ctx->ipa3_active_clients_logging.log_rdy = 0;
	kfree(ipa3_ctx->ipa3_active_clients_logging.log_buffer[0]);
	ipa3_ctx->ipa3_active_clients_logging.log_head = 0;
	ipa3_ctx->ipa3_active_clients_logging.log_tail =
			IPA3_ACTIVE_CLIENTS_LOG_BUFFER_SIZE_LINES - 1;
}

enum ipa_smmu_cb_type {
	IPA_SMMU_CB_AP,
	IPA_SMMU_CB_WLAN,
	IPA_SMMU_CB_UC,
	IPA_SMMU_CB_MAX

};

static struct ipa_smmu_cb_ctx smmu_cb[IPA_SMMU_CB_MAX];

struct iommu_domain *ipa3_get_smmu_domain(void)
{
	if (smmu_cb[IPA_SMMU_CB_AP].valid)
		return smmu_cb[IPA_SMMU_CB_AP].mapping->domain;

	IPAERR("CB not valid\n");

	return NULL;
}

struct iommu_domain *ipa3_get_uc_smmu_domain(void)
{
	struct iommu_domain *domain = NULL;

	if (smmu_cb[IPA_SMMU_CB_UC].valid)
		domain = smmu_cb[IPA_SMMU_CB_UC].mapping->domain;
	else
		IPAERR("CB not valid\n");

	return domain;
}

struct device *ipa3_get_dma_dev(void)
{
	return ipa3_ctx->pdev;
}

struct ipa_smmu_cb_ctx *ipa3_get_wlan_smmu_ctx(void)
{
	return &smmu_cb[IPA_SMMU_CB_WLAN];
}

struct ipa_smmu_cb_ctx *ipa3_get_uc_smmu_ctx(void)
{
	return &smmu_cb[IPA_SMMU_CB_UC];
}

static int ipa3_open(struct inode *inode, struct file *filp)
{
	struct ipa3_context *ctx = NULL;

	IPADBG("ENTER\n");
	ctx = container_of(inode->i_cdev, struct ipa3_context, cdev);
	filp->private_data = ctx;

	return 0;
}

void ipa3_flow_control(enum ipa_client_type ipa_client,
		bool enable, uint32_t qmap_id)
{
	struct ipa_ep_cfg_ctrl ep_ctrl = {0};
	int ep_idx;
	struct ipa3_ep_context *ep;

	
	if (!ipa3_ctx->tethered_flow_control) {
		IPADBG("Apps flow control is not needed\n");
		return;
	}

	
	ep_idx = ipa3_get_ep_mapping(ipa_client);
	if (ep_idx == -1) {
		IPADBG("Invalid IPA client\n");
		return;
	}

	ep = &ipa3_ctx->ep[ep_idx];
	if (!ep->valid || (ep->client != IPA_CLIENT_USB_PROD)) {
		IPADBG("EP not valid/Not applicable for client.\n");
		return;
	}

	spin_lock(&ipa3_ctx->disconnect_lock);
	
	if (ep->cfg.meta.qmap_id != qmap_id) {
		IPADBG("Flow control ind not for same flow: %u %u\n",
			ep->cfg.meta.qmap_id, qmap_id);
		spin_unlock(&ipa3_ctx->disconnect_lock);
		return;
	}
	if (!ep->disconnect_in_progress) {
		if (enable) {
			IPADBG("Enabling Flow\n");
			ep_ctrl.ipa_ep_delay = false;
			IPA_STATS_INC_CNT(ipa3_ctx->stats.flow_enable);
		} else {
			IPADBG("Disabling Flow\n");
			ep_ctrl.ipa_ep_delay = true;
			IPA_STATS_INC_CNT(ipa3_ctx->stats.flow_disable);
		}
		ep_ctrl.ipa_ep_suspend = false;
		ipa3_cfg_ep_ctrl(ep_idx, &ep_ctrl);
	} else {
		IPADBG("EP disconnect is in progress\n");
	}
	spin_unlock(&ipa3_ctx->disconnect_lock);
}

static void ipa3_wan_msg_free_cb(void *buff, u32 len, u32 type)
{
	if (!buff) {
		IPAERR("Null buffer\n");
		return;
	}

	if (type != WAN_UPSTREAM_ROUTE_ADD &&
	    type != WAN_UPSTREAM_ROUTE_DEL &&
	    type != WAN_EMBMS_CONNECT) {
		IPAERR("Wrong type given. buff %p type %d\n", buff, type);
		return;
	}

	kfree(buff);
}

static int ipa3_send_wan_msg(unsigned long usr_param, uint8_t msg_type)
{
	int retval;
	struct ipa_wan_msg *wan_msg;
	struct ipa_msg_meta msg_meta;

	wan_msg = kzalloc(sizeof(struct ipa_wan_msg), GFP_KERNEL);
	if (!wan_msg) {
		IPAERR("no memory\n");
		return -ENOMEM;
	}

	if (copy_from_user((u8 *)wan_msg, (u8 *)usr_param,
		sizeof(struct ipa_wan_msg))) {
		kfree(wan_msg);
		return -EFAULT;
	}

	memset(&msg_meta, 0, sizeof(struct ipa_msg_meta));
	msg_meta.msg_type = msg_type;
	msg_meta.msg_len = sizeof(struct ipa_wan_msg);
	retval = ipa3_send_msg(&msg_meta, wan_msg, ipa3_wan_msg_free_cb);
	if (retval) {
		IPAERR("ipa3_send_msg failed: %d\n", retval);
		kfree(wan_msg);
		return retval;
	}

	return 0;
}


static long ipa3_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	u32 pyld_sz;
	u8 header[128] = { 0 };
	u8 *param = NULL;
	struct ipa_ioc_nat_alloc_mem nat_mem;
	struct ipa_ioc_v4_nat_init nat_init;
	struct ipa_ioc_v4_nat_del nat_del;
	struct ipa_ioc_rm_dependency rm_depend;
	size_t sz;
	int pre_entry;

	IPADBG("cmd=%x nr=%d\n", cmd, _IOC_NR(cmd));

	if (!ipa3_is_ready()) {
		IPAERR("IPA not ready, waiting for init completion\n");
		wait_for_completion(&ipa3_ctx->init_completion_obj);
	}

	if (_IOC_TYPE(cmd) != IPA_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) >= IPA_IOCTL_MAX)
		return -ENOTTY;

	IPA_ACTIVE_CLIENTS_INC_SIMPLE();

	switch (cmd) {
	case IPA_IOC_ALLOC_NAT_MEM:
		if (copy_from_user((u8 *)&nat_mem, (u8 *)arg,
					sizeof(struct ipa_ioc_nat_alloc_mem))) {
			retval = -EFAULT;
			break;
		}
		
		nat_mem.dev_name[IPA_RESOURCE_NAME_MAX - 1] = '\0';

		if (ipa3_allocate_nat_device(&nat_mem)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, (u8 *)&nat_mem,
					sizeof(struct ipa_ioc_nat_alloc_mem))) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_V4_INIT_NAT:
		if (copy_from_user((u8 *)&nat_init, (u8 *)arg,
					sizeof(struct ipa_ioc_v4_nat_init))) {
			retval = -EFAULT;
			break;
		}
		if (ipa3_nat_init_cmd(&nat_init)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_NAT_DMA:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_nat_dma_cmd))) {
			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_nat_dma_cmd *)header)->entries;
		pyld_sz =
		   sizeof(struct ipa_ioc_nat_dma_cmd) +
		   pre_entry * sizeof(struct ipa_ioc_nat_dma_one);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}

		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_nat_dma_cmd *)param)->entries
			!= pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_nat_dma_cmd *)param)->entries,
				pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_nat_dma_cmd((struct ipa_ioc_nat_dma_cmd *)param)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_V4_DEL_NAT:
		if (copy_from_user((u8 *)&nat_del, (u8 *)arg,
					sizeof(struct ipa_ioc_v4_nat_del))) {
			retval = -EFAULT;
			break;
		}
		if (ipa3_nat_del_cmd(&nat_del)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_ADD_HDR:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_add_hdr))) {
			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_add_hdr *)header)->num_hdrs;
		pyld_sz =
		   sizeof(struct ipa_ioc_add_hdr) +
		   pre_entry * sizeof(struct ipa_hdr_add);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_add_hdr *)param)->num_hdrs
			!= pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_add_hdr *)param)->num_hdrs,
				pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_add_hdr((struct ipa_ioc_add_hdr *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_DEL_HDR:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_del_hdr))) {
			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_del_hdr *)header)->num_hdls;
		pyld_sz =
		   sizeof(struct ipa_ioc_del_hdr) +
		   pre_entry * sizeof(struct ipa_hdr_del);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_del_hdr *)param)->num_hdls
			!= pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_del_hdr *)param)->num_hdls,
				pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_del_hdr((struct ipa_ioc_del_hdr *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_ADD_RT_RULE:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_add_rt_rule))) {
			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_add_rt_rule *)header)->num_rules;
		pyld_sz =
		   sizeof(struct ipa_ioc_add_rt_rule) +
		   pre_entry * sizeof(struct ipa_rt_rule_add);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_add_rt_rule *)param)->num_rules
			!= pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_add_rt_rule *)param)->
				num_rules,
				pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_add_rt_rule((struct ipa_ioc_add_rt_rule *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_ADD_RT_RULE_AFTER:
		if (copy_from_user(header, (u8 *)arg,
			sizeof(struct ipa_ioc_add_rt_rule_after))) {

			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_add_rt_rule_after *)header)->num_rules;
		pyld_sz =
		   sizeof(struct ipa_ioc_add_rt_rule_after) +
		   pre_entry * sizeof(struct ipa_rt_rule_add);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_add_rt_rule_after *)param)->
			num_rules != pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_add_rt_rule_after *)param)->
				num_rules,
				pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_add_rt_rule_after(
			(struct ipa_ioc_add_rt_rule_after *)param)) {

			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_MDFY_RT_RULE:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_mdfy_rt_rule))) {
			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_mdfy_rt_rule *)header)->num_rules;
		pyld_sz =
		   sizeof(struct ipa_ioc_mdfy_rt_rule) +
		   pre_entry * sizeof(struct ipa_rt_rule_mdfy);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_mdfy_rt_rule *)param)->num_rules
			!= pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_mdfy_rt_rule *)param)->
				num_rules,
				pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_mdfy_rt_rule((struct ipa_ioc_mdfy_rt_rule *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_DEL_RT_RULE:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_del_rt_rule))) {
			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_del_rt_rule *)header)->num_hdls;
		pyld_sz =
		   sizeof(struct ipa_ioc_del_rt_rule) +
		   pre_entry * sizeof(struct ipa_rt_rule_del);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_del_rt_rule *)param)->num_hdls
			!= pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_del_rt_rule *)param)->num_hdls,
				pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_del_rt_rule((struct ipa_ioc_del_rt_rule *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_ADD_FLT_RULE:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_add_flt_rule))) {
			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_add_flt_rule *)header)->num_rules;
		pyld_sz =
		   sizeof(struct ipa_ioc_add_flt_rule) +
		   pre_entry * sizeof(struct ipa_flt_rule_add);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_add_flt_rule *)param)->num_rules
			!= pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_add_flt_rule *)param)->
				num_rules,
				pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_add_flt_rule((struct ipa_ioc_add_flt_rule *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_ADD_FLT_RULE_AFTER:
		if (copy_from_user(header, (u8 *)arg,
				sizeof(struct ipa_ioc_add_flt_rule_after))) {

			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_add_flt_rule_after *)header)->
			num_rules;
		pyld_sz =
		   sizeof(struct ipa_ioc_add_flt_rule_after) +
		   pre_entry * sizeof(struct ipa_flt_rule_add);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_add_flt_rule_after *)param)->
			num_rules != pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_add_flt_rule_after *)param)->
				num_rules,
				pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_add_flt_rule_after(
				(struct ipa_ioc_add_flt_rule_after *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_DEL_FLT_RULE:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_del_flt_rule))) {
			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_del_flt_rule *)header)->num_hdls;
		pyld_sz =
		   sizeof(struct ipa_ioc_del_flt_rule) +
		   pre_entry * sizeof(struct ipa_flt_rule_del);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_del_flt_rule *)param)->num_hdls
			!= pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_del_flt_rule *)param)->
				num_hdls,
				pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_del_flt_rule((struct ipa_ioc_del_flt_rule *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_MDFY_FLT_RULE:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_mdfy_flt_rule))) {
			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_mdfy_flt_rule *)header)->num_rules;
		pyld_sz =
		   sizeof(struct ipa_ioc_mdfy_flt_rule) +
		   pre_entry * sizeof(struct ipa_flt_rule_mdfy);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_mdfy_flt_rule *)param)->num_rules
			!= pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_mdfy_flt_rule *)param)->
				num_rules,
				pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_mdfy_flt_rule((struct ipa_ioc_mdfy_flt_rule *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_COMMIT_HDR:
		retval = ipa3_commit_hdr();
		break;
	case IPA_IOC_RESET_HDR:
		retval = ipa3_reset_hdr();
		break;
	case IPA_IOC_COMMIT_RT:
		retval = ipa3_commit_rt(arg);
		break;
	case IPA_IOC_RESET_RT:
		retval = ipa3_reset_rt(arg);
		break;
	case IPA_IOC_COMMIT_FLT:
		retval = ipa3_commit_flt(arg);
		break;
	case IPA_IOC_RESET_FLT:
		retval = ipa3_reset_flt(arg);
		break;
	case IPA_IOC_GET_RT_TBL:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_get_rt_tbl))) {
			retval = -EFAULT;
			break;
		}
		if (ipa3_get_rt_tbl((struct ipa_ioc_get_rt_tbl *)header)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, header,
					sizeof(struct ipa_ioc_get_rt_tbl))) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_PUT_RT_TBL:
		retval = ipa3_put_rt_tbl(arg);
		break;
	case IPA_IOC_GET_HDR:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_get_hdr))) {
			retval = -EFAULT;
			break;
		}
		if (ipa3_get_hdr((struct ipa_ioc_get_hdr *)header)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, header,
					sizeof(struct ipa_ioc_get_hdr))) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_PUT_HDR:
		retval = ipa3_put_hdr(arg);
		break;
	case IPA_IOC_SET_FLT:
		retval = ipa3_cfg_filter(arg);
		break;
	case IPA_IOC_COPY_HDR:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_copy_hdr))) {
			retval = -EFAULT;
			break;
		}
		if (ipa3_copy_hdr((struct ipa_ioc_copy_hdr *)header)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, header,
					sizeof(struct ipa_ioc_copy_hdr))) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_QUERY_INTF:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_query_intf))) {
			retval = -EFAULT;
			break;
		}
		if (ipa3_query_intf((struct ipa_ioc_query_intf *)header)) {
			retval = -1;
			break;
		}
		if (copy_to_user((u8 *)arg, header,
					sizeof(struct ipa_ioc_query_intf))) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_QUERY_INTF_TX_PROPS:
		sz = sizeof(struct ipa_ioc_query_intf_tx_props);
		if (copy_from_user(header, (u8 *)arg, sz)) {
			retval = -EFAULT;
			break;
		}

		if (((struct ipa_ioc_query_intf_tx_props *)header)->num_tx_props
				> IPA_NUM_PROPS_MAX) {
			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_query_intf_tx_props *)
			header)->num_tx_props;
		pyld_sz = sz + pre_entry *
			sizeof(struct ipa_ioc_tx_intf_prop);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_query_intf_tx_props *)
			param)->num_tx_props
			!= pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_query_intf_tx_props *)
				param)->num_tx_props, pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_query_intf_tx_props(
				(struct ipa_ioc_query_intf_tx_props *)param)) {
			retval = -1;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_QUERY_INTF_RX_PROPS:
		sz = sizeof(struct ipa_ioc_query_intf_rx_props);
		if (copy_from_user(header, (u8 *)arg, sz)) {
			retval = -EFAULT;
			break;
		}

		if (((struct ipa_ioc_query_intf_rx_props *)header)->num_rx_props
				> IPA_NUM_PROPS_MAX) {
			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_query_intf_rx_props *)
			header)->num_rx_props;
		pyld_sz = sz + pre_entry *
			sizeof(struct ipa_ioc_rx_intf_prop);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_query_intf_rx_props *)
			param)->num_rx_props != pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_query_intf_rx_props *)
				param)->num_rx_props, pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_query_intf_rx_props(
				(struct ipa_ioc_query_intf_rx_props *)param)) {
			retval = -1;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_QUERY_INTF_EXT_PROPS:
		sz = sizeof(struct ipa_ioc_query_intf_ext_props);
		if (copy_from_user(header, (u8 *)arg, sz)) {
			retval = -EFAULT;
			break;
		}

		if (((struct ipa_ioc_query_intf_ext_props *)
				header)->num_ext_props > IPA_NUM_PROPS_MAX) {
			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_query_intf_ext_props *)
			header)->num_ext_props;
		pyld_sz = sz + pre_entry *
			sizeof(struct ipa_ioc_ext_intf_prop);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_query_intf_ext_props *)
			param)->num_ext_props != pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_query_intf_ext_props *)
				param)->num_ext_props, pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_query_intf_ext_props(
				(struct ipa_ioc_query_intf_ext_props *)param)) {
			retval = -1;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_PULL_MSG:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_msg_meta))) {
			retval = -EFAULT;
			break;
		}
		pre_entry =
		   ((struct ipa_msg_meta *)header)->msg_len;
		pyld_sz = sizeof(struct ipa_msg_meta) +
		   pre_entry;
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_msg_meta *)param)->msg_len
			!= pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_msg_meta *)param)->msg_len,
				pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_pull_msg((struct ipa_msg_meta *)param,
				 (char *)param + sizeof(struct ipa_msg_meta),
				 ((struct ipa_msg_meta *)param)->msg_len) !=
		       ((struct ipa_msg_meta *)param)->msg_len) {
			retval = -1;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_RM_ADD_DEPENDENCY:
		if (copy_from_user((u8 *)&rm_depend, (u8 *)arg,
				sizeof(struct ipa_ioc_rm_dependency))) {
			retval = -EFAULT;
			break;
		}
		retval = ipa3_rm_add_dependency(rm_depend.resource_name,
						rm_depend.depends_on_name);
		break;
	case IPA_IOC_RM_DEL_DEPENDENCY:
		if (copy_from_user((u8 *)&rm_depend, (u8 *)arg,
				sizeof(struct ipa_ioc_rm_dependency))) {
			retval = -EFAULT;
			break;
		}
		retval = ipa3_rm_delete_dependency(rm_depend.resource_name,
						rm_depend.depends_on_name);
		break;
	case IPA_IOC_GENERATE_FLT_EQ:
		{
			struct ipa_ioc_generate_flt_eq flt_eq;

			if (copy_from_user(&flt_eq, (u8 *)arg,
				sizeof(struct ipa_ioc_generate_flt_eq))) {
				retval = -EFAULT;
				break;
			}
			if (ipa3_generate_flt_eq(flt_eq.ip, &flt_eq.attrib,
						&flt_eq.eq_attrib)) {
				retval = -EFAULT;
				break;
			}
			if (copy_to_user((u8 *)arg, &flt_eq,
				sizeof(struct ipa_ioc_generate_flt_eq))) {
				retval = -EFAULT;
				break;
			}
			break;
		}
	case IPA_IOC_QUERY_EP_MAPPING:
		{
			retval = ipa3_get_ep_mapping(arg);
			break;
		}
	case IPA_IOC_QUERY_RT_TBL_INDEX:
		if (copy_from_user(header, (u8 *)arg,
				sizeof(struct ipa_ioc_get_rt_tbl_indx))) {
			retval = -EFAULT;
			break;
		}
		if (ipa3_query_rt_index(
			 (struct ipa_ioc_get_rt_tbl_indx *)header)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, header,
				sizeof(struct ipa_ioc_get_rt_tbl_indx))) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_WRITE_QMAPID:
		if (copy_from_user(header, (u8 *)arg,
					sizeof(struct ipa_ioc_write_qmapid))) {
			retval = -EFAULT;
			break;
		}
		if (ipa3_write_qmap_id((struct ipa_ioc_write_qmapid *)header)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, header,
					sizeof(struct ipa_ioc_write_qmapid))) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_NOTIFY_WAN_UPSTREAM_ROUTE_ADD:
		retval = ipa3_send_wan_msg(arg, WAN_UPSTREAM_ROUTE_ADD);
		if (retval) {
			IPAERR("ipa3_send_wan_msg failed: %d\n", retval);
			break;
		}
		break;
	case IPA_IOC_NOTIFY_WAN_UPSTREAM_ROUTE_DEL:
		retval = ipa3_send_wan_msg(arg, WAN_UPSTREAM_ROUTE_DEL);
		if (retval) {
			IPAERR("ipa3_send_wan_msg failed: %d\n", retval);
			break;
		}
		break;
	case IPA_IOC_NOTIFY_WAN_EMBMS_CONNECTED:
		retval = ipa3_send_wan_msg(arg, WAN_EMBMS_CONNECT);
		if (retval) {
			IPAERR("ipa3_send_wan_msg failed: %d\n", retval);
			break;
		}
		break;
	case IPA_IOC_ADD_HDR_PROC_CTX:
		if (copy_from_user(header, (u8 *)arg,
			sizeof(struct ipa_ioc_add_hdr_proc_ctx))) {
			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_add_hdr_proc_ctx *)
			header)->num_proc_ctxs;
		pyld_sz =
		   sizeof(struct ipa_ioc_add_hdr_proc_ctx) +
		   pre_entry * sizeof(struct ipa_hdr_proc_ctx_add);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_add_hdr_proc_ctx *)
			param)->num_proc_ctxs != pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_add_hdr_proc_ctx *)
				param)->num_proc_ctxs, pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_add_hdr_proc_ctx(
			(struct ipa_ioc_add_hdr_proc_ctx *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;
	case IPA_IOC_DEL_HDR_PROC_CTX:
		if (copy_from_user(header, (u8 *)arg,
			sizeof(struct ipa_ioc_del_hdr_proc_ctx))) {
			retval = -EFAULT;
			break;
		}
		pre_entry =
			((struct ipa_ioc_del_hdr_proc_ctx *)header)->num_hdls;
		pyld_sz =
		   sizeof(struct ipa_ioc_del_hdr_proc_ctx) +
		   pre_entry * sizeof(struct ipa_hdr_proc_ctx_del);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		if (copy_from_user(param, (u8 *)arg, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		
		if (unlikely(((struct ipa_ioc_del_hdr_proc_ctx *)
			param)->num_hdls != pre_entry)) {
			IPAERR("current %d pre %d\n",
				((struct ipa_ioc_del_hdr_proc_ctx *)param)->
				num_hdls,
				pre_entry);
			retval = -EFAULT;
			break;
		}
		if (ipa3_del_hdr_proc_ctx(
			(struct ipa_ioc_del_hdr_proc_ctx *)param)) {
			retval = -EFAULT;
			break;
		}
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	case IPA_IOC_GET_HW_VERSION:
		pyld_sz = sizeof(enum ipa_hw_type);
		param = kzalloc(pyld_sz, GFP_KERNEL);
		if (!param) {
			retval = -ENOMEM;
			break;
		}
		memcpy(param, &ipa3_ctx->ipa_hw_type, pyld_sz);
		if (copy_to_user((u8 *)arg, param, pyld_sz)) {
			retval = -EFAULT;
			break;
		}
		break;

	default:        
		IPA_ACTIVE_CLIENTS_DEC_SIMPLE();
		return -ENOTTY;
	}
	kfree(param);
	IPA_ACTIVE_CLIENTS_DEC_SIMPLE();

	return retval;
}

int ipa3_setup_dflt_rt_tables(void)
{
	struct ipa_ioc_add_rt_rule *rt_rule;
	struct ipa_rt_rule_add *rt_rule_entry;

	rt_rule =
	   kzalloc(sizeof(struct ipa_ioc_add_rt_rule) + 1 *
			   sizeof(struct ipa_rt_rule_add), GFP_KERNEL);
	if (!rt_rule) {
		IPAERR("fail to alloc mem\n");
		return -ENOMEM;
	}
	
	rt_rule->num_rules = 1;
	rt_rule->commit = 1;
	rt_rule->ip = IPA_IP_v4;
	strlcpy(rt_rule->rt_tbl_name, IPA_DFLT_RT_TBL_NAME,
			IPA_RESOURCE_NAME_MAX);

	rt_rule_entry = &rt_rule->rules[0];
	rt_rule_entry->at_rear = 1;
	rt_rule_entry->rule.dst = IPA_CLIENT_APPS_LAN_CONS;
	rt_rule_entry->rule.hdr_hdl = ipa3_ctx->excp_hdr_hdl;
	rt_rule_entry->rule.retain_hdr = 1;

	if (ipa3_add_rt_rule(rt_rule)) {
		IPAERR("fail to add dflt v4 rule\n");
		kfree(rt_rule);
		return -EPERM;
	}
	IPADBG("dflt v4 rt rule hdl=%x\n", rt_rule_entry->rt_rule_hdl);
	ipa3_ctx->dflt_v4_rt_rule_hdl = rt_rule_entry->rt_rule_hdl;

	
	rt_rule->ip = IPA_IP_v6;
	if (ipa3_add_rt_rule(rt_rule)) {
		IPAERR("fail to add dflt v6 rule\n");
		kfree(rt_rule);
		return -EPERM;
	}
	IPADBG("dflt v6 rt rule hdl=%x\n", rt_rule_entry->rt_rule_hdl);
	ipa3_ctx->dflt_v6_rt_rule_hdl = rt_rule_entry->rt_rule_hdl;


	kfree(rt_rule);

	return 0;
}

static int ipa3_setup_exception_path(void)
{
	struct ipa_ioc_add_hdr *hdr;
	struct ipa_hdr_add *hdr_entry;
	struct ipa3_route route = { 0 };
	int ret;

	
	hdr = kzalloc(sizeof(struct ipa_ioc_add_hdr) + 1 *
		      sizeof(struct ipa_hdr_add), GFP_KERNEL);
	if (!hdr) {
		IPAERR("fail to alloc exception hdr\n");
		return -ENOMEM;
	}
	hdr->num_hdrs = 1;
	hdr->commit = 1;
	hdr_entry = &hdr->hdr[0];

	strlcpy(hdr_entry->name, IPA_LAN_RX_HDR_NAME, IPA_RESOURCE_NAME_MAX);
	hdr_entry->hdr_len = IPA_LAN_RX_HEADER_LENGTH;

	if (ipa3_add_hdr(hdr)) {
		IPAERR("fail to add exception hdr\n");
		ret = -EPERM;
		goto bail;
	}

	if (hdr_entry->status) {
		IPAERR("fail to add exception hdr\n");
		ret = -EPERM;
		goto bail;
	}

	ipa3_ctx->excp_hdr_hdl = hdr_entry->hdr_hdl;

	
	route.route_def_pipe = ipa3_get_ep_mapping(IPA_CLIENT_APPS_LAN_CONS);
	route.route_frag_def_pipe = ipa3_get_ep_mapping(
		IPA_CLIENT_APPS_LAN_CONS);
	route.route_def_hdr_table = !ipa3_ctx->hdr_tbl_lcl;
	route.route_def_retain_hdr = 1;

	if (ipa3_cfg_route(&route)) {
		IPAERR("fail to add exception hdr\n");
		ret = -EPERM;
		goto bail;
	}

	ret = 0;
bail:
	kfree(hdr);
	return ret;
}

static int ipa3_init_smem_region(int memory_region_size,
				int memory_region_offset)
{
	struct ipa3_hw_imm_cmd_dma_shared_mem cmd;
	struct ipa3_desc desc;
	struct ipa3_mem_buffer mem;
	int rc;

	if (memory_region_size == 0)
		return 0;

	memset(&desc, 0, sizeof(desc));
	memset(&cmd, 0, sizeof(cmd));
	memset(&mem, 0, sizeof(mem));

	mem.size = memory_region_size;
	mem.base = dma_alloc_coherent(ipa3_ctx->pdev, mem.size,
		&mem.phys_base, GFP_KERNEL);
	if (!mem.base) {
		IPAERR("failed to alloc DMA buff of size %d\n", mem.size);
		return -ENOMEM;
	}

	memset(mem.base, 0, mem.size);
	cmd.skip_pipeline_clear = 0;
	cmd.pipeline_clear_options = IPA_HPS_CLEAR;
	cmd.size = mem.size;
	cmd.system_addr = mem.phys_base;
	cmd.local_addr = ipa3_ctx->smem_restricted_bytes +
		memory_region_offset;
	desc.opcode = IPA_DMA_SHARED_MEM;
	desc.pyld = &cmd;
	desc.len = sizeof(cmd);
	desc.type = IPA_IMM_CMD_DESC;

	rc = ipa3_send_cmd(1, &desc);
	if (rc) {
		IPAERR("failed to send immediate command (error %d)\n", rc);
		rc = -EFAULT;
	}

	dma_free_coherent(ipa3_ctx->pdev, mem.size, mem.base,
		mem.phys_base);

	return rc;
}

int ipa3_init_q6_smem(void)
{
	int rc;

	IPA_ACTIVE_CLIENTS_INC_SIMPLE();

	rc = ipa3_init_smem_region(IPA_MEM_PART(modem_size),
		IPA_MEM_PART(modem_ofst));
	if (rc) {
		IPAERR("failed to initialize Modem RAM memory\n");
		IPA_ACTIVE_CLIENTS_DEC_SIMPLE();
		return rc;
	}

	rc = ipa3_init_smem_region(IPA_MEM_PART(modem_hdr_size),
		IPA_MEM_PART(modem_hdr_ofst));
	if (rc) {
		IPAERR("failed to initialize Modem HDRs RAM memory\n");
		IPA_ACTIVE_CLIENTS_DEC_SIMPLE();
		return rc;
	}

	rc = ipa3_init_smem_region(IPA_MEM_PART(modem_hdr_proc_ctx_size),
		IPA_MEM_PART(modem_hdr_proc_ctx_ofst));
	if (rc) {
		IPAERR("failed to initialize Modem proc ctx RAM memory\n");
		IPA_ACTIVE_CLIENTS_DEC_SIMPLE();
		return rc;
	}

	rc = ipa3_init_smem_region(IPA_MEM_PART(modem_comp_decomp_size),
		IPA_MEM_PART(modem_comp_decomp_ofst));
	if (rc) {
		IPAERR("failed to initialize Modem Comp/Decomp RAM memory\n");
		IPA_ACTIVE_CLIENTS_DEC_SIMPLE();
		return rc;
	}
	IPA_ACTIVE_CLIENTS_DEC_SIMPLE();

	return rc;
}

static void ipa3_free_buffer(void *user1, int user2)
{
	kfree(user1);
}

static int ipa3_q6_pipe_delay(void)
{
	u32 reg_val = 0;
	int client_idx;
	int ep_idx;

	for (client_idx = 0; client_idx < IPA_CLIENT_MAX; client_idx++) {
		if (IPA_CLIENT_IS_Q6_PROD(client_idx)) {
			ep_idx = ipa3_get_ep_mapping(client_idx);
			if (ep_idx == -1)
				continue;

			IPA_SETFIELD_IN_REG(reg_val, 1,
				IPA_ENDP_INIT_CTRL_N_ENDP_DELAY_SHFT,
				IPA_ENDP_INIT_CTRL_N_ENDP_DELAY_BMSK);

			ipa_write_reg(ipa3_ctx->mmio,
				IPA_ENDP_INIT_CTRL_N_OFST(ep_idx), reg_val);
		}
	}

	return 0;
}

static int ipa3_q6_avoid_holb(void)
{
	u32 reg_val;
	int ep_idx;
	int client_idx;
	struct ipa_ep_cfg_ctrl avoid_holb;

	memset(&avoid_holb, 0, sizeof(avoid_holb));
	avoid_holb.ipa_ep_suspend = true;

	for (client_idx = 0; client_idx < IPA_CLIENT_MAX; client_idx++) {
		if (IPA_CLIENT_IS_Q6_CONS(client_idx)) {
			ep_idx = ipa3_get_ep_mapping(client_idx);
			if (ep_idx == -1)
				continue;

			reg_val = 0;
			IPA_SETFIELD_IN_REG(reg_val, 0,
				IPA_ENDP_INIT_HOL_BLOCK_TIMER_N_TIMER_SHFT,
				IPA_ENDP_INIT_HOL_BLOCK_TIMER_N_TIMER_BMSK);

			ipa_write_reg(ipa3_ctx->mmio,
			IPA_ENDP_INIT_HOL_BLOCK_TIMER_N_OFST_v3_0(ep_idx),
				reg_val);

			reg_val = 0;
			IPA_SETFIELD_IN_REG(reg_val, 1,
				IPA_ENDP_INIT_HOL_BLOCK_EN_N_EN_SHFT,
				IPA_ENDP_INIT_HOL_BLOCK_EN_N_EN_BMSK);

			ipa_write_reg(ipa3_ctx->mmio,
				IPA_ENDP_INIT_HOL_BLOCK_EN_N_OFST_v3_0(ep_idx),
				reg_val);

			ipa3_cfg_ep_ctrl(ep_idx, &avoid_holb);
		}
	}

	return 0;
}

static u32 ipa3_get_max_flt_rt_cmds(u32 num_pipes)
{
	u32 max_cmds = 0;

	max_cmds += ipa3_ctx->ep_flt_num * 4;

	
	max_cmds += ((IPA_MEM_PART(v4_modem_rt_index_hi) -
		     IPA_MEM_PART(v4_modem_rt_index_lo) + 1) * 2);

	max_cmds += ((IPA_MEM_PART(v6_modem_rt_index_hi) -
		     IPA_MEM_PART(v6_modem_rt_index_lo) + 1) * 2);

	return max_cmds;
}

static int ipa3_q6_clean_q6_tables(void)
{
	struct ipa3_desc *desc;
	struct ipa3_hw_imm_cmd_dma_shared_mem *cmd = NULL;
	int pipe_idx;
	int num_cmds = 0;
	int index;
	int retval;
	struct ipa3_mem_buffer mem = { 0 };
	u64 *entry;
	u32 max_cmds = ipa3_get_max_flt_rt_cmds(ipa3_ctx->ipa_num_pipes);
	int flt_idx = 0;

	mem.size = IPA_HW_TBL_HDR_WIDTH;
	mem.base = dma_alloc_coherent(ipa3_ctx->pdev, mem.size,
		&mem.phys_base, GFP_KERNEL);
	if (!mem.base) {
		IPAERR("failed to alloc DMA buff of size %d\n", mem.size);
		return -ENOMEM;
	}

	entry = mem.base;
	*entry = ipa3_ctx->empty_rt_tbl_mem.phys_base;

	desc = kcalloc(max_cmds, sizeof(struct ipa3_desc), GFP_KERNEL);
	if (!desc) {
		IPAERR("failed to allocate memory\n");
		retval = -ENOMEM;
		goto bail_dma;
	}

	cmd = kcalloc(max_cmds, sizeof(struct ipa3_hw_imm_cmd_dma_shared_mem),
		GFP_KERNEL);
	if (!cmd) {
		IPAERR("failed to allocate memory\n");
		retval = -ENOMEM;
		goto bail_desc;
	}

	for (pipe_idx = 0; pipe_idx < ipa3_ctx->ipa_num_pipes; pipe_idx++) {
		if (!ipa_is_ep_support_flt(pipe_idx))
			continue;

		if (!ipa3_ctx->ep[pipe_idx].valid ||
		    ipa3_ctx->ep[pipe_idx].skip_ep_cfg) {
			cmd[num_cmds].skip_pipeline_clear = 0;
			cmd[num_cmds].pipeline_clear_options =
				IPA_FULL_PIPELINE_CLEAR;
			cmd[num_cmds].size = mem.size;
			cmd[num_cmds].system_addr = mem.phys_base;
			cmd[num_cmds].local_addr =
				ipa3_ctx->smem_restricted_bytes +
				IPA_MEM_PART(v4_flt_hash_ofst) +
				IPA_HW_TBL_HDR_WIDTH +
				flt_idx * IPA_HW_TBL_HDR_WIDTH;

			desc[num_cmds].opcode = IPA_DMA_SHARED_MEM;
			desc[num_cmds].pyld = &cmd[num_cmds];
			desc[num_cmds].len = sizeof(*cmd);
			desc[num_cmds].type = IPA_IMM_CMD_DESC;
			num_cmds++;

			cmd[num_cmds].skip_pipeline_clear = 0;
			cmd[num_cmds].pipeline_clear_options =
				IPA_FULL_PIPELINE_CLEAR;
			cmd[num_cmds].size = mem.size;
			cmd[num_cmds].system_addr =  mem.phys_base;
			cmd[num_cmds].local_addr =
				ipa3_ctx->smem_restricted_bytes +
				IPA_MEM_PART(v6_flt_hash_ofst) +
				IPA_HW_TBL_HDR_WIDTH +
				flt_idx * IPA_HW_TBL_HDR_WIDTH;

			desc[num_cmds].opcode = IPA_DMA_SHARED_MEM;
			desc[num_cmds].pyld = &cmd[num_cmds];
			desc[num_cmds].len = sizeof(*cmd);
			desc[num_cmds].type = IPA_IMM_CMD_DESC;
			num_cmds++;

			cmd[num_cmds].skip_pipeline_clear = 0;
			cmd[num_cmds].pipeline_clear_options =
				IPA_FULL_PIPELINE_CLEAR;
			cmd[num_cmds].size = mem.size;
			cmd[num_cmds].system_addr = mem.phys_base;
			cmd[num_cmds].local_addr =
				ipa3_ctx->smem_restricted_bytes +
				IPA_MEM_PART(v4_flt_nhash_ofst) +
				IPA_HW_TBL_HDR_WIDTH +
				flt_idx * IPA_HW_TBL_HDR_WIDTH;

			desc[num_cmds].opcode = IPA_DMA_SHARED_MEM;
			desc[num_cmds].pyld = &cmd[num_cmds];
			desc[num_cmds].len = sizeof(*cmd);
			desc[num_cmds].type = IPA_IMM_CMD_DESC;
			num_cmds++;

			cmd[num_cmds].skip_pipeline_clear = 0;
			cmd[num_cmds].pipeline_clear_options =
				IPA_FULL_PIPELINE_CLEAR;
			cmd[num_cmds].size = mem.size;
			cmd[num_cmds].system_addr =  mem.phys_base;
			cmd[num_cmds].local_addr =
				ipa3_ctx->smem_restricted_bytes +
				IPA_MEM_PART(v6_flt_nhash_ofst) +
				IPA_HW_TBL_HDR_WIDTH +
				flt_idx * IPA_HW_TBL_HDR_WIDTH;

			desc[num_cmds].opcode = IPA_DMA_SHARED_MEM;
			desc[num_cmds].pyld = &cmd[num_cmds];
			desc[num_cmds].len = sizeof(*cmd);
			desc[num_cmds].type = IPA_IMM_CMD_DESC;
			num_cmds++;
		}

		flt_idx++;
	}

	
	for (index = IPA_MEM_PART(v4_modem_rt_index_lo);
		 index <= IPA_MEM_PART(v4_modem_rt_index_hi);
		 index++) {
		cmd[num_cmds].skip_pipeline_clear = 0;
		cmd[num_cmds].pipeline_clear_options =
			IPA_FULL_PIPELINE_CLEAR;
		cmd[num_cmds].size = mem.size;
		cmd[num_cmds].system_addr =  mem.phys_base;
		cmd[num_cmds].local_addr = ipa3_ctx->smem_restricted_bytes +
			IPA_MEM_PART(v4_rt_hash_ofst) +
			index * IPA_HW_TBL_HDR_WIDTH;

		desc[num_cmds].opcode = IPA_DMA_SHARED_MEM;
		desc[num_cmds].pyld = &cmd[num_cmds];
		desc[num_cmds].len = sizeof(*cmd);
		desc[num_cmds].type = IPA_IMM_CMD_DESC;
		num_cmds++;

		cmd[num_cmds].skip_pipeline_clear = 0;
		cmd[num_cmds].pipeline_clear_options =
			IPA_FULL_PIPELINE_CLEAR;
		cmd[num_cmds].size = mem.size;
		cmd[num_cmds].system_addr =  mem.phys_base;
		cmd[num_cmds].local_addr = ipa3_ctx->smem_restricted_bytes +
			IPA_MEM_PART(v4_rt_nhash_ofst) +
			index * IPA_HW_TBL_HDR_WIDTH;

		desc[num_cmds].opcode = IPA_DMA_SHARED_MEM;
		desc[num_cmds].pyld = &cmd[num_cmds];
		desc[num_cmds].len = sizeof(*cmd);
		desc[num_cmds].type = IPA_IMM_CMD_DESC;
		num_cmds++;
	}

	for (index = IPA_MEM_PART(v6_modem_rt_index_lo);
		 index <= IPA_MEM_PART(v6_modem_rt_index_hi);
		 index++) {
		cmd[num_cmds].skip_pipeline_clear = 0;
		cmd[num_cmds].pipeline_clear_options =
			IPA_FULL_PIPELINE_CLEAR;
		cmd[num_cmds].size = mem.size;
		cmd[num_cmds].system_addr =  mem.phys_base;
		cmd[num_cmds].local_addr = ipa3_ctx->smem_restricted_bytes +
			IPA_MEM_PART(v6_rt_hash_ofst) +
			index * IPA_HW_TBL_HDR_WIDTH;

		desc[num_cmds].opcode = IPA_DMA_SHARED_MEM;
		desc[num_cmds].pyld = &cmd[num_cmds];
		desc[num_cmds].len = sizeof(*cmd);
		desc[num_cmds].type = IPA_IMM_CMD_DESC;
		num_cmds++;

		cmd[num_cmds].skip_pipeline_clear = 0;
		cmd[num_cmds].pipeline_clear_options =
			IPA_FULL_PIPELINE_CLEAR;
		cmd[num_cmds].size = mem.size;
		cmd[num_cmds].system_addr =  mem.phys_base;
		cmd[num_cmds].local_addr = ipa3_ctx->smem_restricted_bytes +
			IPA_MEM_PART(v6_rt_nhash_ofst) +
			index * IPA_HW_TBL_HDR_WIDTH;

		desc[num_cmds].opcode = IPA_DMA_SHARED_MEM;
		desc[num_cmds].pyld = &cmd[num_cmds];
		desc[num_cmds].len = sizeof(*cmd);
		desc[num_cmds].type = IPA_IMM_CMD_DESC;
		num_cmds++;
	}

	retval = ipa3_send_cmd(num_cmds, desc);
	if (retval) {
		IPAERR("failed to send immediate command (error %d)\n", retval);
		retval = -EFAULT;
	}

	kfree(cmd);

bail_desc:
	kfree(desc);

bail_dma:
	dma_free_coherent(ipa3_ctx->pdev, mem.size, mem.base, mem.phys_base);

	return retval;
}

static void ipa3_q6_disable_agg_reg(struct ipa3_register_write *reg_write,
				   int ep_idx)
{
	reg_write->skip_pipeline_clear = 0;
	reg_write->pipeline_clear_options = IPA_FULL_PIPELINE_CLEAR;

	reg_write->offset = IPA_ENDP_INIT_AGGR_N_OFST_v3_0(ep_idx);
	reg_write->value =
		(1 & IPA_ENDP_INIT_AGGR_N_AGGR_FORCE_CLOSE_BMSK) <<
		IPA_ENDP_INIT_AGGR_N_AGGR_FORCE_CLOSE_SHFT;
	reg_write->value_mask =
		IPA_ENDP_INIT_AGGR_N_AGGR_FORCE_CLOSE_BMSK <<
		IPA_ENDP_INIT_AGGR_N_AGGR_FORCE_CLOSE_SHFT;

	reg_write->value |=
		((0 & IPA_ENDP_INIT_AGGR_N_AGGR_EN_BMSK) <<
		IPA_ENDP_INIT_AGGR_N_AGGR_EN_SHFT);
	reg_write->value_mask |=
		((IPA_ENDP_INIT_AGGR_N_AGGR_EN_BMSK <<
		IPA_ENDP_INIT_AGGR_N_AGGR_EN_SHFT));
}

static int ipa3_q6_set_ex_path_dis_agg(void)
{
	int ep_idx;
	int client_idx;
	struct ipa3_desc *desc;
	int num_descs = 0;
	int index;
	struct ipa3_register_write *reg_write;
	int retval;

	desc = kcalloc(ipa3_ctx->ipa_num_pipes, sizeof(struct ipa3_desc),
			GFP_KERNEL);
	if (!desc) {
		IPAERR("failed to allocate memory\n");
		return -ENOMEM;
	}

	
	for (client_idx = 0; client_idx < IPA_CLIENT_MAX; client_idx++) {
		ep_idx = ipa3_get_ep_mapping(client_idx);
		if (ep_idx == -1)
			continue;

		if (ipa3_ctx->ep[ep_idx].valid &&
			ipa3_ctx->ep[ep_idx].skip_ep_cfg) {
			BUG_ON(num_descs >= ipa3_ctx->ipa_num_pipes);
			reg_write = kzalloc(sizeof(*reg_write), GFP_KERNEL);

			if (!reg_write) {
				IPAERR("failed to allocate memory\n");
				BUG();
			}
			reg_write->skip_pipeline_clear = 0;
			reg_write->pipeline_clear_options =
				IPA_FULL_PIPELINE_CLEAR;
			reg_write->offset = IPA_ENDP_STATUS_n_OFST(ep_idx);
			reg_write->value =
				(ipa3_get_ep_mapping(IPA_CLIENT_APPS_LAN_CONS) &
				IPA_ENDP_STATUS_n_STATUS_ENDP_BMSK) <<
				IPA_ENDP_STATUS_n_STATUS_ENDP_SHFT;
			reg_write->value_mask =
				IPA_ENDP_STATUS_n_STATUS_ENDP_BMSK <<
				IPA_ENDP_STATUS_n_STATUS_ENDP_SHFT;

			desc[num_descs].opcode = IPA_REGISTER_WRITE;
			desc[num_descs].pyld = reg_write;
			desc[num_descs].len = sizeof(*reg_write);
			desc[num_descs].type = IPA_IMM_CMD_DESC;
			desc[num_descs].callback = ipa3_free_buffer;
			desc[num_descs].user1 = reg_write;
			num_descs++;
		}
	}

	
	for (client_idx = 0; client_idx < IPA_CLIENT_MAX; client_idx++) {
		if (IPA_CLIENT_IS_Q6_CONS(client_idx)) {
			reg_write = kzalloc(sizeof(*reg_write), GFP_KERNEL);

			if (!reg_write) {
				IPAERR("failed to allocate memory\n");
				BUG();
			}

			ipa3_q6_disable_agg_reg(reg_write,
					       ipa3_get_ep_mapping(client_idx));

			desc[num_descs].opcode = IPA_REGISTER_WRITE;
			desc[num_descs].pyld = reg_write;
			desc[num_descs].len = sizeof(*reg_write);
			desc[num_descs].type = IPA_IMM_CMD_DESC;
			desc[num_descs].callback = ipa3_free_buffer;
			desc[num_descs].user1 = reg_write;
			num_descs++;
		}
	}

	
	retval = ipa3_tag_process(desc, num_descs,
				 msecs_to_jiffies(CLEANUP_TAG_PROCESS_TIMEOUT));
	if (retval) {
		IPAERR("TAG process failed! (error %d)\n", retval);
		
		if (retval != -ETIME) {
			for (index = 0; index < num_descs; index++)
				kfree(desc[index].user1);
			retval = -EINVAL;
		}
	}

	kfree(desc);

	return retval;
}

int ipa3_q6_cleanup(void)
{
	if (ipa3_ctx->uc_ctx.uc_zip_error)
		BUG();

	IPA_ACTIVE_CLIENTS_INC_SPECIAL("Q6");

	if (ipa3_q6_pipe_delay()) {
		IPAERR("Failed to delay Q6 pipes\n");
		BUG();
	}
	if (ipa3_q6_avoid_holb()) {
		IPAERR("Failed to set HOLB on Q6 pipes\n");
		BUG();
	}
	if (ipa3_q6_clean_q6_tables()) {
		IPAERR("Failed to clean Q6 tables\n");
		BUG();
	}
	if (ipa3_q6_set_ex_path_dis_agg()) {
		IPAERR("Failed to disable aggregation on Q6 pipes\n");
		BUG();
	}

	ipa3_ctx->q6_proxy_clk_vote_valid = true;

	return 0;
}

int ipa3_q6_pipe_reset(void)
{
	int client_idx;
	int res;

	if (!ipa3_ctx->uc_ctx.uc_loaded) {
		IPAERR("uC is not loaded, won't reset Q6 pipes\n");
		return 0;
	}

	for (client_idx = 0; client_idx < IPA_CLIENT_MAX; client_idx++)
		if (IPA_CLIENT_IS_Q6_CONS(client_idx) ||
			IPA_CLIENT_IS_Q6_PROD(client_idx)) {
			res = ipa3_uc_reset_pipe(client_idx);
			if (res)
				BUG();
		}
	return 0;
}

static inline void ipa3_sram_set_canary(u32 *sram_mmio, int offset)
{
	
	sram_mmio[(offset - 4) / 4] = IPA_MEM_CANARY_VAL;
}

int _ipa_init_sram_v3_0(void)
{
	u32 *ipa_sram_mmio;
	unsigned long phys_addr;

	phys_addr = ipa3_ctx->ipa_wrapper_base +
		ipa3_ctx->ctrl->ipa_reg_base_ofst +
		IPA_SRAM_DIRECT_ACCESS_N_OFST_v3_0(
			ipa3_ctx->smem_restricted_bytes / 4);

	ipa_sram_mmio = ioremap(phys_addr, ipa3_ctx->smem_sz);
	if (!ipa_sram_mmio) {
		IPAERR("fail to ioremap IPA SRAM\n");
		return -ENOMEM;
	}

	
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(v4_flt_hash_ofst) - 4);
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(v4_flt_hash_ofst));
	ipa3_sram_set_canary(ipa_sram_mmio,
		IPA_MEM_PART(v4_flt_nhash_ofst) - 4);
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(v4_flt_nhash_ofst));
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(v6_flt_hash_ofst) - 4);
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(v6_flt_hash_ofst));
	ipa3_sram_set_canary(ipa_sram_mmio,
		IPA_MEM_PART(v6_flt_nhash_ofst) - 4);
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(v6_flt_nhash_ofst));
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(v4_rt_hash_ofst) - 4);
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(v4_rt_hash_ofst));
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(v4_rt_nhash_ofst) - 4);
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(v4_rt_nhash_ofst));
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(v6_rt_hash_ofst) - 4);
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(v6_rt_hash_ofst));
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(v6_rt_nhash_ofst) - 4);
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(v6_rt_nhash_ofst));
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(modem_hdr_ofst) - 4);
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(modem_hdr_ofst));
	ipa3_sram_set_canary(ipa_sram_mmio,
		IPA_MEM_PART(modem_hdr_proc_ctx_ofst) - 4);
	ipa3_sram_set_canary(ipa_sram_mmio,
		IPA_MEM_PART(modem_hdr_proc_ctx_ofst));
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(modem_ofst) - 4);
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(modem_ofst));
	ipa3_sram_set_canary(ipa_sram_mmio, IPA_MEM_PART(end_ofst));

	iounmap(ipa_sram_mmio);

	return 0;
}

int _ipa_init_hdr_v3_0(void)
{
	struct ipa3_desc desc = { 0 };
	struct ipa3_mem_buffer mem;
	struct ipa3_hdr_init_local cmd = { 0 };
	struct ipa3_hw_imm_cmd_dma_shared_mem dma_cmd = { 0 };

	mem.size = IPA_MEM_PART(modem_hdr_size) + IPA_MEM_PART(apps_hdr_size);
	mem.base = dma_alloc_coherent(ipa3_ctx->pdev, mem.size, &mem.phys_base,
		GFP_KERNEL);
	if (!mem.base) {
		IPAERR("fail to alloc DMA buff of size %d\n", mem.size);
		return -ENOMEM;
	}
	memset(mem.base, 0, mem.size);

	cmd.hdr_table_src_addr = mem.phys_base;
	cmd.size_hdr_table = mem.size;
	cmd.hdr_table_dst_addr = ipa3_ctx->smem_restricted_bytes +
		IPA_MEM_PART(modem_hdr_ofst);

	desc.opcode = IPA_HDR_INIT_LOCAL;
	desc.pyld = &cmd;
	desc.len = sizeof(struct ipa3_hdr_init_local);
	desc.type = IPA_IMM_CMD_DESC;
	IPA_DUMP_BUFF(mem.base, mem.phys_base, mem.size);

	if (ipa3_send_cmd(1, &desc)) {
		IPAERR("fail to send immediate command\n");
		dma_free_coherent(ipa3_ctx->pdev,
			mem.size, mem.base,
			mem.phys_base);
		return -EFAULT;
	}

	dma_free_coherent(ipa3_ctx->pdev, mem.size, mem.base, mem.phys_base);

	mem.size = IPA_MEM_PART(modem_hdr_proc_ctx_size) +
		IPA_MEM_PART(apps_hdr_proc_ctx_size);
	mem.base = dma_alloc_coherent(ipa3_ctx->pdev, mem.size, &mem.phys_base,
		GFP_KERNEL);
	if (!mem.base) {
		IPAERR("fail to alloc DMA buff of size %d\n", mem.size);
		return -ENOMEM;
	}
	memset(mem.base, 0, mem.size);
	memset(&desc, 0, sizeof(desc));

	dma_cmd.skip_pipeline_clear = 0;
	dma_cmd.pipeline_clear_options = IPA_FULL_PIPELINE_CLEAR;
	dma_cmd.system_addr = mem.phys_base;
	dma_cmd.local_addr = ipa3_ctx->smem_restricted_bytes +
		IPA_MEM_PART(modem_hdr_proc_ctx_ofst);
	dma_cmd.size = mem.size;
	desc.opcode = IPA_DMA_SHARED_MEM;
	desc.pyld = &dma_cmd;
	desc.len = sizeof(struct ipa3_hw_imm_cmd_dma_shared_mem);
	desc.type = IPA_IMM_CMD_DESC;
	IPA_DUMP_BUFF(mem.base, mem.phys_base, mem.size);

	if (ipa3_send_cmd(1, &desc)) {
		IPAERR("fail to send immediate command\n");
		dma_free_coherent(ipa3_ctx->pdev,
			mem.size,
			mem.base,
			mem.phys_base);
		return -EFAULT;
	}

	ipa_write_reg(ipa3_ctx->mmio,
		IPA_LOCAL_PKT_PROC_CNTXT_BASE_OFST,
		dma_cmd.local_addr);

	dma_free_coherent(ipa3_ctx->pdev, mem.size, mem.base, mem.phys_base);

	return 0;
}

int _ipa_init_rt4_v3(void)
{
	struct ipa3_desc desc = { 0 };
	struct ipa3_mem_buffer mem;
	struct ipa3_ip_v4_routing_init v4_cmd;
	u64 *entry;
	int i;
	int rc = 0;

	for (i = IPA_MEM_PART(v4_modem_rt_index_lo);
		i <= IPA_MEM_PART(v4_modem_rt_index_hi);
		i++)
		ipa3_ctx->rt_idx_bitmap[IPA_IP_v4] |= (1 << i);
	IPADBG("v4 rt bitmap 0x%lx\n", ipa3_ctx->rt_idx_bitmap[IPA_IP_v4]);

	mem.size = IPA_MEM_PART(v4_rt_nhash_size);
	mem.base = dma_alloc_coherent(ipa3_ctx->pdev, mem.size, &mem.phys_base,
			GFP_KERNEL);
	if (!mem.base) {
		IPAERR("fail to alloc DMA buff of size %d\n", mem.size);
		return -ENOMEM;
	}

	entry = mem.base;
	for (i = 0; i < IPA_MEM_PART(v4_rt_num_index); i++) {
		*entry = ipa3_ctx->empty_rt_tbl_mem.phys_base;
		entry++;
	}

	desc.opcode = IPA_IP_V4_ROUTING_INIT;
	v4_cmd.hash_rules_addr = mem.phys_base;
	v4_cmd.hash_rules_size = mem.size;
	v4_cmd.hash_local_addr = ipa3_ctx->smem_restricted_bytes +
		IPA_MEM_PART(v4_rt_hash_ofst);
	v4_cmd.nhash_rules_addr = mem.phys_base;
	v4_cmd.nhash_rules_size = mem.size;
	v4_cmd.nhash_local_addr = ipa3_ctx->smem_restricted_bytes +
		IPA_MEM_PART(v4_rt_nhash_ofst);
	IPADBG("putting hashable routing IPv4 rules to phys 0x%x\n",
				v4_cmd.hash_local_addr);
	IPADBG("putting non-hashable routing IPv4 rules to phys 0x%x\n",
				v4_cmd.nhash_local_addr);

	desc.pyld = &v4_cmd;
	desc.len = sizeof(struct ipa3_ip_v4_routing_init);
	desc.type = IPA_IMM_CMD_DESC;
	IPA_DUMP_BUFF(mem.base, mem.phys_base, mem.size);

	if (ipa3_send_cmd(1, &desc)) {
		IPAERR("fail to send immediate command\n");
		rc = -EFAULT;
	}

	dma_free_coherent(ipa3_ctx->pdev, mem.size, mem.base, mem.phys_base);
	return rc;
}

int _ipa_init_rt6_v3(void)
{
	struct ipa3_desc desc = { 0 };
	struct ipa3_mem_buffer mem;
	struct ipa3_ip_v6_routing_init v6_cmd;
	u64 *entry;
	int i;
	int rc = 0;

	for (i = IPA_MEM_PART(v6_modem_rt_index_lo);
		i <= IPA_MEM_PART(v6_modem_rt_index_hi);
		i++)
		ipa3_ctx->rt_idx_bitmap[IPA_IP_v6] |= (1 << i);
	IPADBG("v6 rt bitmap 0x%lx\n", ipa3_ctx->rt_idx_bitmap[IPA_IP_v6]);

	mem.size = IPA_MEM_PART(v6_rt_nhash_size);
	mem.base = dma_alloc_coherent(ipa3_ctx->pdev, mem.size, &mem.phys_base,
			GFP_KERNEL);
	if (!mem.base) {
		IPAERR("fail to alloc DMA buff of size %d\n", mem.size);
		return -ENOMEM;
	}

	entry = mem.base;
	for (i = 0; i < IPA_MEM_PART(v6_rt_num_index); i++) {
		*entry = ipa3_ctx->empty_rt_tbl_mem.phys_base;
		entry++;
	}

	desc.opcode = IPA_IP_V6_ROUTING_INIT;
	v6_cmd.hash_rules_addr = mem.phys_base;
	v6_cmd.hash_rules_size = mem.size;
	v6_cmd.hash_local_addr = ipa3_ctx->smem_restricted_bytes +
		IPA_MEM_PART(v6_rt_hash_ofst);
	v6_cmd.nhash_rules_addr = mem.phys_base;
	v6_cmd.nhash_rules_size = mem.size;
	v6_cmd.nhash_local_addr = ipa3_ctx->smem_restricted_bytes +
		IPA_MEM_PART(v6_rt_nhash_ofst);
	IPADBG("putting hashable routing IPv6 rules to phys 0x%x\n",
				v6_cmd.hash_local_addr);
	IPADBG("putting non-hashable routing IPv6 rules to phys 0x%x\n",
				v6_cmd.nhash_local_addr);

	desc.pyld = &v6_cmd;
	desc.len = sizeof(struct ipa3_ip_v6_routing_init);
	desc.type = IPA_IMM_CMD_DESC;
	IPA_DUMP_BUFF(mem.base, mem.phys_base, mem.size);

	if (ipa3_send_cmd(1, &desc)) {
		IPAERR("fail to send immediate command\n");
		rc = -EFAULT;
	}

	dma_free_coherent(ipa3_ctx->pdev, mem.size, mem.base, mem.phys_base);
	return rc;
}

int _ipa_init_flt4_v3(void)
{
	struct ipa3_desc desc = { 0 };
	struct ipa3_mem_buffer mem;
	struct ipa3_ip_v4_filter_init v4_cmd;
	u64 *entry;
	int i;
	int rc = 0;
	int flt_spc;

	flt_spc = IPA_MEM_PART(v4_flt_hash_size);
	
	flt_spc -= IPA_HW_TBL_HDR_WIDTH;
	flt_spc /= IPA_HW_TBL_HDR_WIDTH;
	if (ipa3_ctx->ep_flt_num > flt_spc) {
		IPAERR("space for v4 hash flt hdr is too small\n");
		WARN_ON(1);
		return -EPERM;
	}
	flt_spc = IPA_MEM_PART(v4_flt_nhash_size);
	
	flt_spc -= IPA_HW_TBL_HDR_WIDTH;
	flt_spc /= IPA_HW_TBL_HDR_WIDTH;
	if (ipa3_ctx->ep_flt_num > flt_spc) {
		IPAERR("space for v4 non-hash flt hdr is too small\n");
		WARN_ON(1);
		return -EPERM;
	}

	
	mem.size = (ipa3_ctx->ep_flt_num + 1) * IPA_HW_TBL_HDR_WIDTH;
	mem.base = dma_alloc_coherent(ipa3_ctx->pdev, mem.size, &mem.phys_base,
			GFP_KERNEL);
	if (!mem.base) {
		IPAERR("fail to alloc DMA buff of size %d\n", mem.size);
		return -ENOMEM;
	}

	entry = mem.base;

	*entry = ((u64)ipa3_ctx->ep_flt_bitmap) << 1;
	IPADBG("v4 flt bitmap 0x%llx\n", *entry);
	entry++;

	for (i = 0; i <= ipa3_ctx->ep_flt_num; i++) {
		*entry = ipa3_ctx->empty_rt_tbl_mem.phys_base;
		entry++;
	}

	desc.opcode = IPA_IP_V4_FILTER_INIT;
	v4_cmd.hash_rules_addr = mem.phys_base;
	v4_cmd.hash_rules_size = mem.size;
	v4_cmd.hash_local_addr = ipa3_ctx->smem_restricted_bytes +
		IPA_MEM_PART(v4_flt_hash_ofst);
	v4_cmd.nhash_rules_addr = mem.phys_base;
	v4_cmd.nhash_rules_size = mem.size;
	v4_cmd.nhash_local_addr = ipa3_ctx->smem_restricted_bytes +
		IPA_MEM_PART(v4_flt_nhash_ofst);
	IPADBG("putting hashable filtering IPv4 rules to phys 0x%x\n",
				v4_cmd.hash_local_addr);
	IPADBG("putting non-hashable filtering IPv4 rules to phys 0x%x\n",
				v4_cmd.nhash_local_addr);

	desc.pyld = &v4_cmd;
	desc.len = sizeof(struct ipa3_ip_v4_filter_init);
	desc.type = IPA_IMM_CMD_DESC;
	IPA_DUMP_BUFF(mem.base, mem.phys_base, mem.size);

	if (ipa3_send_cmd(1, &desc)) {
		IPAERR("fail to send immediate command\n");
		rc = -EFAULT;
	}

	dma_free_coherent(ipa3_ctx->pdev, mem.size, mem.base, mem.phys_base);
	return rc;
}

int _ipa_init_flt6_v3(void)
{
	struct ipa3_desc desc = { 0 };
	struct ipa3_mem_buffer mem;
	struct ipa3_ip_v6_filter_init v6_cmd;
	u64 *entry;
	int i;
	int rc = 0;
	int flt_spc;

	flt_spc = IPA_MEM_PART(v6_flt_hash_size);
	
	flt_spc -= IPA_HW_TBL_HDR_WIDTH;
	flt_spc /= IPA_HW_TBL_HDR_WIDTH;
	if (ipa3_ctx->ep_flt_num > flt_spc) {
		IPAERR("space for v6 hash flt hdr is too small\n");
		WARN_ON(1);
		return -EPERM;
	}
	flt_spc = IPA_MEM_PART(v6_flt_nhash_size);
	
	flt_spc -= IPA_HW_TBL_HDR_WIDTH;
	flt_spc /= IPA_HW_TBL_HDR_WIDTH;
	if (ipa3_ctx->ep_flt_num > flt_spc) {
		IPAERR("space for v6 non-hash flt hdr is too small\n");
		WARN_ON(1);
		return -EPERM;
	}

	
	mem.size = (ipa3_ctx->ep_flt_num + 1) * IPA_HW_TBL_HDR_WIDTH;
	mem.base = dma_alloc_coherent(ipa3_ctx->pdev, mem.size, &mem.phys_base,
			GFP_KERNEL);
	if (!mem.base) {
		IPAERR("fail to alloc DMA buff of size %d\n", mem.size);
		return -ENOMEM;
	}

	entry = mem.base;

	*entry = ((u64)ipa3_ctx->ep_flt_bitmap) << 1;
	IPADBG("v6 flt bitmap 0x%llx\n", *entry);
	entry++;

	for (i = 0; i <= ipa3_ctx->ep_flt_num; i++) {
		*entry = ipa3_ctx->empty_rt_tbl_mem.phys_base;
		entry++;
	}

	desc.opcode = IPA_IP_V6_FILTER_INIT;
	v6_cmd.hash_rules_addr = mem.phys_base;
	v6_cmd.hash_rules_size = mem.size;
	v6_cmd.hash_local_addr = ipa3_ctx->smem_restricted_bytes +
		IPA_MEM_PART(v6_flt_hash_ofst);
	v6_cmd.nhash_rules_addr = mem.phys_base;
	v6_cmd.nhash_rules_size = mem.size;
	v6_cmd.nhash_local_addr = ipa3_ctx->smem_restricted_bytes +
		IPA_MEM_PART(v6_flt_nhash_ofst);
	IPADBG("putting hashable filtering IPv6 rules to phys 0x%x\n",
				v6_cmd.hash_local_addr);
	IPADBG("putting non-hashable filtering IPv6 rules to phys 0x%x\n",
				v6_cmd.nhash_local_addr);

	desc.pyld = &v6_cmd;
	desc.len = sizeof(struct ipa3_ip_v6_filter_init);
	desc.type = IPA_IMM_CMD_DESC;
	IPA_DUMP_BUFF(mem.base, mem.phys_base, mem.size);

	if (ipa3_send_cmd(1, &desc)) {
		IPAERR("fail to send immediate command\n");
		rc = -EFAULT;
	}

	dma_free_coherent(ipa3_ctx->pdev, mem.size, mem.base, mem.phys_base);
	return rc;
}

static int ipa3_setup_flt_hash_tuple(void)
{
	int pipe_idx;
	struct ipa3_hash_tuple tuple;

	memset(&tuple, 0, sizeof(struct ipa3_hash_tuple));

	for (pipe_idx = 0; pipe_idx < ipa3_ctx->ipa_num_pipes ; pipe_idx++) {
		if (!ipa_is_ep_support_flt(pipe_idx))
			continue;

		if (ipa_is_modem_pipe(pipe_idx))
			continue;

		if (ipa3_set_flt_tuple_mask(pipe_idx, &tuple)) {
			IPAERR("failed to setup pipe %d flt tuple\n", pipe_idx);
			return -EFAULT;
		}
	}

	return 0;
}

static int ipa3_setup_rt_hash_tuple(void)
{
	int tbl_idx;
	struct ipa3_hash_tuple tuple;

	memset(&tuple, 0, sizeof(struct ipa3_hash_tuple));

	for (tbl_idx = 0;
		tbl_idx < max(IPA_MEM_PART(v6_rt_num_index),
		IPA_MEM_PART(v4_rt_num_index));
		tbl_idx++) {

		if (tbl_idx >= IPA_MEM_PART(v4_modem_rt_index_lo) &&
			tbl_idx <= IPA_MEM_PART(v4_modem_rt_index_hi))
			continue;

		if (tbl_idx >= IPA_MEM_PART(v6_modem_rt_index_lo) &&
			tbl_idx <= IPA_MEM_PART(v6_modem_rt_index_hi))
			continue;

		if (ipa3_set_rt_tuple_mask(tbl_idx, &tuple)) {
			IPAERR("failed to setup tbl %d rt tuple\n", tbl_idx);
			return -EFAULT;
		}
	}

	return 0;
}

static int ipa3_setup_apps_pipes(void)
{
	struct ipa_sys_connect_params sys_in;
	int result = 0;

	
	memset(&sys_in, 0, sizeof(struct ipa_sys_connect_params));
	sys_in.client = IPA_CLIENT_APPS_CMD_PROD;
	sys_in.desc_fifo_sz = IPA_SYS_DESC_FIFO_SZ;
	sys_in.ipa_ep_cfg.mode.mode = IPA_DMA;
	sys_in.ipa_ep_cfg.mode.dst = IPA_CLIENT_APPS_LAN_CONS;
	if (ipa3_setup_sys_pipe(&sys_in, &ipa3_ctx->clnt_hdl_cmd)) {
		IPAERR(":setup sys pipe failed.\n");
		result = -EPERM;
		goto fail_cmd;
	}
	IPADBG("Apps to IPA cmd pipe is connected\n");

	ipa3_ctx->ctrl->ipa_init_sram();
	IPADBG("SRAM initialized\n");

	ipa3_ctx->ctrl->ipa_init_hdr();
	IPADBG("HDR initialized\n");

	ipa3_ctx->ctrl->ipa_init_rt4();
	IPADBG("V4 RT initialized\n");

	ipa3_ctx->ctrl->ipa_init_rt6();
	IPADBG("V6 RT initialized\n");

	ipa3_ctx->ctrl->ipa_init_flt4();
	IPADBG("V4 FLT initialized\n");

	ipa3_ctx->ctrl->ipa_init_flt6();
	IPADBG("V6 FLT initialized\n");

	if (ipa3_setup_flt_hash_tuple()) {
		IPAERR(":fail to configure flt hash tuple\n");
		result = -EPERM;
		goto fail_schedule_delayed_work;
	}
	IPADBG("flt hash tuple is configured\n");

	if (ipa3_setup_rt_hash_tuple()) {
		IPAERR(":fail to configure rt hash tuple\n");
		result = -EPERM;
		goto fail_schedule_delayed_work;
	}
	IPADBG("rt hash tuple is configured\n");

	if (ipa3_setup_exception_path()) {
		IPAERR(":fail to setup excp path\n");
		result = -EPERM;
		goto fail_schedule_delayed_work;
	}
	IPADBG("Exception path was successfully set");

	if (ipa3_setup_dflt_rt_tables()) {
		IPAERR(":fail to setup dflt routes\n");
		result = -EPERM;
		goto fail_schedule_delayed_work;
	}
	IPADBG("default routing was set\n");

	
	memset(&sys_in, 0, sizeof(struct ipa_sys_connect_params));
	sys_in.client = IPA_CLIENT_APPS_LAN_CONS;
	sys_in.desc_fifo_sz = IPA_SYS_DESC_FIFO_SZ;
	sys_in.notify = ipa3_lan_rx_cb;
	sys_in.priv = NULL;
	sys_in.ipa_ep_cfg.hdr.hdr_len = IPA_LAN_RX_HEADER_LENGTH;
	sys_in.ipa_ep_cfg.hdr_ext.hdr_little_endian = false;
	sys_in.ipa_ep_cfg.hdr_ext.hdr_total_len_or_pad_valid = true;
	sys_in.ipa_ep_cfg.hdr_ext.hdr_total_len_or_pad = IPA_HDR_PAD;
	sys_in.ipa_ep_cfg.hdr_ext.hdr_payload_len_inc_padding = false;
	sys_in.ipa_ep_cfg.hdr_ext.hdr_total_len_or_pad_offset = 0;
	sys_in.ipa_ep_cfg.hdr_ext.hdr_pad_to_alignment = 2;
	sys_in.ipa_ep_cfg.cfg.cs_offload_en = IPA_ENABLE_CS_OFFLOAD_DL;

	spin_lock_init(&ipa3_ctx->disconnect_lock);
	if (ipa3_setup_sys_pipe(&sys_in, &ipa3_ctx->clnt_hdl_data_in)) {
		IPAERR(":setup sys pipe failed.\n");
		result = -EPERM;
		goto fail_schedule_delayed_work;
	}

	
	memset(&sys_in, 0, sizeof(struct ipa_sys_connect_params));
	sys_in.client = IPA_CLIENT_APPS_LAN_WAN_PROD;
	sys_in.desc_fifo_sz = IPA_SYS_TX_DATA_DESC_FIFO_SZ;
	sys_in.ipa_ep_cfg.mode.mode = IPA_BASIC;
	if (ipa3_setup_sys_pipe(&sys_in, &ipa3_ctx->clnt_hdl_data_out)) {
		IPAERR(":setup sys pipe failed.\n");
		result = -EPERM;
		goto fail_data_out;
	}

	return 0;

fail_data_out:
	ipa3_teardown_sys_pipe(ipa3_ctx->clnt_hdl_data_in);
fail_schedule_delayed_work:
	if (ipa3_ctx->dflt_v6_rt_rule_hdl)
		__ipa3_del_rt_rule(ipa3_ctx->dflt_v6_rt_rule_hdl);
	if (ipa3_ctx->dflt_v4_rt_rule_hdl)
		__ipa3_del_rt_rule(ipa3_ctx->dflt_v4_rt_rule_hdl);
	if (ipa3_ctx->excp_hdr_hdl)
		__ipa3_del_hdr(ipa3_ctx->excp_hdr_hdl);
	ipa3_teardown_sys_pipe(ipa3_ctx->clnt_hdl_cmd);
fail_cmd:
	return result;
}

static void ipa3_teardown_apps_pipes(void)
{
	ipa3_teardown_sys_pipe(ipa3_ctx->clnt_hdl_data_out);
	ipa3_teardown_sys_pipe(ipa3_ctx->clnt_hdl_data_in);
	__ipa3_del_rt_rule(ipa3_ctx->dflt_v6_rt_rule_hdl);
	__ipa3_del_rt_rule(ipa3_ctx->dflt_v4_rt_rule_hdl);
	__ipa3_del_hdr(ipa3_ctx->excp_hdr_hdl);
	ipa3_teardown_sys_pipe(ipa3_ctx->clnt_hdl_cmd);
}

#ifdef CONFIG_COMPAT
long compat_ipa_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	struct ipa3_ioc_nat_alloc_mem32 nat_mem32;
	struct ipa_ioc_nat_alloc_mem nat_mem;

	switch (cmd) {
	case IPA_IOC_ADD_HDR32:
		cmd = IPA_IOC_ADD_HDR;
		break;
	case IPA_IOC_DEL_HDR32:
		cmd = IPA_IOC_DEL_HDR;
		break;
	case IPA_IOC_ADD_RT_RULE32:
		cmd = IPA_IOC_ADD_RT_RULE;
		break;
	case IPA_IOC_DEL_RT_RULE32:
		cmd = IPA_IOC_DEL_RT_RULE;
		break;
	case IPA_IOC_ADD_FLT_RULE32:
		cmd = IPA_IOC_ADD_FLT_RULE;
		break;
	case IPA_IOC_DEL_FLT_RULE32:
		cmd = IPA_IOC_DEL_FLT_RULE;
		break;
	case IPA_IOC_GET_RT_TBL32:
		cmd = IPA_IOC_GET_RT_TBL;
		break;
	case IPA_IOC_COPY_HDR32:
		cmd = IPA_IOC_COPY_HDR;
		break;
	case IPA_IOC_QUERY_INTF32:
		cmd = IPA_IOC_QUERY_INTF;
		break;
	case IPA_IOC_QUERY_INTF_TX_PROPS32:
		cmd = IPA_IOC_QUERY_INTF_TX_PROPS;
		break;
	case IPA_IOC_QUERY_INTF_RX_PROPS32:
		cmd = IPA_IOC_QUERY_INTF_RX_PROPS;
		break;
	case IPA_IOC_QUERY_INTF_EXT_PROPS32:
		cmd = IPA_IOC_QUERY_INTF_EXT_PROPS;
		break;
	case IPA_IOC_GET_HDR32:
		cmd = IPA_IOC_GET_HDR;
		break;
	case IPA_IOC_ALLOC_NAT_MEM32:
		if (copy_from_user((u8 *)&nat_mem32, (u8 *)arg,
			sizeof(struct ipa3_ioc_nat_alloc_mem32))) {
			retval = -EFAULT;
			goto ret;
		}
		memcpy(nat_mem.dev_name, nat_mem32.dev_name,
				IPA_RESOURCE_NAME_MAX);
		nat_mem.size = (size_t)nat_mem32.size;
		nat_mem.offset = (off_t)nat_mem32.offset;

		
		nat_mem.dev_name[IPA_RESOURCE_NAME_MAX - 1] = '\0';

		if (ipa3_allocate_nat_device(&nat_mem)) {
			retval = -EFAULT;
			goto ret;
		}
		nat_mem32.offset = (compat_off_t)nat_mem.offset;
		if (copy_to_user((u8 *)arg, (u8 *)&nat_mem32,
			sizeof(struct ipa3_ioc_nat_alloc_mem32))) {
			retval = -EFAULT;
		}
ret:
		return retval;
	case IPA_IOC_V4_INIT_NAT32:
		cmd = IPA_IOC_V4_INIT_NAT;
		break;
	case IPA_IOC_NAT_DMA32:
		cmd = IPA_IOC_NAT_DMA;
		break;
	case IPA_IOC_V4_DEL_NAT32:
		cmd = IPA_IOC_V4_DEL_NAT;
		break;
	case IPA_IOC_GET_NAT_OFFSET32:
		cmd = IPA_IOC_GET_NAT_OFFSET;
		break;
	case IPA_IOC_PULL_MSG32:
		cmd = IPA_IOC_PULL_MSG;
		break;
	case IPA_IOC_RM_ADD_DEPENDENCY32:
		cmd = IPA_IOC_RM_ADD_DEPENDENCY;
		break;
	case IPA_IOC_RM_DEL_DEPENDENCY32:
		cmd = IPA_IOC_RM_DEL_DEPENDENCY;
		break;
	case IPA_IOC_GENERATE_FLT_EQ32:
		cmd = IPA_IOC_GENERATE_FLT_EQ;
		break;
	case IPA_IOC_QUERY_RT_TBL_INDEX32:
		cmd = IPA_IOC_QUERY_RT_TBL_INDEX;
		break;
	case IPA_IOC_WRITE_QMAPID32:
		cmd = IPA_IOC_WRITE_QMAPID;
		break;
	case IPA_IOC_MDFY_FLT_RULE32:
		cmd = IPA_IOC_MDFY_FLT_RULE;
		break;
	case IPA_IOC_NOTIFY_WAN_UPSTREAM_ROUTE_ADD32:
		cmd = IPA_IOC_NOTIFY_WAN_UPSTREAM_ROUTE_ADD;
		break;
	case IPA_IOC_NOTIFY_WAN_UPSTREAM_ROUTE_DEL32:
		cmd = IPA_IOC_NOTIFY_WAN_UPSTREAM_ROUTE_DEL;
		break;
	case IPA_IOC_NOTIFY_WAN_EMBMS_CONNECTED32:
		cmd = IPA_IOC_NOTIFY_WAN_EMBMS_CONNECTED;
		break;
	case IPA_IOC_MDFY_RT_RULE32:
		cmd = IPA_IOC_MDFY_RT_RULE;
		break;
	case IPA_IOC_COMMIT_HDR:
	case IPA_IOC_RESET_HDR:
	case IPA_IOC_COMMIT_RT:
	case IPA_IOC_RESET_RT:
	case IPA_IOC_COMMIT_FLT:
	case IPA_IOC_RESET_FLT:
	case IPA_IOC_DUMP:
	case IPA_IOC_PUT_RT_TBL:
	case IPA_IOC_PUT_HDR:
	case IPA_IOC_SET_FLT:
	case IPA_IOC_QUERY_EP_MAPPING:
		break;
	default:
		return -ENOIOCTLCMD;
	}
	return ipa3_ioctl(file, cmd, (unsigned long) compat_ptr(arg));
}
#endif

static ssize_t ipa3_write(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos);

static const struct file_operations ipa3_drv_fops = {
	.owner = THIS_MODULE,
	.open = ipa3_open,
	.read = ipa3_read,
	.write = ipa3_write,
	.unlocked_ioctl = ipa3_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = compat_ipa_ioctl,
#endif
};

static int ipa3_get_clks(struct device *dev)
{
	ipa3_clk = clk_get(dev, "core_clk");
	if (IS_ERR(ipa3_clk)) {
		if (ipa3_clk != ERR_PTR(-EPROBE_DEFER))
			IPAERR("fail to get ipa clk\n");
		return PTR_ERR(ipa3_clk);
	}

	if (smmu_present && arm_smmu) {
		smmu_clk = clk_get(dev, "smmu_clk");
		if (IS_ERR(smmu_clk)) {
			if (smmu_clk != ERR_PTR(-EPROBE_DEFER))
				IPAERR("fail to get smmu clk\n");
			return PTR_ERR(smmu_clk);
		}

		if (clk_get_rate(smmu_clk) == 0) {
			long rate = clk_round_rate(smmu_clk, 1000);

			clk_set_rate(smmu_clk, rate);
		}
	}

	return 0;
}

void _ipa_enable_clks_v3_0(void)
{
	IPADBG("enabling gcc_ipa_clk\n");
	if (ipa3_clk) {
		clk_prepare(ipa3_clk);
		clk_enable(ipa3_clk);
		IPADBG("curr_ipa_clk_rate=%d", ipa3_ctx->curr_ipa_clk_rate);
		clk_set_rate(ipa3_clk, ipa3_ctx->curr_ipa_clk_rate);
		ipa3_uc_notify_clk_state(true);
	} else {
		WARN_ON(1);
	}

	if (smmu_clk)
		clk_prepare_enable(smmu_clk);
	ipa3_suspend_apps_pipes(false);
}

static unsigned int ipa3_get_bus_vote(void)
{
	unsigned int idx = 1;

	if (ipa3_ctx->curr_ipa_clk_rate == ipa3_ctx->ctrl->ipa_clk_rate_svs) {
		idx = 1;
	} else if (ipa3_ctx->curr_ipa_clk_rate ==
			ipa3_ctx->ctrl->ipa_clk_rate_nominal) {
		if (ipa3_ctx->ctrl->msm_bus_data_ptr->num_usecases <= 2)
			idx = 1;
		else
			idx = 2;
	} else if (ipa3_ctx->curr_ipa_clk_rate ==
			ipa3_ctx->ctrl->ipa_clk_rate_turbo) {
		idx = ipa3_ctx->ctrl->msm_bus_data_ptr->num_usecases - 1;
	} else {
		WARN_ON(1);
	}

	IPADBG("curr %d idx %d\n", ipa3_ctx->curr_ipa_clk_rate, idx);

	return idx;
}

void ipa3_enable_clks(void)
{
	IPADBG("enabling IPA clocks and bus voting\n");

	ipa3_ctx->ctrl->ipa3_enable_clks();

	if (ipa3_ctx->ipa3_hw_mode != IPA_HW_MODE_VIRTUAL)
		if (msm_bus_scale_client_update_request(ipa3_ctx->ipa_bus_hdl,
		    ipa3_get_bus_vote()))
			WARN_ON(1);
}


void _ipa_disable_clks_v3_0(void)
{
	IPADBG("disabling gcc_ipa_clk\n");
	ipa3_suspend_apps_pipes(true);
	ipa3_uc_notify_clk_state(false);
	if (ipa3_clk)
		clk_disable_unprepare(ipa3_clk);
	else
		WARN_ON(1);

	if (smmu_clk)
		clk_disable_unprepare(smmu_clk);
}

void ipa3_disable_clks(void)
{
	IPADBG("disabling IPA clocks and bus voting\n");

	ipa3_ctx->ctrl->ipa3_disable_clks();

	if (ipa3_ctx->ipa3_hw_mode != IPA_HW_MODE_VIRTUAL)
		if (msm_bus_scale_client_update_request(ipa3_ctx->ipa_bus_hdl,
		    0))
			WARN_ON(1);
}

static void ipa3_start_tag_process(struct work_struct *work)
{
	int res;

	IPADBG("starting TAG process\n");
	
	res = ipa3_tag_aggr_force_close(-1);
	if (res)
		IPAERR("ipa3_tag_aggr_force_close failed %d\n", res);
	IPA_ACTIVE_CLIENTS_DEC_SPECIAL("TAG_PROCESS");

	IPADBG("TAG process done\n");
}

void ipa3_active_clients_log_mod(struct ipa3_active_client_logging_info *id,
		bool inc, bool int_ctx)
{
	char temp_str[IPA3_ACTIVE_CLIENTS_LOG_LINE_LEN];
	unsigned long long t;
	unsigned long nanosec_rem;
	struct ipa3_active_client_htable_entry *hentry;
	struct ipa3_active_client_htable_entry *hfound;
	u32 hkey;
	char str_to_hash[IPA3_ACTIVE_CLIENTS_LOG_NAME_LEN];

	hfound = NULL;
	memset(str_to_hash, 0, IPA3_ACTIVE_CLIENTS_LOG_NAME_LEN);
	strlcpy(str_to_hash, id->id_string, IPA3_ACTIVE_CLIENTS_LOG_NAME_LEN);
	hkey = arch_fast_hash(str_to_hash, IPA3_ACTIVE_CLIENTS_LOG_NAME_LEN,
			0);
	hash_for_each_possible(ipa3_ctx->ipa3_active_clients_logging.htable,
			hentry, list, hkey) {
		if (!strcmp(hentry->id_string, id->id_string)) {
			hentry->count = hentry->count + (inc ? 1 : -1);
			hfound = hentry;
		}
	}
	if (hfound == NULL) {
		hentry = NULL;
		hentry = kzalloc(sizeof(
				struct ipa3_active_client_htable_entry),
				int_ctx ? GFP_ATOMIC : GFP_KERNEL);
		if (hentry == NULL) {
			IPAERR("failed allocating active clients hash entry");
			return;
		}
		hentry->type = id->type;
		strlcpy(hentry->id_string, id->id_string,
				IPA3_ACTIVE_CLIENTS_LOG_NAME_LEN);
		INIT_HLIST_NODE(&hentry->list);
		hentry->count = inc ? 1 : -1;
		hash_add(ipa3_ctx->ipa3_active_clients_logging.htable,
				&hentry->list, hkey);
	} else if (hfound->count == 0) {
		hash_del(&hfound->list);
		kfree(hfound);
	}

	if (id->type != SIMPLE) {
		t = local_clock();
		nanosec_rem = do_div(t, 1000000000) / 1000;
		snprintf(temp_str, IPA3_ACTIVE_CLIENTS_LOG_LINE_LEN,
				inc ? "[%5lu.%06lu] ^ %s, %s: %d" :
						"[%5lu.%06lu] v %s, %s: %d",
				(unsigned long)t, nanosec_rem,
				id->id_string, id->file, id->line);
		ipa3_active_clients_log_insert(temp_str);
	}
}

void ipa3_active_clients_log_dec(struct ipa3_active_client_logging_info *id,
		bool int_ctx)
{
	ipa3_active_clients_log_mod(id, false, int_ctx);
}

void ipa3_active_clients_log_inc(struct ipa3_active_client_logging_info *id,
		bool int_ctx)
{
	ipa3_active_clients_log_mod(id, true, int_ctx);
}

void ipa3_inc_client_enable_clks(struct ipa3_active_client_logging_info *id)
{
	ipa3_active_clients_lock();
	ipa3_active_clients_log_inc(id, false);
	ipa3_ctx->ipa3_active_clients.cnt++;
	if (ipa3_ctx->ipa3_active_clients.cnt == 1)
		ipa3_enable_clks();
	IPADBG("active clients = %d\n", ipa3_ctx->ipa3_active_clients.cnt);
	ipa3_active_clients_unlock();
}

int ipa3_inc_client_enable_clks_no_block(struct ipa3_active_client_logging_info
		*id)
{
	int res = 0;
	unsigned long flags;

	if (ipa3_active_clients_trylock(&flags) == 0)
		return -EPERM;

	if (ipa3_ctx->ipa3_active_clients.cnt == 0) {
		res = -EPERM;
		goto bail;
	}
	ipa3_active_clients_log_inc(id, true);
	ipa3_ctx->ipa3_active_clients.cnt++;
	IPADBG("active clients = %d\n", ipa3_ctx->ipa3_active_clients.cnt);
bail:
	ipa3_active_clients_trylock_unlock(&flags);

	return res;
}

void ipa3_dec_client_disable_clks(struct ipa3_active_client_logging_info *id)
{
	struct ipa3_active_client_logging_info log_info;

	ipa3_active_clients_lock();
	ipa3_active_clients_log_dec(id, false);
	ipa3_ctx->ipa3_active_clients.cnt--;
	IPADBG("active clients = %d\n", ipa3_ctx->ipa3_active_clients.cnt);
	if (ipa3_ctx->ipa3_active_clients.cnt == 0) {
		if (ipa3_ctx->tag_process_before_gating) {
			ipa3_ctx->tag_process_before_gating = false;
			IPA_ACTIVE_CLIENTS_PREP_SPECIAL(log_info,
					"TAG_PROCESS");
			ipa3_active_clients_log_inc(&log_info, false);
			ipa3_ctx->ipa3_active_clients.cnt = 1;
			queue_work(ipa3_ctx->power_mgmt_wq, &ipa3_tag_work);
		} else {
			ipa3_disable_clks();
		}
	}
	ipa3_active_clients_unlock();
}

void ipa3_inc_acquire_wakelock(void)
{
	unsigned long flags;

	spin_lock_irqsave(&ipa3_ctx->wakelock_ref_cnt.spinlock, flags);
	ipa3_ctx->wakelock_ref_cnt.cnt++;
	if (ipa3_ctx->wakelock_ref_cnt.cnt == 1)
		__pm_stay_awake(&ipa3_ctx->w_lock);
	IPADBG("active wakelock ref cnt = %d\n",
		ipa3_ctx->wakelock_ref_cnt.cnt);
	spin_unlock_irqrestore(&ipa3_ctx->wakelock_ref_cnt.spinlock, flags);
}

void ipa3_dec_release_wakelock(void)
{
	unsigned long flags;

	spin_lock_irqsave(&ipa3_ctx->wakelock_ref_cnt.spinlock, flags);
	ipa3_ctx->wakelock_ref_cnt.cnt--;
	IPADBG("active wakelock ref cnt = %d\n",
		ipa3_ctx->wakelock_ref_cnt.cnt);
	if (ipa3_ctx->wakelock_ref_cnt.cnt == 0)
		__pm_relax(&ipa3_ctx->w_lock);
	spin_unlock_irqrestore(&ipa3_ctx->wakelock_ref_cnt.spinlock, flags);
}

int ipa3_set_required_perf_profile(enum ipa_voltage_level floor_voltage,
				  u32 bandwidth_mbps)
{
	enum ipa_voltage_level needed_voltage;
	u32 clk_rate;

	IPADBG("floor_voltage=%d, bandwidth_mbps=%u",
					floor_voltage, bandwidth_mbps);

	if (floor_voltage < IPA_VOLTAGE_UNSPECIFIED ||
		floor_voltage >= IPA_VOLTAGE_MAX) {
		IPAERR("bad voltage\n");
		return -EINVAL;
	}

	if (ipa3_ctx->enable_clock_scaling) {
		IPADBG("Clock scaling is enabled\n");
		if (bandwidth_mbps >=
			ipa3_ctx->ctrl->clock_scaling_bw_threshold_turbo)
			needed_voltage = IPA_VOLTAGE_TURBO;
		else if (bandwidth_mbps >=
			ipa3_ctx->ctrl->clock_scaling_bw_threshold_nominal)
			needed_voltage = IPA_VOLTAGE_NOMINAL;
		else
			needed_voltage = IPA_VOLTAGE_SVS;
	} else {
		IPADBG("Clock scaling is disabled\n");
		needed_voltage = IPA_VOLTAGE_NOMINAL;
	}

	needed_voltage = max(needed_voltage, floor_voltage);
	switch (needed_voltage) {
	case IPA_VOLTAGE_SVS:
		clk_rate = ipa3_ctx->ctrl->ipa_clk_rate_svs;
		break;
	case IPA_VOLTAGE_NOMINAL:
		clk_rate = ipa3_ctx->ctrl->ipa_clk_rate_nominal;
		break;
	case IPA_VOLTAGE_TURBO:
		clk_rate = ipa3_ctx->ctrl->ipa_clk_rate_turbo;
		break;
	default:
		IPAERR("bad voltage\n");
		WARN_ON(1);
		return -EFAULT;
	}

	if (clk_rate == ipa3_ctx->curr_ipa_clk_rate) {
		IPADBG("Same voltage\n");
		return 0;
	}

	ipa3_active_clients_lock();
	ipa3_ctx->curr_ipa_clk_rate = clk_rate;
	IPADBG("setting clock rate to %u\n", ipa3_ctx->curr_ipa_clk_rate);
	if (ipa3_ctx->ipa3_active_clients.cnt > 0) {
		clk_set_rate(ipa3_clk, ipa3_ctx->curr_ipa_clk_rate);
		if (ipa3_ctx->ipa3_hw_mode != IPA_HW_MODE_VIRTUAL)
			if (msm_bus_scale_client_update_request(
			    ipa3_ctx->ipa_bus_hdl, ipa3_get_bus_vote()))
				WARN_ON(1);
	} else {
		IPADBG("clocks are gated, not setting rate\n");
	}
	ipa3_active_clients_unlock();
	IPADBG("Done\n");
	return 0;
}

static void ipa3_sps_process_irq_schedule_rel(void)
{
	queue_delayed_work(ipa3_ctx->transport_power_mgmt_wq,
		&ipa3_sps_release_resource_work,
		msecs_to_jiffies(IPA_TRANSPORT_PROD_TIMEOUT_MSEC));
}

void ipa3_suspend_handler(enum ipa_irq_type interrupt,
				void *private_data,
				void *interrupt_data)
{
	enum ipa_rm_resource_name resource;
	u32 suspend_data =
		((struct ipa_tx_suspend_irq_data *)interrupt_data)->endpoints;
	u32 bmsk = 1;
	u32 i = 0;
	int res;
	struct ipa_ep_cfg_holb holb_cfg;

	IPADBG("interrupt=%d, interrupt_data=%u\n", interrupt, suspend_data);
	memset(&holb_cfg, 0, sizeof(holb_cfg));
	holb_cfg.tmr_val = 0;

	for (i = 0; i < ipa3_ctx->ipa_num_pipes; i++) {
		if ((suspend_data & bmsk) && (ipa3_ctx->ep[i].valid)) {
			if (IPA_CLIENT_IS_APPS_CONS(ipa3_ctx->ep[i].client)) {
				if (!atomic_read(
					&ipa3_ctx->transport_pm.dec_clients)
					) {
					IPA_ACTIVE_CLIENTS_INC_EP(
						ipa3_ctx->ep[i].client);
					IPADBG("Pipes un-suspended.\n");
					IPADBG("Enter poll mode.\n");
					atomic_set(
					&ipa3_ctx->transport_pm.dec_clients,
					1);
					ipa3_sps_process_irq_schedule_rel();
				}
			} else {
				resource = ipa3_get_rm_resource_from_ep(i);
				res =
				ipa3_rm_request_resource_with_timer(resource);
				if (res == -EPERM &&
					IPA_CLIENT_IS_CONS(
					   ipa3_ctx->ep[i].client)) {
					holb_cfg.en = 1;
					res = ipa3_cfg_ep_holb_by_client(
					   ipa3_ctx->ep[i].client, &holb_cfg);
					if (res) {
						IPAERR("holb en fail, stall\n");
						BUG();
					}
				}
			}
		}
		bmsk = bmsk << 1;
	}
}

int ipa3_restore_suspend_handler(void)
{
	int result = 0;

	result  = ipa3_remove_interrupt_handler(IPA_TX_SUSPEND_IRQ);
	if (result) {
		IPAERR("remove handler for suspend interrupt failed\n");
		return -EPERM;
	}

	result = ipa3_add_interrupt_handler(IPA_TX_SUSPEND_IRQ,
			ipa3_suspend_handler, false, NULL);
	if (result) {
		IPAERR("register handler for suspend interrupt failed\n");
		result = -EPERM;
	}

	IPADBG("suspend handler successfully restored\n");

	return result;
}

static int ipa3_apps_cons_release_resource(void)
{
	return 0;
}

static int ipa3_apps_cons_request_resource(void)
{
	return 0;
}

static void ipa3_sps_release_resource(struct work_struct *work)
{
	
	if (atomic_read(&ipa3_ctx->transport_pm.dec_clients)) {
		if (atomic_read(&ipa3_ctx->transport_pm.eot_activity)) {
			IPADBG("EOT pending Re-scheduling\n");
			ipa3_sps_process_irq_schedule_rel();
		} else {
			atomic_set(&ipa3_ctx->transport_pm.dec_clients, 0);
			IPA_ACTIVE_CLIENTS_DEC_SPECIAL("SPS_RESOURCE");
		}
	}
	atomic_set(&ipa3_ctx->transport_pm.eot_activity, 0);
}

int ipa3_create_apps_resource(void)
{
	struct ipa_rm_create_params apps_cons_create_params;
	struct ipa_rm_perf_profile profile;
	int result = 0;

	memset(&apps_cons_create_params, 0,
				sizeof(apps_cons_create_params));
	apps_cons_create_params.name = IPA_RM_RESOURCE_APPS_CONS;
	apps_cons_create_params.request_resource =
		ipa3_apps_cons_request_resource;
	apps_cons_create_params.release_resource =
		ipa3_apps_cons_release_resource;
	result = ipa3_rm_create_resource(&apps_cons_create_params);
	if (result) {
		IPAERR("ipa3_rm_create_resource failed\n");
		return result;
	}

	profile.max_supported_bandwidth_mbps = IPA_APPS_MAX_BW_IN_MBPS;
	ipa3_rm_set_perf_profile(IPA_RM_RESOURCE_APPS_CONS, &profile);

	return result;
}

int ipa3_init_interrupts(void)
{
	int result;

	
	result = ipa3_interrupts_init(ipa3_res.ipa_irq, 0,
			master_dev);
	if (result) {
		IPAERR("ipa interrupts initialization failed\n");
		return -ENODEV;
	}

	
	result = ipa3_add_interrupt_handler(IPA_TX_SUSPEND_IRQ,
			ipa3_suspend_handler, false, NULL);
	if (result) {
		IPAERR("register handler for suspend interrupt failed\n");
		result = -ENODEV;
		goto fail_add_interrupt_handler;
	}

	return 0;

fail_add_interrupt_handler:
	free_irq(ipa3_res.ipa_irq, master_dev);
	return result;
}

static void ipa3_destroy_flt_tbl_idrs(void)
{
	int i;
	struct ipa3_flt_tbl *flt_tbl;

	for (i = 0; i < ipa3_ctx->ipa_num_pipes; i++) {
		if (!ipa_is_ep_support_flt(i))
			continue;

		flt_tbl = &ipa3_ctx->flt_tbl[i][IPA_IP_v4];
		idr_destroy(&flt_tbl->rule_ids);
		flt_tbl = &ipa3_ctx->flt_tbl[i][IPA_IP_v6];
		idr_destroy(&flt_tbl->rule_ids);
	}
}

static void ipa3_trigger_ipa_ready_cbs(void)
{
	struct ipa3_ready_cb_info *info;

	mutex_lock(&ipa3_ctx->lock);

	
	list_for_each_entry(info, &ipa3_ctx->ipa_ready_cb_list, link)
		if (info->ready_cb)
			info->ready_cb(info->user_data);

	mutex_unlock(&ipa3_ctx->lock);
}

static int ipa3_gsi_pre_fw_load_init(void)
{
	int result;

	
	ipa_write_reg(ipa3_ctx->mmio, IPA_ENABLE_GSI_OFST, 1);

	result = gsi_configure_regs(ipa3_res.transport_mem_base,
		ipa3_res.transport_mem_size,
		ipa3_res.ipa_mem_base);
	if (result) {
		IPAERR("Failed to configure GSI registers\n");
		return -EINVAL;
	}

	return 0;
}

static int ipa3_post_init(const struct ipa3_plat_drv_res *resource_p,
			  struct device *ipa_dev)
{
	int result;
	struct sps_bam_props bam_props = { 0 };
	struct gsi_per_props gsi_props;

	if (ipa3_ctx->transport_prototype == IPA_TRANSPORT_TYPE_GSI) {
		memset(&gsi_props, 0, sizeof(gsi_props));
		gsi_props.ee = resource_p->ee;
		gsi_props.intr = GSI_INTR_IRQ;
		gsi_props.irq = resource_p->transport_irq;
		gsi_props.phys_addr = resource_p->transport_mem_base;
		gsi_props.size = resource_p->transport_mem_size;
		gsi_props.notify_cb = ipa_gsi_notify_cb;
		gsi_props.req_clk_cb = NULL;
		gsi_props.rel_clk_cb = NULL;

		result = gsi_register_device(&gsi_props,
			&ipa3_ctx->gsi_dev_hdl);
		if (result != GSI_STATUS_SUCCESS) {
			IPAERR(":gsi register error - %d\n", result);
			result = -ENODEV;
			goto fail_register_device;
		}
		IPADBG("IPA gsi is registered\n");
	} else {
		
		bam_props.phys_addr = resource_p->transport_mem_base;
		bam_props.virt_size = resource_p->transport_mem_size;
		bam_props.irq = resource_p->transport_irq;
		bam_props.num_pipes = ipa3_ctx->ipa_num_pipes;
		bam_props.summing_threshold = IPA_SUMMING_THRESHOLD;
		bam_props.event_threshold = IPA_EVENT_THRESHOLD;
		bam_props.options |= SPS_BAM_NO_LOCAL_CLK_GATING;
		if (ipa3_ctx->ipa3_hw_mode != IPA_HW_MODE_VIRTUAL)
			bam_props.options |= SPS_BAM_OPT_IRQ_WAKEUP;
		if (ipa3_ctx->ipa_bam_remote_mode == true)
			bam_props.manage |= SPS_BAM_MGR_DEVICE_REMOTE;
		if (ipa3_ctx->smmu_present)
			bam_props.options |= SPS_BAM_SMMU_EN;
		bam_props.ee = resource_p->ee;
		bam_props.ipc_loglevel = 3;

		result = sps_register_bam_device(&bam_props,
			&ipa3_ctx->bam_handle);
		if (result) {
			IPAERR(":bam register error - %d\n", result);
			result = -EPROBE_DEFER;
			goto fail_register_device;
		}
		IPADBG("IPA BAM is registered\n");
	}

	
	if (ipa3_setup_apps_pipes()) {
		IPAERR(":failed to setup IPA-Apps pipes\n");
		result = -ENODEV;
		goto fail_setup_apps_pipes;
	}
	IPADBG("IPA System2Bam pipes were connected\n");

	if (ipa3_ctx->use_ipa_teth_bridge) {
		
		result = ipa3_teth_bridge_driver_init();
		if (result) {
			IPAERR(":teth_bridge init failed (%d)\n", -result);
			result = -ENODEV;
			goto fail_teth_bridge_driver_init;
		}
		IPADBG("teth_bridge initialized");
	}

	ipa3_debugfs_init();

	result = ipa3_uc_interface_init();
	if (result)
		IPAERR(":ipa Uc interface init failed (%d)\n", -result);
	else
		IPADBG(":ipa Uc interface init ok\n");

	result = ipa3_wdi_init();
	if (result)
		IPAERR(":wdi init failed (%d)\n", -result);
	else
		IPADBG(":wdi init ok\n");

	result = ipa3_usb_init();
	if (result)
		IPAERR(":ipa_usb init failed (%d)\n", -result);
	else
		IPADBG(":ipa_usb init ok\n");

	ipa3_register_panic_hdlr();

	ipa3_ctx->q6_proxy_clk_vote_valid = true;

	mutex_lock(&ipa3_ctx->lock);
	ipa3_ctx->ipa_initialization_complete = true;
	mutex_unlock(&ipa3_ctx->lock);

	ipa3_trigger_ipa_ready_cbs();
	complete_all(&ipa3_ctx->init_completion_obj);
	pr_info("IPA driver initialization was successful.\n");

	return 0;

fail_teth_bridge_driver_init:
	ipa3_teardown_apps_pipes();
fail_setup_apps_pipes:
	if (ipa3_ctx->transport_prototype == IPA_TRANSPORT_TYPE_GSI)
		gsi_deregister_device(ipa3_ctx->gsi_dev_hdl, false);
	else
		sps_deregister_bam_device(ipa3_ctx->bam_handle);
fail_register_device:
	ipa3_rm_delete_resource(IPA_RM_RESOURCE_APPS_CONS);
	ipa3_rm_exit();
	cdev_del(&ipa3_ctx->cdev);
	device_destroy(ipa3_ctx->class, ipa3_ctx->dev_num);
	unregister_chrdev_region(ipa3_ctx->dev_num, 1);
	if (ipa3_ctx->pipe_mem_pool)
		gen_pool_destroy(ipa3_ctx->pipe_mem_pool);
	dma_free_coherent(ipa3_ctx->pdev,
		ipa3_ctx->empty_rt_tbl_mem.size,
		ipa3_ctx->empty_rt_tbl_mem.base,
		ipa3_ctx->empty_rt_tbl_mem.phys_base);
	ipa3_destroy_flt_tbl_idrs();
	idr_destroy(&ipa3_ctx->ipa_idr);
	kmem_cache_destroy(ipa3_ctx->rx_pkt_wrapper_cache);
	kmem_cache_destroy(ipa3_ctx->tx_pkt_wrapper_cache);
	kmem_cache_destroy(ipa3_ctx->rt_tbl_cache);
	kmem_cache_destroy(ipa3_ctx->hdr_proc_ctx_offset_cache);
	kmem_cache_destroy(ipa3_ctx->hdr_proc_ctx_cache);
	kmem_cache_destroy(ipa3_ctx->hdr_offset_cache);
	kmem_cache_destroy(ipa3_ctx->hdr_cache);
	kmem_cache_destroy(ipa3_ctx->rt_rule_cache);
	kmem_cache_destroy(ipa3_ctx->flt_rule_cache);
	destroy_workqueue(ipa3_ctx->transport_power_mgmt_wq);
	destroy_workqueue(ipa3_ctx->power_mgmt_wq);
	iounmap(ipa3_ctx->mmio);
	ipa3_disable_clks();
	msm_bus_scale_unregister_client(ipa3_ctx->ipa_bus_hdl);
	if (ipa3_bus_scale_table) {
		msm_bus_cl_clear_pdata(ipa3_bus_scale_table);
		ipa3_bus_scale_table = NULL;
	}
	kfree(ipa3_ctx->ctrl);
	kfree(ipa3_ctx);
	ipa3_ctx = NULL;
	return result;
}

static void ipa3_trigger_fw_loading(void)
{
	int result;
	const struct firmware *fw;

	IPADBG("FW loading process initiated\n");

	result = request_firmware(&fw, IPA_FWS_PATH, ipa3_ctx->dev);
	if (result < 0) {
		IPAERR("request_firmware failed, error %d\n", result);
		return;
	}
	if (fw == NULL) {
		IPAERR("Firmware is NULL!\n");
		return;
	}

	IPADBG("FWs are available for loading\n");

	result = ipa3_load_fws(fw);
	if (result) {
		IPAERR("IPA FWs loading has failed\n");
		return;
	}

	result = gsi_enable_fw(ipa3_res.transport_mem_base,
				ipa3_res.transport_mem_size);
	if (result) {
		IPAERR("Failed to enable GSI FW\n");
		return;
	}

	release_firmware(fw);

	IPADBG("FW loading process is complete\n");
	ipa3_post_init(&ipa3_res, ipa3_ctx->dev);
}

static ssize_t ipa3_write(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	unsigned long missing;

	char dbg_buff[16] = { 0 };

	if (sizeof(dbg_buff) < count + 1)
		return -EFAULT;

	missing = copy_from_user(dbg_buff, buf, count);

	if (missing) {
		IPAERR("Unable to copy data from user\n");
		return -EFAULT;
	}

	
	if (ipa3_is_ready())
		return count;

	if (ipa3_ctx->transport_prototype == IPA_TRANSPORT_TYPE_GSI)
		ipa3_trigger_fw_loading();

	return count;
}

static int ipa3_pre_init(const struct ipa3_plat_drv_res *resource_p,
		struct device *ipa_dev)
{
	int result = 0;
	int i;
	struct ipa3_flt_tbl *flt_tbl;
	struct ipa3_rt_tbl_set *rset;
	struct ipa3_active_client_logging_info log_info;

	IPADBG("IPA Driver initialization started\n");

	BUILD_BUG_ON(sizeof(struct ipa3_hw_pkt_status) != IPA_PKT_STATUS_SIZE);

	ipa3_ctx = kzalloc(sizeof(*ipa3_ctx), GFP_KERNEL);
	if (!ipa3_ctx) {
		IPAERR(":kzalloc err.\n");
		result = -ENOMEM;
		goto fail_mem_ctx;
	}

	ipa3_ctx->pdev = ipa_dev;
	ipa3_ctx->uc_pdev = ipa_dev;
	ipa3_ctx->smmu_present = smmu_present;
	ipa3_ctx->ipa_wrapper_base = resource_p->ipa_mem_base;
	ipa3_ctx->ipa_hw_type = resource_p->ipa_hw_type;
	ipa3_ctx->ipa3_hw_mode = resource_p->ipa3_hw_mode;
	ipa3_ctx->use_ipa_teth_bridge = resource_p->use_ipa_teth_bridge;
	ipa3_ctx->ipa_bam_remote_mode = resource_p->ipa_bam_remote_mode;
	ipa3_ctx->modem_cfg_emb_pipe_flt = resource_p->modem_cfg_emb_pipe_flt;
	ipa3_ctx->wan_rx_ring_size = resource_p->wan_rx_ring_size;
	ipa3_ctx->skip_uc_pipe_reset = resource_p->skip_uc_pipe_reset;
	ipa3_ctx->tethered_flow_control = resource_p->tethered_flow_control;
	ipa3_ctx->transport_prototype = resource_p->transport_prototype;
	ipa3_ctx->ee = resource_p->ee;
	ipa3_ctx->apply_rg10_wa = resource_p->apply_rg10_wa;
	ipa3_ctx->ipa3_active_clients_logging.log_rdy = false;

	
	ipa3_ctx->aggregation_type = IPA_MBIM_16;
	ipa3_ctx->aggregation_byte_limit = 1;
	ipa3_ctx->aggregation_time_limit = 0;

	ipa3_ctx->ctrl = kzalloc(sizeof(*ipa3_ctx->ctrl), GFP_KERNEL);
	if (!ipa3_ctx->ctrl) {
		IPAERR("memory allocation error for ctrl\n");
		result = -ENOMEM;
		goto fail_mem_ctrl;
	}
	result = ipa3_controller_static_bind(ipa3_ctx->ctrl,
			ipa3_ctx->ipa_hw_type);
	if (result) {
		IPAERR("fail to static bind IPA ctrl.\n");
		result = -EFAULT;
		goto fail_bind;
	}

	if (ipa3_bus_scale_table) {
		IPADBG("Use bus scaling info from device tree\n");
		ipa3_ctx->ctrl->msm_bus_data_ptr = ipa3_bus_scale_table;
	}

	if (ipa3_ctx->ipa3_hw_mode != IPA_HW_MODE_VIRTUAL) {
		
		ipa3_ctx->ipa_bus_hdl =
			msm_bus_scale_register_client(
				ipa3_ctx->ctrl->msm_bus_data_ptr);
		if (!ipa3_ctx->ipa_bus_hdl) {
			IPAERR("fail to register with bus mgr!\n");
			result = -ENODEV;
			goto fail_bus_reg;
		}
	} else {
		IPADBG("Skipping bus scaling registration on Virtual plat\n");
	}

	if (ipa3_active_clients_log_init())
		goto fail_init_active_client;

	
	result = ipa3_get_clks(master_dev);
	if (result)
		goto fail_clk;

	
	ipa3_ctx->enable_clock_scaling = 1;
	ipa3_ctx->curr_ipa_clk_rate = ipa3_ctx->ctrl->ipa_clk_rate_turbo;

	
	ipa3_enable_clks();

	
	IPADBG("Mapping 0x%x\n", resource_p->ipa_mem_base +
		ipa3_ctx->ctrl->ipa_reg_base_ofst);
	ipa3_ctx->mmio = ioremap(resource_p->ipa_mem_base +
			ipa3_ctx->ctrl->ipa_reg_base_ofst,
			resource_p->ipa_mem_size);
	if (!ipa3_ctx->mmio) {
		IPAERR(":ipa-base ioremap err.\n");
		result = -EFAULT;
		goto fail_remap;
	}

	result = ipa3_init_hw();
	if (result) {
		IPAERR(":error initializing HW.\n");
		result = -ENODEV;
		goto fail_init_hw;
	}
	IPADBG("IPA HW initialization sequence completed");

	ipa3_ctx->ipa_num_pipes = ipa3_get_num_pipes();
	if (ipa3_ctx->ipa_num_pipes > IPA3_MAX_NUM_PIPES) {
		IPAERR("IPA has more pipes then supported! has %d, max %d\n",
			ipa3_ctx->ipa_num_pipes, IPA3_MAX_NUM_PIPES);
		result = -ENODEV;
		goto fail_init_hw;
	}

	ipa_init_ep_flt_bitmap();
	IPADBG("EP with flt support bitmap 0x%x (%u pipes)\n",
		ipa3_ctx->ep_flt_bitmap, ipa3_ctx->ep_flt_num);

	ipa3_ctx->ctrl->ipa_sram_read_settings();
	IPADBG("SRAM, size: 0x%x, restricted bytes: 0x%x\n",
		ipa3_ctx->smem_sz, ipa3_ctx->smem_restricted_bytes);

	IPADBG("hdr_lcl=%u ip4_rt_hash=%u ip4_rt_nonhash=%u\n",
		ipa3_ctx->hdr_tbl_lcl, ipa3_ctx->ip4_rt_tbl_hash_lcl,
		ipa3_ctx->ip4_rt_tbl_nhash_lcl);

	IPADBG("ip6_rt_hash=%u ip6_rt_nonhash=%u\n",
		ipa3_ctx->ip6_rt_tbl_hash_lcl, ipa3_ctx->ip6_rt_tbl_nhash_lcl);

	IPADBG("ip4_flt_hash=%u ip4_flt_nonhash=%u\n",
		ipa3_ctx->ip4_flt_tbl_hash_lcl,
		ipa3_ctx->ip4_flt_tbl_nhash_lcl);

	IPADBG("ip6_flt_hash=%u ip6_flt_nonhash=%u\n",
		ipa3_ctx->ip6_flt_tbl_hash_lcl,
		ipa3_ctx->ip6_flt_tbl_nhash_lcl);

	if (ipa3_ctx->smem_reqd_sz > ipa3_ctx->smem_sz) {
		IPAERR("SW expect more core memory, needed %d, avail %d\n",
			ipa3_ctx->smem_reqd_sz, ipa3_ctx->smem_sz);
		result = -ENOMEM;
		goto fail_init_hw;
	}

	mutex_init(&ipa3_ctx->ipa3_active_clients.mutex);
	spin_lock_init(&ipa3_ctx->ipa3_active_clients.spinlock);
	IPA_ACTIVE_CLIENTS_PREP_SPECIAL(log_info, "PROXY_CLK_VOTE");
	ipa3_active_clients_log_inc(&log_info, false);
	ipa3_ctx->ipa3_active_clients.cnt = 1;

	
	ipa3_set_resorce_groups_min_max_limits();

	
	ipa3_ctx->power_mgmt_wq =
		create_singlethread_workqueue("ipa_power_mgmt");
	if (!ipa3_ctx->power_mgmt_wq) {
		IPAERR("failed to create power mgmt wq\n");
		result = -ENOMEM;
		goto fail_init_hw;
	}

	ipa3_ctx->transport_power_mgmt_wq =
		create_singlethread_workqueue("transport_power_mgmt");
	if (!ipa3_ctx->transport_power_mgmt_wq) {
		IPAERR("failed to create transport power mgmt wq\n");
		result = -ENOMEM;
		goto fail_create_transport_wq;
	}

	spin_lock_init(&ipa3_ctx->transport_pm.lock);
	ipa3_ctx->transport_pm.res_granted = false;
	ipa3_ctx->transport_pm.res_rel_in_prog = false;

	
	ipa3_ctx->flt_rule_cache = kmem_cache_create("IPA FLT",
			sizeof(struct ipa3_flt_entry), 0, 0, NULL);
	if (!ipa3_ctx->flt_rule_cache) {
		IPAERR(":ipa flt cache create failed\n");
		result = -ENOMEM;
		goto fail_flt_rule_cache;
	}
	ipa3_ctx->rt_rule_cache = kmem_cache_create("IPA RT",
			sizeof(struct ipa3_rt_entry), 0, 0, NULL);
	if (!ipa3_ctx->rt_rule_cache) {
		IPAERR(":ipa rt cache create failed\n");
		result = -ENOMEM;
		goto fail_rt_rule_cache;
	}
	ipa3_ctx->hdr_cache = kmem_cache_create("IPA HDR",
			sizeof(struct ipa3_hdr_entry), 0, 0, NULL);
	if (!ipa3_ctx->hdr_cache) {
		IPAERR(":ipa hdr cache create failed\n");
		result = -ENOMEM;
		goto fail_hdr_cache;
	}
	ipa3_ctx->hdr_offset_cache =
	   kmem_cache_create("IPA HDR OFFSET",
			   sizeof(struct ipa3_hdr_offset_entry), 0, 0, NULL);
	if (!ipa3_ctx->hdr_offset_cache) {
		IPAERR(":ipa hdr off cache create failed\n");
		result = -ENOMEM;
		goto fail_hdr_offset_cache;
	}
	ipa3_ctx->hdr_proc_ctx_cache = kmem_cache_create("IPA HDR PROC CTX",
		sizeof(struct ipa3_hdr_proc_ctx_entry), 0, 0, NULL);
	if (!ipa3_ctx->hdr_proc_ctx_cache) {
		IPAERR(":ipa hdr proc ctx cache create failed\n");
		result = -ENOMEM;
		goto fail_hdr_proc_ctx_cache;
	}
	ipa3_ctx->hdr_proc_ctx_offset_cache =
		kmem_cache_create("IPA HDR PROC CTX OFFSET",
		sizeof(struct ipa3_hdr_proc_ctx_offset_entry), 0, 0, NULL);
	if (!ipa3_ctx->hdr_proc_ctx_offset_cache) {
		IPAERR(":ipa hdr proc ctx off cache create failed\n");
		result = -ENOMEM;
		goto fail_hdr_proc_ctx_offset_cache;
	}
	ipa3_ctx->rt_tbl_cache = kmem_cache_create("IPA RT TBL",
			sizeof(struct ipa3_rt_tbl), 0, 0, NULL);
	if (!ipa3_ctx->rt_tbl_cache) {
		IPAERR(":ipa rt tbl cache create failed\n");
		result = -ENOMEM;
		goto fail_rt_tbl_cache;
	}
	ipa3_ctx->tx_pkt_wrapper_cache =
	   kmem_cache_create("IPA TX PKT WRAPPER",
			   sizeof(struct ipa3_tx_pkt_wrapper), 0, 0, NULL);
	if (!ipa3_ctx->tx_pkt_wrapper_cache) {
		IPAERR(":ipa tx pkt wrapper cache create failed\n");
		result = -ENOMEM;
		goto fail_tx_pkt_wrapper_cache;
	}
	ipa3_ctx->rx_pkt_wrapper_cache =
	   kmem_cache_create("IPA RX PKT WRAPPER",
			   sizeof(struct ipa3_rx_pkt_wrapper), 0, 0, NULL);
	if (!ipa3_ctx->rx_pkt_wrapper_cache) {
		IPAERR(":ipa rx pkt wrapper cache create failed\n");
		result = -ENOMEM;
		goto fail_rx_pkt_wrapper_cache;
	}

	
	ipa3_ctx->dma_pool = dma_pool_create("ipa_tx", ipa3_ctx->pdev,
		IPA_NUM_DESC_PER_SW_TX * sizeof(struct sps_iovec),
		0, 0);
	if (!ipa3_ctx->dma_pool) {
		IPAERR("cannot alloc DMA pool.\n");
		result = -ENOMEM;
		goto fail_dma_pool;
	}

	
	INIT_LIST_HEAD(&ipa3_ctx->hdr_tbl.head_hdr_entry_list);
	for (i = 0; i < IPA_HDR_BIN_MAX; i++) {
		INIT_LIST_HEAD(&ipa3_ctx->hdr_tbl.head_offset_list[i]);
		INIT_LIST_HEAD(&ipa3_ctx->hdr_tbl.head_free_offset_list[i]);
	}
	INIT_LIST_HEAD(&ipa3_ctx->hdr_proc_ctx_tbl.head_proc_ctx_entry_list);
	for (i = 0; i < IPA_HDR_PROC_CTX_BIN_MAX; i++) {
		INIT_LIST_HEAD(&ipa3_ctx->hdr_proc_ctx_tbl.head_offset_list[i]);
		INIT_LIST_HEAD(&ipa3_ctx->
				hdr_proc_ctx_tbl.head_free_offset_list[i]);
	}
	INIT_LIST_HEAD(&ipa3_ctx->rt_tbl_set[IPA_IP_v4].head_rt_tbl_list);
	INIT_LIST_HEAD(&ipa3_ctx->rt_tbl_set[IPA_IP_v6].head_rt_tbl_list);
	for (i = 0; i < ipa3_ctx->ipa_num_pipes; i++) {
		if (!ipa_is_ep_support_flt(i))
			continue;

		flt_tbl = &ipa3_ctx->flt_tbl[i][IPA_IP_v4];
		INIT_LIST_HEAD(&flt_tbl->head_flt_rule_list);
		flt_tbl->in_sys[IPA_RULE_HASHABLE] =
			!ipa3_ctx->ip4_flt_tbl_hash_lcl;
		flt_tbl->in_sys[IPA_RULE_NON_HASHABLE] =
			!ipa3_ctx->ip4_flt_tbl_nhash_lcl;
		idr_init(&flt_tbl->rule_ids);

		flt_tbl = &ipa3_ctx->flt_tbl[i][IPA_IP_v6];
		INIT_LIST_HEAD(&flt_tbl->head_flt_rule_list);
		flt_tbl->in_sys[IPA_RULE_HASHABLE] =
			!ipa3_ctx->ip6_flt_tbl_hash_lcl;
		flt_tbl->in_sys[IPA_RULE_NON_HASHABLE] =
			!ipa3_ctx->ip6_flt_tbl_nhash_lcl;
		idr_init(&flt_tbl->rule_ids);
	}

	rset = &ipa3_ctx->reap_rt_tbl_set[IPA_IP_v4];
	INIT_LIST_HEAD(&rset->head_rt_tbl_list);
	rset = &ipa3_ctx->reap_rt_tbl_set[IPA_IP_v6];
	INIT_LIST_HEAD(&rset->head_rt_tbl_list);

	INIT_LIST_HEAD(&ipa3_ctx->intf_list);
	INIT_LIST_HEAD(&ipa3_ctx->msg_list);
	INIT_LIST_HEAD(&ipa3_ctx->pull_msg_list);
	init_waitqueue_head(&ipa3_ctx->msg_waitq);
	mutex_init(&ipa3_ctx->msg_lock);

	mutex_init(&ipa3_ctx->lock);
	mutex_init(&ipa3_ctx->nat_mem.lock);

	idr_init(&ipa3_ctx->ipa_idr);
	spin_lock_init(&ipa3_ctx->idr_lock);

	
	memset(&ipa3_ctx->wc_memb, 0, sizeof(ipa3_ctx->wc_memb));
	spin_lock_init(&ipa3_ctx->wc_memb.wlan_spinlock);
	spin_lock_init(&ipa3_ctx->wc_memb.ipa_tx_mul_spinlock);
	INIT_LIST_HEAD(&ipa3_ctx->wc_memb.wlan_comm_desc_list);
	ipa3_ctx->empty_rt_tbl_mem.size = IPA_HW_TBL_WIDTH;

	ipa3_ctx->empty_rt_tbl_mem.base =
		dma_alloc_coherent(ipa3_ctx->pdev,
				ipa3_ctx->empty_rt_tbl_mem.size,
				    &ipa3_ctx->empty_rt_tbl_mem.phys_base,
				    GFP_KERNEL);
	if (!ipa3_ctx->empty_rt_tbl_mem.base) {
		IPAERR("DMA buff alloc fail %d bytes for empty routing tbl\n",
				ipa3_ctx->empty_rt_tbl_mem.size);
		result = -ENOMEM;
		goto fail_empty_rt_tbl_alloc;
	}
	if (ipa3_ctx->empty_rt_tbl_mem.phys_base &
		IPA_HW_TBL_SYSADDR_ALIGNMENT) {
		IPAERR("Empty rt-table buf is not address aligned 0x%pad\n",
				&ipa3_ctx->empty_rt_tbl_mem.phys_base);
		result = -EFAULT;
		goto fail_empty_rt_tbl;
	}
	memset(ipa3_ctx->empty_rt_tbl_mem.base, 0,
			ipa3_ctx->empty_rt_tbl_mem.size);
	IPADBG("empty routing table was allocated in system memory");

	
	if (resource_p->ipa_pipe_mem_size)
		ipa3_pipe_mem_init(resource_p->ipa_pipe_mem_start_ofst,
				resource_p->ipa_pipe_mem_size);

	ipa3_ctx->class = class_create(THIS_MODULE, DRV_NAME);

	result = alloc_chrdev_region(&ipa3_ctx->dev_num, 0, 1, DRV_NAME);
	if (result) {
		IPAERR("alloc_chrdev_region err.\n");
		result = -ENODEV;
		goto fail_alloc_chrdev_region;
	}

	ipa3_ctx->dev = device_create(ipa3_ctx->class, NULL, ipa3_ctx->dev_num,
			ipa3_ctx, DRV_NAME);
	if (IS_ERR(ipa3_ctx->dev)) {
		IPAERR(":device_create err.\n");
		result = -ENODEV;
		goto fail_device_create;
	}

	cdev_init(&ipa3_ctx->cdev, &ipa3_drv_fops);
	ipa3_ctx->cdev.owner = THIS_MODULE;
	ipa3_ctx->cdev.ops = &ipa3_drv_fops;  

	result = cdev_add(&ipa3_ctx->cdev, ipa3_ctx->dev_num, 1);
	if (result) {
		IPAERR(":cdev_add err=%d\n", -result);
		result = -ENODEV;
		goto fail_cdev_add;
	}
	IPADBG("ipa cdev added successful. major:%d minor:%d\n",
			MAJOR(ipa3_ctx->dev_num),
			MINOR(ipa3_ctx->dev_num));

	if (ipa3_create_nat_device()) {
		IPAERR("unable to create nat device\n");
		result = -ENODEV;
		goto fail_nat_dev_add;
	}

	
	wakeup_source_init(&ipa3_ctx->w_lock, "IPA_WS");
	spin_lock_init(&ipa3_ctx->wakelock_ref_cnt.spinlock);

	
	result = ipa3_rm_initialize();
	if (result) {
		IPAERR("RM initialization failed (%d)\n", -result);
		result = -ENODEV;
		goto fail_ipa_rm_init;
	}
	IPADBG("IPA resource manager initialized");

	result = ipa3_create_apps_resource();
	if (result) {
		IPAERR("Failed to create APPS_CONS resource\n");
		result = -ENODEV;
		goto fail_create_apps_resource;
	}

	if (!ipa3_ctx->apply_rg10_wa) {
		result = ipa3_init_interrupts();
		if (result) {
			IPAERR("ipa initialization of interrupts failed\n");
			result = -ENODEV;
			goto fail_ipa_init_interrupts;
		}
	} else {
		IPADBG("Initialization of ipa interrupts skipped\n");
	}

	INIT_LIST_HEAD(&ipa3_ctx->ipa_ready_cb_list);

	init_completion(&ipa3_ctx->init_completion_obj);

	if (ipa3_ctx->transport_prototype == IPA_TRANSPORT_TYPE_GSI) {
		result = ipa3_gsi_pre_fw_load_init();
		if (result) {
			IPAERR("ipa gsi pre FW loading procedure failed\n");
			result = -ENODEV;
			goto fail_ipa_init_interrupts;
		}
	}
	
	else
		return ipa3_post_init(resource_p, ipa_dev);

	return 0;

fail_ipa_init_interrupts:
	ipa3_rm_delete_resource(IPA_RM_RESOURCE_APPS_CONS);
fail_create_apps_resource:
	ipa3_rm_exit();
fail_ipa_rm_init:
fail_nat_dev_add:
	cdev_del(&ipa3_ctx->cdev);
fail_cdev_add:
	device_destroy(ipa3_ctx->class, ipa3_ctx->dev_num);
fail_device_create:
	unregister_chrdev_region(ipa3_ctx->dev_num, 1);
fail_alloc_chrdev_region:
	if (ipa3_ctx->pipe_mem_pool)
		gen_pool_destroy(ipa3_ctx->pipe_mem_pool);
fail_empty_rt_tbl:
	dma_free_coherent(ipa3_ctx->pdev,
			  ipa3_ctx->empty_rt_tbl_mem.size,
			  ipa3_ctx->empty_rt_tbl_mem.base,
			  ipa3_ctx->empty_rt_tbl_mem.phys_base);
fail_empty_rt_tbl_alloc:
	ipa3_destroy_flt_tbl_idrs();
	idr_destroy(&ipa3_ctx->ipa_idr);
fail_dma_pool:
	kmem_cache_destroy(ipa3_ctx->rx_pkt_wrapper_cache);
fail_rx_pkt_wrapper_cache:
	kmem_cache_destroy(ipa3_ctx->tx_pkt_wrapper_cache);
fail_tx_pkt_wrapper_cache:
	kmem_cache_destroy(ipa3_ctx->rt_tbl_cache);
fail_rt_tbl_cache:
	kmem_cache_destroy(ipa3_ctx->hdr_proc_ctx_offset_cache);
fail_hdr_proc_ctx_offset_cache:
	kmem_cache_destroy(ipa3_ctx->hdr_proc_ctx_cache);
fail_hdr_proc_ctx_cache:
	kmem_cache_destroy(ipa3_ctx->hdr_offset_cache);
fail_hdr_offset_cache:
	kmem_cache_destroy(ipa3_ctx->hdr_cache);
fail_hdr_cache:
	kmem_cache_destroy(ipa3_ctx->rt_rule_cache);
fail_rt_rule_cache:
	kmem_cache_destroy(ipa3_ctx->flt_rule_cache);
fail_flt_rule_cache:
	destroy_workqueue(ipa3_ctx->transport_power_mgmt_wq);
fail_create_transport_wq:
	destroy_workqueue(ipa3_ctx->power_mgmt_wq);
fail_init_hw:
	iounmap(ipa3_ctx->mmio);
fail_remap:
	ipa3_disable_clks();
fail_clk:
	ipa3_active_clients_log_destroy();
fail_init_active_client:
	msm_bus_scale_unregister_client(ipa3_ctx->ipa_bus_hdl);
fail_bus_reg:
	ipa3_bus_scale_table = NULL;
fail_bind:
	kfree(ipa3_ctx->ctrl);
fail_mem_ctrl:
	kfree(ipa3_ctx);
	ipa3_ctx = NULL;
fail_mem_ctx:
	return result;
}

static int get_ipa_dts_configuration(struct platform_device *pdev,
		struct ipa3_plat_drv_res *ipa_drv_res)
{
	int result;
	struct resource *resource;

	
	ipa_drv_res->ipa_pipe_mem_start_ofst = IPA_PIPE_MEM_START_OFST;
	ipa_drv_res->ipa_pipe_mem_size = IPA_PIPE_MEM_SIZE;
	ipa_drv_res->ipa_hw_type = 0;
	ipa_drv_res->ipa3_hw_mode = 0;
	ipa_drv_res->ipa_bam_remote_mode = false;
	ipa_drv_res->modem_cfg_emb_pipe_flt = false;
	ipa_drv_res->wan_rx_ring_size = IPA_GENERIC_RX_POOL_SZ;
	ipa_drv_res->apply_rg10_wa = false;

	smmu_disable_htw = of_property_read_bool(pdev->dev.of_node,
			"qcom,smmu-disable-htw");

	
	result = of_property_read_u32(pdev->dev.of_node, "qcom,ipa-hw-ver",
					&ipa_drv_res->ipa_hw_type);
	if ((result) || (ipa_drv_res->ipa_hw_type == 0)) {
		IPAERR(":get resource failed for ipa-hw-ver!\n");
		return -ENODEV;
	}
	IPADBG(": ipa_hw_type = %d", ipa_drv_res->ipa_hw_type);

	if (ipa_drv_res->ipa_hw_type < IPA_HW_v3_0) {
		IPAERR(":IPA version below 3.0 not supported!\n");
		return -ENODEV;
	}

	
	result = of_property_read_u32(pdev->dev.of_node, "qcom,ipa-hw-mode",
			&ipa_drv_res->ipa3_hw_mode);
	if (result)
		IPADBG("using default (IPA_MODE_NORMAL) for ipa-hw-mode\n");
	else
		IPADBG(": found ipa_drv_res->ipa3_hw_mode = %d",
				ipa_drv_res->ipa3_hw_mode);

	
	result = of_property_read_u32(pdev->dev.of_node,
			"qcom,wan-rx-ring-size",
			&ipa_drv_res->wan_rx_ring_size);
	if (result)
		IPADBG("using default for wan-rx-ring-size\n");
	else
		IPADBG(": found ipa_drv_res->wan-rx-ring-size = %u",
				ipa_drv_res->wan_rx_ring_size);

	ipa_drv_res->use_ipa_teth_bridge =
			of_property_read_bool(pdev->dev.of_node,
			"qcom,use-ipa-tethering-bridge");
	IPADBG(": using TBDr = %s",
		ipa_drv_res->use_ipa_teth_bridge
		? "True" : "False");

	ipa_drv_res->ipa_bam_remote_mode =
			of_property_read_bool(pdev->dev.of_node,
			"qcom,ipa-bam-remote-mode");
	IPADBG(": ipa bam remote mode = %s\n",
			ipa_drv_res->ipa_bam_remote_mode
			? "True" : "False");

	ipa_drv_res->modem_cfg_emb_pipe_flt =
			of_property_read_bool(pdev->dev.of_node,
			"qcom,modem-cfg-emb-pipe-flt");
	IPADBG(": modem configure embedded pipe filtering = %s\n",
			ipa_drv_res->modem_cfg_emb_pipe_flt
			? "True" : "False");

	ipa_drv_res->skip_uc_pipe_reset =
		of_property_read_bool(pdev->dev.of_node,
		"qcom,skip-uc-pipe-reset");
	IPADBG(": skip uC pipe reset = %s\n",
		ipa_drv_res->skip_uc_pipe_reset
		? "True" : "False");

	ipa_drv_res->tethered_flow_control =
		of_property_read_bool(pdev->dev.of_node,
		"qcom,tethered-flow-control");
	IPADBG(": Use apps based flow control = %s\n",
		ipa_drv_res->tethered_flow_control
		? "True" : "False");

	if (of_property_read_bool(pdev->dev.of_node,
		"qcom,use-gsi"))
		ipa_drv_res->transport_prototype = IPA_TRANSPORT_TYPE_GSI;
	else
		ipa_drv_res->transport_prototype = IPA_TRANSPORT_TYPE_SPS;

	IPADBG(": transport type = %s\n",
		ipa_drv_res->transport_prototype == IPA_TRANSPORT_TYPE_SPS
		? "SPS" : "GSI");

	
	resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
			"ipa-base");
	if (!resource) {
		IPAERR(":get resource failed for ipa-base!\n");
		return -ENODEV;
	}
	ipa_drv_res->ipa_mem_base = resource->start;
	ipa_drv_res->ipa_mem_size = resource_size(resource);
	IPADBG(": ipa-base = 0x%x, size = 0x%x\n",
			ipa_drv_res->ipa_mem_base,
			ipa_drv_res->ipa_mem_size);

	if (ipa_drv_res->transport_prototype == IPA_TRANSPORT_TYPE_SPS) {
		
		resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
				"bam-base");
		if (!resource) {
			IPAERR(":get resource failed for bam-base!\n");
			return -ENODEV;
		}
		ipa_drv_res->transport_mem_base = resource->start;
		ipa_drv_res->transport_mem_size = resource_size(resource);
		IPADBG(": bam-base = 0x%x, size = 0x%x\n",
				ipa_drv_res->transport_mem_base,
				ipa_drv_res->transport_mem_size);

		
		resource = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
				"bam-irq");
		if (!resource) {
			IPAERR(":get resource failed for bam-irq!\n");
			return -ENODEV;
		}
		ipa_drv_res->transport_irq = resource->start;
		IPADBG(": bam-irq = %d\n", ipa_drv_res->transport_irq);
	} else {
		
		resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
				"gsi-base");
		if (!resource) {
			IPAERR(":get resource failed for gsi-base!\n");
			return -ENODEV;
		}
		ipa_drv_res->transport_mem_base = resource->start;
		ipa_drv_res->transport_mem_size = resource_size(resource);
		IPADBG(": gsi-base = 0x%x, size = 0x%x\n",
				ipa_drv_res->transport_mem_base,
				ipa_drv_res->transport_mem_size);

		
		resource = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
				"gsi-irq");
		if (!resource) {
			IPAERR(":get resource failed for gsi-irq!\n");
			return -ENODEV;
		}
		ipa_drv_res->transport_irq = resource->start;
		IPADBG(": gsi-irq = %d\n", ipa_drv_res->transport_irq);
	}

	
	resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
			"ipa-pipe-mem");
	if (!resource) {
		IPADBG(":not using pipe memory - resource nonexisting\n");
	} else {
		ipa_drv_res->ipa_pipe_mem_start_ofst = resource->start;
		ipa_drv_res->ipa_pipe_mem_size = resource_size(resource);
		IPADBG(":using pipe memory - at 0x%x of size 0x%x\n",
				ipa_drv_res->ipa_pipe_mem_start_ofst,
				ipa_drv_res->ipa_pipe_mem_size);
	}

	
	resource = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
			"ipa-irq");
	if (!resource) {
		IPAERR(":get resource failed for ipa-irq!\n");
		return -ENODEV;
	}
	ipa_drv_res->ipa_irq = resource->start;
	IPADBG(":ipa-irq = %d\n", ipa_drv_res->ipa_irq);

	result = of_property_read_u32(pdev->dev.of_node, "qcom,ee",
			&ipa_drv_res->ee);
	if (result)
		ipa_drv_res->ee = 0;

	ipa_drv_res->apply_rg10_wa =
		of_property_read_bool(pdev->dev.of_node,
		"qcom,use-rg10-limitation-mitigation");
	IPADBG(": Use Register Group 10 limitation mitigation = %s\n",
		ipa_drv_res->apply_rg10_wa
		? "True" : "False");

	return 0;
}

static int ipa_smmu_wlan_cb_probe(struct device *dev)
{
	struct ipa_smmu_cb_ctx *cb = &smmu_cb[IPA_SMMU_CB_WLAN];
	int disable_htw = 1;
	int atomic_ctx = 1;
	int ret;

	IPADBG("sub pdev=%p\n", dev);

	cb->dev = dev;
	cb->iommu = iommu_domain_alloc(msm_iommu_get_bus(dev));
	if (!cb->iommu) {
		IPAERR("could not alloc iommu domain\n");
		
		return -EPROBE_DEFER;
	}

	if (smmu_disable_htw) {
		ret = iommu_domain_set_attr(cb->iommu,
			DOMAIN_ATTR_COHERENT_HTW_DISABLE,
			&disable_htw);
		if (ret) {
			IPAERR("couldn't disable coherent HTW\n");
			return -EIO;
		}
	}

	if (iommu_domain_set_attr(cb->iommu,
				DOMAIN_ATTR_ATOMIC,
				&atomic_ctx)) {
		IPAERR("couldn't disable coherent HTW\n");
		return -EIO;
	}

	ret = iommu_attach_device(cb->iommu, dev);
	if (ret) {
		IPAERR("could not attach device ret=%d\n", ret);
		return ret;
	}

	cb->valid = true;

	return 0;
}

static int ipa_smmu_uc_cb_probe(struct device *dev)
{
	struct ipa_smmu_cb_ctx *cb = &smmu_cb[IPA_SMMU_CB_UC];
	int disable_htw = 1;
	int ret;

	IPADBG("sub pdev=%p\n", dev);

	if (dma_set_mask(dev, DMA_BIT_MASK(32)) ||
		    dma_set_coherent_mask(dev, DMA_BIT_MASK(32))) {
		IPAERR("DMA set mask failed\n");
		return -EOPNOTSUPP;
	}

	cb->dev = dev;
	cb->mapping = arm_iommu_create_mapping(msm_iommu_get_bus(dev),
			IPA_SMMU_UC_VA_START, IPA_SMMU_UC_VA_SIZE);
	if (IS_ERR(cb->mapping)) {
		IPADBG("Fail to create mapping\n");
		
		return -EPROBE_DEFER;
	}

	if (smmu_disable_htw) {
		if (iommu_domain_set_attr(cb->mapping->domain,
				DOMAIN_ATTR_COHERENT_HTW_DISABLE,
				 &disable_htw)) {
			IPAERR("couldn't disable coherent HTW\n");
			return -EIO;
		}
	}


	ret = arm_iommu_attach_device(cb->dev, cb->mapping);
	if (ret) {
		IPAERR("could not attach device ret=%d\n", ret);
		return ret;
	}

	cb->valid = true;
	cb->next_addr = IPA_SMMU_UC_VA_END;
	ipa3_ctx->uc_pdev = dev;

	return 0;
}

static int ipa_smmu_ap_cb_probe(struct device *dev)
{
	struct ipa_smmu_cb_ctx *cb = &smmu_cb[IPA_SMMU_CB_AP];
	int result;
	int disable_htw = 1;
	int atomic_ctx = 1;

	IPADBG("sub pdev=%p\n", dev);

	if (dma_set_mask(dev, DMA_BIT_MASK(32)) ||
		    dma_set_coherent_mask(dev, DMA_BIT_MASK(32))) {
		IPAERR("DMA set mask failed\n");
		return -EOPNOTSUPP;
	}

	cb->dev = dev;
	cb->mapping = arm_iommu_create_mapping(msm_iommu_get_bus(dev),
			IPA_SMMU_AP_VA_START, IPA_SMMU_AP_VA_SIZE);
	if (IS_ERR(cb->mapping)) {
		IPADBG("Fail to create mapping\n");
		
		return -EPROBE_DEFER;
	}

	if (smmu_disable_htw) {
		if (iommu_domain_set_attr(cb->mapping->domain,
				DOMAIN_ATTR_COHERENT_HTW_DISABLE,
				 &disable_htw)) {
			IPAERR("couldn't disable coherent HTW\n");
			arm_iommu_detach_device(cb->dev);
			return -EIO;
		}
	}

	if (iommu_domain_set_attr(cb->mapping->domain,
				  DOMAIN_ATTR_ATOMIC,
				  &atomic_ctx)) {
		IPAERR("couldn't set domain as atomic\n");
		arm_iommu_detach_device(cb->dev);
		return -EIO;
	}

	result = arm_iommu_attach_device(cb->dev, cb->mapping);
	if (result) {
		IPAERR("couldn't attach to IOMMU ret=%d\n", result);
		return result;
	}

	cb->valid = true;
	smmu_present = true;

	if (!ipa3_bus_scale_table)
		ipa3_bus_scale_table = msm_bus_cl_get_pdata(ipa3_pdev);

	
	result = ipa3_pre_init(&ipa3_res, dev);
	if (result) {
		IPAERR("ipa_init failed\n");
		arm_iommu_detach_device(cb->dev);
		arm_iommu_release_mapping(cb->mapping);
		return result;
	}

	return result;
}

int ipa3_plat_drv_probe(struct platform_device *pdev_p,
	struct ipa_api_controller *api_ctrl, struct of_device_id *pdrv_match)
{
	int result;
	struct device *dev = &pdev_p->dev;

	IPADBG("IPA driver probing started\n");

	if (of_device_is_compatible(dev->of_node, "qcom,ipa-smmu-ap-cb"))
		return ipa_smmu_ap_cb_probe(dev);

	if (of_device_is_compatible(dev->of_node, "qcom,ipa-smmu-wlan-cb"))
		return ipa_smmu_wlan_cb_probe(dev);

	if (of_device_is_compatible(dev->of_node, "qcom,ipa-smmu-uc-cb"))
		return ipa_smmu_uc_cb_probe(dev);

	master_dev = dev;
	if (!ipa3_pdev)
		ipa3_pdev = pdev_p;

	result = get_ipa_dts_configuration(pdev_p, &ipa3_res);
	if (result) {
		IPAERR("IPA dts parsing failed\n");
		return result;
	}

	result = ipa3_bind_api_controller(ipa3_res.ipa_hw_type, api_ctrl);
	if (result) {
		IPAERR("IPA API binding failed\n");
		return result;
	}

	if (of_property_read_bool(pdev_p->dev.of_node, "qcom,arm-smmu")) {
		arm_smmu = true;
		result = of_platform_populate(pdev_p->dev.of_node,
				pdrv_match, NULL, &pdev_p->dev);
	} else if (of_property_read_bool(pdev_p->dev.of_node,
				"qcom,msm-smmu")) {
		IPAERR("Legacy IOMMU not supported\n");
		result = -EOPNOTSUPP;
	} else {
		if (dma_set_mask(&pdev_p->dev, DMA_BIT_MASK(32)) ||
			    dma_set_coherent_mask(&pdev_p->dev,
			    DMA_BIT_MASK(32))) {
			IPAERR("DMA set mask failed\n");
			return -EOPNOTSUPP;
		}

		if (!ipa3_bus_scale_table)
			ipa3_bus_scale_table = msm_bus_cl_get_pdata(pdev_p);

		
		result = ipa3_pre_init(&ipa3_res, dev);
		if (result) {
			IPAERR("ipa3_init failed\n");
			return result;
		}
	}

	return result;
}

int ipa3_ap_suspend(struct device *dev)
{
	int i;

	IPADBG("Enter...\n");

	
	for (i = 0; i < ipa3_ctx->ipa_num_pipes; i++) {
		if (ipa3_ctx->ep[i].sys &&
			atomic_read(&ipa3_ctx->ep[i].sys->curr_polling_state)) {
			IPAERR("EP %d is in polling state, do not suspend\n",
				i);
			return -EAGAIN;
		}
	}

	
	atomic_set(&ipa3_ctx->transport_pm.eot_activity, 0);
	ipa3_sps_release_resource(NULL);
	IPADBG("Exit\n");

	return 0;
}

int ipa3_ap_resume(struct device *dev)
{
	return 0;
}

struct ipa3_context *ipa3_get_ctx(void)
{
	return ipa3_ctx;
}

static void ipa_gsi_request_resource(struct work_struct *work)
{
	unsigned long flags;
	int ret;

	
	IPA_ACTIVE_CLIENTS_INC_SIMPLE();

	
	spin_lock_irqsave(&ipa3_ctx->transport_pm.lock, flags);
	ipa3_ctx->transport_pm.res_granted = true;

	IPADBG("IPA is ON, calling gsi driver\n");
	ret = gsi_complete_clk_grant(ipa3_ctx->gsi_dev_hdl);
	if (ret != GSI_STATUS_SUCCESS)
		IPAERR("gsi_complete_clk_grant failed %d\n", ret);

	spin_unlock_irqrestore(&ipa3_ctx->transport_pm.lock, flags);
}

void ipa_gsi_req_res_cb(void *user_data, bool *granted)
{
	unsigned long flags;
	struct ipa3_active_client_logging_info log_info;

	spin_lock_irqsave(&ipa3_ctx->transport_pm.lock, flags);

	
	cancel_delayed_work(&ipa_gsi_release_resource_work);
	ipa3_ctx->transport_pm.res_rel_in_prog = false;

	if (ipa3_ctx->transport_pm.res_granted) {
		*granted = true;
	} else {
		IPA_ACTIVE_CLIENTS_PREP_SPECIAL(log_info, "GSI_RESOURCE");
		if (ipa3_inc_client_enable_clks_no_block(&log_info) == 0) {
			ipa3_ctx->transport_pm.res_granted = true;
			*granted = true;
		} else {
			queue_work(ipa3_ctx->transport_power_mgmt_wq,
				   &ipa_gsi_request_resource_work);
			*granted = false;
		}
	}
	spin_unlock_irqrestore(&ipa3_ctx->transport_pm.lock, flags);
}

static void ipa_gsi_release_resource(struct work_struct *work)
{
	unsigned long flags;
	bool dec_clients = false;

	spin_lock_irqsave(&ipa3_ctx->transport_pm.lock, flags);
	
	if (ipa3_ctx->transport_pm.res_rel_in_prog) {
		dec_clients = true;
		ipa3_ctx->transport_pm.res_rel_in_prog = false;
		ipa3_ctx->transport_pm.res_granted = false;
	}
	spin_unlock_irqrestore(&ipa3_ctx->transport_pm.lock, flags);
	if (dec_clients)
		IPA_ACTIVE_CLIENTS_DEC_SPECIAL("GSI_RESOURCE");
}

int ipa_gsi_rel_res_cb(void *user_data)
{
	unsigned long flags;

	spin_lock_irqsave(&ipa3_ctx->transport_pm.lock, flags);

	ipa3_ctx->transport_pm.res_rel_in_prog = true;
	queue_delayed_work(ipa3_ctx->transport_power_mgmt_wq,
			   &ipa_gsi_release_resource_work,
			   msecs_to_jiffies(IPA_TRANSPORT_PROD_TIMEOUT_MSEC));

	spin_unlock_irqrestore(&ipa3_ctx->transport_pm.lock, flags);
	return 0;
}

static void ipa_gsi_notify_cb(struct gsi_per_notify *notify)
{
	switch (notify->evt_id) {
	case GSI_PER_EVT_GLOB_ERROR:
		IPAERR("Got GSI_PER_EVT_GLOB_ERROR\n");
		IPAERR("Err_desc = 0x%04x\n", notify->data.err_desc);
		break;
	case GSI_PER_EVT_GLOB_GP1:
		IPAERR("Got GSI_PER_EVT_GLOB_GP1\n");
		BUG();
		break;
	case GSI_PER_EVT_GLOB_GP2:
		IPAERR("Got GSI_PER_EVT_GLOB_GP2\n");
		BUG();
		break;
	case GSI_PER_EVT_GLOB_GP3:
		IPAERR("Got GSI_PER_EVT_GLOB_GP3\n");
		BUG();
		break;
	case GSI_PER_EVT_GENERAL_BREAK_POINT:
		IPAERR("Got GSI_PER_EVT_GENERAL_BREAK_POINT\n");
		break;
	case GSI_PER_EVT_GENERAL_BUS_ERROR:
		IPAERR("Got GSI_PER_EVT_GENERAL_BUS_ERROR\n");
		BUG();
		break;
	case GSI_PER_EVT_GENERAL_CMD_FIFO_OVERFLOW:
		IPAERR("Got GSI_PER_EVT_GENERAL_CMD_FIFO_OVERFLOW\n");
		BUG();
		break;
	case GSI_PER_EVT_GENERAL_MCS_STACK_OVERFLOW:
		IPAERR("Got GSI_PER_EVT_GENERAL_MCS_STACK_OVERFLOW\n");
		BUG();
		break;
	default:
		IPAERR("Received unexpected evt: %d\n",
			notify->evt_id);
		BUG();
	}
}

int ipa3_register_ipa_ready_cb(void (*ipa_ready_cb)(void *), void *user_data)
{
	struct ipa3_ready_cb_info *cb_info = NULL;

	mutex_lock(&ipa3_ctx->lock);
	if (ipa3_ctx->ipa_initialization_complete) {
		mutex_unlock(&ipa3_ctx->lock);
		IPADBG("IPA driver finished initialization already\n");
		return -EEXIST;
	}

	cb_info = kmalloc(sizeof(struct ipa3_ready_cb_info), GFP_KERNEL);
	if (!cb_info) {
		mutex_unlock(&ipa3_ctx->lock);
		return -ENOMEM;
	}

	cb_info->ready_cb = ipa_ready_cb;
	cb_info->user_data = user_data;

	list_add_tail(&cb_info->link, &ipa3_ctx->ipa_ready_cb_list);
	mutex_unlock(&ipa3_ctx->lock);

	return 0;
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("IPA HW device driver");
