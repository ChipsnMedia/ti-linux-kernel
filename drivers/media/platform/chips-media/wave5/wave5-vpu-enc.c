// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Wave5 series multi-standard codec IP - encoder interface
 *
 * Copyright (C) 2021 CHIPS&MEDIA INC
 */

#include "wave5-helper.h"

#define VPU_ENC_DEV_NAME "C&M Wave5 VPU encoder"
#define VPU_ENC_DRV_NAME "wave5-enc"

static const struct vpu_format enc_fmt_list[FMT_TYPES][MAX_FMTS] = {
	[VPU_FMT_TYPE_CODEC] = {
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_HEVC,
			.max_width = W5_MAX_ENC_PIC_WIDTH,
			.min_width = W5_MIN_ENC_PIC_WIDTH,
			.max_height = W5_MAX_ENC_PIC_HEIGHT,
			.min_height = W5_MIN_ENC_PIC_HEIGHT,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_H264,
			.max_width = W5_MAX_ENC_PIC_WIDTH,
			.min_width = W5_MIN_ENC_PIC_WIDTH,
			.max_height = W5_MAX_ENC_PIC_HEIGHT,
			.min_height = W5_MIN_ENC_PIC_HEIGHT,
		},
	},
	[VPU_FMT_TYPE_RAW] = {
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_YUV420,
			.max_width = W5_MAX_ENC_PIC_WIDTH,
			.min_width = W5_MIN_ENC_PIC_WIDTH,
			.max_height = W5_MAX_ENC_PIC_HEIGHT,
			.min_height = W5_MIN_ENC_PIC_HEIGHT,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_NV12,
			.max_width = W5_MAX_ENC_PIC_WIDTH,
			.min_width = W5_MIN_ENC_PIC_WIDTH,
			.max_height = W5_MAX_ENC_PIC_HEIGHT,
			.min_height = W5_MIN_ENC_PIC_HEIGHT,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_NV21,
			.max_width = W5_MAX_ENC_PIC_WIDTH,
			.min_width = W5_MIN_ENC_PIC_WIDTH,
			.max_height = W5_MAX_ENC_PIC_HEIGHT,
			.min_height = W5_MIN_ENC_PIC_HEIGHT,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_YUV420M,
			.max_width = W5_MAX_ENC_PIC_WIDTH,
			.min_width = W5_MIN_ENC_PIC_WIDTH,
			.max_height = W5_MAX_ENC_PIC_HEIGHT,
			.min_height = W5_MIN_ENC_PIC_HEIGHT,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_NV12M,
			.max_width = W5_MAX_ENC_PIC_WIDTH,
			.min_width = W5_MIN_ENC_PIC_WIDTH,
			.max_height = W5_MAX_ENC_PIC_HEIGHT,
			.min_height = W5_MIN_ENC_PIC_HEIGHT,
		},
		{
			.v4l2_pix_fmt = V4L2_PIX_FMT_NV21M,
			.max_width = W5_MAX_ENC_PIC_WIDTH,
			.min_width = W5_MIN_ENC_PIC_WIDTH,
			.max_height = W5_MAX_ENC_PIC_HEIGHT,
			.min_height = W5_MIN_ENC_PIC_HEIGHT,
		},
	}
};

static enum wave_std wave5_to_vpu_wavestd(unsigned int v4l2_pix_fmt)
{
	switch (v4l2_pix_fmt) {
	case V4L2_PIX_FMT_H264:
		return W_AVC_ENC;
	case V4L2_PIX_FMT_HEVC:
		return W_HEVC_ENC;
	default:
		return STD_UNKNOWN;
	}
}

static struct vb2_v4l2_buffer *wave5_get_valid_src_buf(struct vpu_instance *inst)
{
	struct v4l2_m2m_buffer *v4l2_m2m_buf;

	v4l2_m2m_for_each_src_buf(inst->v4l2_fh.m2m_ctx, v4l2_m2m_buf) {
		struct vb2_v4l2_buffer *vb2_v4l2_buf;
		struct vpu_buffer *vpu_buf = NULL;

		vb2_v4l2_buf = &v4l2_m2m_buf->vb;
		vpu_buf = wave5_to_vpu_buf(vb2_v4l2_buf);

		if (!vpu_buf->consumed) {
			dev_dbg(inst->dev->dev, "%s: src buf (index: %d) has not been consumed\n",
				__func__, vb2_v4l2_buf->vb2_buf.index);
			return vb2_v4l2_buf;
		}
	}

	return NULL;
}

static struct vb2_v4l2_buffer *wave5_get_valid_dst_buf(struct vpu_instance *inst)
{
	struct v4l2_m2m_buffer *v4l2_m2m_buf;

	v4l2_m2m_for_each_dst_buf(inst->v4l2_fh.m2m_ctx, v4l2_m2m_buf) {
		struct vb2_v4l2_buffer *vb2_v4l2_buf;
		struct vpu_buffer *vpu_buf = NULL;

		vb2_v4l2_buf = &v4l2_m2m_buf->vb;
		vpu_buf = wave5_to_vpu_buf(vb2_v4l2_buf);

		if (!vpu_buf->consumed) {
			dev_dbg(inst->dev->dev, "%s: dst buf (index: %d) has not been consumed\n",
				__func__, vb2_v4l2_buf->vb2_buf.index);
			return vb2_v4l2_buf;
		}
	}

	return NULL;
}

static void wave5_update_pix_fmt(struct v4l2_pix_format_mplane *pix_mp, unsigned int width,
				 unsigned int height)
{
	switch (pix_mp->pixelformat) {
	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_NV12:
	case V4L2_PIX_FMT_NV21:
		pix_mp->width = width;
		pix_mp->height = height;
		pix_mp->plane_fmt[0].bytesperline = round_up(width, 32);
		pix_mp->plane_fmt[0].sizeimage = round_up(width, 32) * height * 3 / 2;
		break;
	case V4L2_PIX_FMT_YUV420M:
		pix_mp->width = width;
		pix_mp->height = height;
		pix_mp->plane_fmt[0].bytesperline = round_up(width, 32);
		pix_mp->plane_fmt[0].sizeimage = round_up(width, 32) * height;
		pix_mp->plane_fmt[1].bytesperline = round_up(width, 32) / 2;
		pix_mp->plane_fmt[1].sizeimage = round_up(width, 32) * height / 4;
		pix_mp->plane_fmt[2].bytesperline = round_up(width, 32) / 2;
		pix_mp->plane_fmt[2].sizeimage = round_up(width, 32) * height / 4;
		break;
	case V4L2_PIX_FMT_NV12M:
	case V4L2_PIX_FMT_NV21M:
		pix_mp->width = width;
		pix_mp->height = height;
		pix_mp->plane_fmt[0].bytesperline = round_up(width, 32);
		pix_mp->plane_fmt[0].sizeimage = round_up(width, 32) * height;
		pix_mp->plane_fmt[1].bytesperline = round_up(width, 32);
		pix_mp->plane_fmt[1].sizeimage = round_up(width, 32) * height / 2;
		break;
	default:
		pix_mp->width = width;
		pix_mp->height = height;
		pix_mp->plane_fmt[0].bytesperline = 0;
		pix_mp->plane_fmt[0].sizeimage = width * height;
		break;
	}
}

static void wave5_vpu_enc_start_encode(struct vpu_instance *inst)
{
	u32 max_cmd_q = 0;

	max_cmd_q = (inst->src_buf_count < COMMAND_QUEUE_DEPTH) ?
		inst->src_buf_count : COMMAND_QUEUE_DEPTH;

	if (inst->state == VPU_INST_STATE_STOP)
		max_cmd_q = 1;

	while (max_cmd_q) {
		struct vb2_v4l2_buffer *src_buf;
		struct vb2_v4l2_buffer *dst_buf;
		struct vpu_buffer *src_vbuf;
		struct vpu_buffer *dst_vbuf;
		struct frame_buffer frame_buf;
		struct enc_param pic_param;
		u32 stride = ALIGN(inst->dst_fmt.width, 32);
		u32 luma_size = (stride * inst->dst_fmt.height);
		u32 chroma_size = ((stride / 2) * (inst->dst_fmt.height / 2));
		u32 fail_res;
		int ret;

		memset(&pic_param, 0, sizeof(struct enc_param));
		memset(&frame_buf, 0, sizeof(struct frame_buffer));

		src_buf = wave5_get_valid_src_buf(inst);
		dst_buf = wave5_get_valid_dst_buf(inst);

		if (!dst_buf) {
			dev_dbg(inst->dev->dev, "%s: No valid dst buf\n", __func__);
			break;
		}

		dst_vbuf = wave5_to_vpu_buf(dst_buf);
		pic_param.pic_stream_buffer_addr =
			vb2_dma_contig_plane_dma_addr(&dst_buf->vb2_buf, 0);
		pic_param.pic_stream_buffer_size =
			vb2_plane_size(&dst_buf->vb2_buf, 0);

		if (!src_buf) {
			dev_dbg(inst->dev->dev, "%s: No valid src buf\n", __func__);
			if (inst->state == VPU_INST_STATE_STOP)
				pic_param.src_end_flag = true;
			else
				break;
		} else {
			src_vbuf = wave5_to_vpu_buf(src_buf);
			if (inst->src_fmt.num_planes == 1) {
				frame_buf.buf_y =
					vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, 0);
				frame_buf.buf_cb = frame_buf.buf_y + luma_size;
				frame_buf.buf_cr = frame_buf.buf_cb + chroma_size;
			} else if (inst->src_fmt.num_planes == 2) {
				frame_buf.buf_y =
					vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, 0);
				frame_buf.buf_cb =
					vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, 1);
				frame_buf.buf_cr = frame_buf.buf_cb + chroma_size;
			} else if (inst->src_fmt.num_planes == 3) {
				frame_buf.buf_y =
					vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, 0);
				frame_buf.buf_cb =
					vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, 1);
				frame_buf.buf_cr =
					vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, 2);
			}
			frame_buf.stride = stride;
			pic_param.src_idx = src_buf->vb2_buf.index;
		}

		pic_param.source_frame = &frame_buf;
		pic_param.code_option.implicit_header_encode = 1;
		ret = wave5_vpu_enc_start_one_frame(inst, &pic_param, &fail_res);
		if (ret) {
			if (fail_res == WAVE5_SYSERR_QUEUEING_FAIL)
				break;

			dev_dbg(inst->dev->dev, "%s: wave5_vpu_enc_start_one_frame fail: %d\n",
				__func__, ret);
			src_buf = v4l2_m2m_src_buf_remove(inst->v4l2_fh.m2m_ctx);
			if (!src_buf) {
				dev_err(inst->dev->dev,
					"Removing src buf failed, the queue is empty\n");
				continue;
			}
			dst_buf = v4l2_m2m_dst_buf_remove(inst->v4l2_fh.m2m_ctx);
			if (!dst_buf) {
				dev_err(inst->dev->dev,
					"Removing dst buf failed, the queue is empty\n");
				continue;
			}
			inst->state = VPU_INST_STATE_STOP;
			v4l2_m2m_buf_done(src_buf, VB2_BUF_STATE_ERROR);
			v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_ERROR);
		} else {
			dev_dbg(inst->dev->dev, "%s: wave5_vpu_enc_start_one_frame success\n",
				__func__);
			if (src_buf)
				src_vbuf->consumed = true;
			if (dst_buf)
				dst_vbuf->consumed = true;
		}

		max_cmd_q--;
	}
}

static void wave5_vpu_enc_stop_encode(struct vpu_instance *inst)
{
	inst->state = VPU_INST_STATE_STOP;

	v4l2_m2m_job_finish(inst->v4l2_m2m_dev, inst->v4l2_fh.m2m_ctx);
}

static void wave5_vpu_enc_finish_encode(struct vpu_instance *inst)
{
	int ret;
	struct enc_output_info enc_output_info;
	u32 irq_status;
	struct vb2_v4l2_buffer *dst_buf = NULL;
	struct v4l2_m2m_buffer *v4l2_m2m_buf = NULL;

	if (kfifo_out(&inst->irq_status, &irq_status, sizeof(int)))
		wave5_vpu_clear_interrupt_ex(inst, irq_status);

	ret = wave5_vpu_enc_get_output_info(inst, &enc_output_info);
	if (ret) {
		dev_dbg(inst->dev->dev,
			"%s: vpu_enc_get_output_info fail: %d  reason: %d | info: %d\n",
			__func__, ret, enc_output_info.error_reason, enc_output_info.warn_info);
		return;
	}

	v4l2_m2m_for_each_dst_buf(inst->v4l2_fh.m2m_ctx, v4l2_m2m_buf) {
		dst_buf = &v4l2_m2m_buf->vb;
		if (enc_output_info.bitstream_buffer ==
			vb2_dma_contig_plane_dma_addr(&dst_buf->vb2_buf, 0))
			break;
	}

	if (enc_output_info.enc_src_idx >= 0) {
		struct vb2_v4l2_buffer *src_buf =
			v4l2_m2m_src_buf_remove_by_idx(inst->v4l2_fh.m2m_ctx,
						       enc_output_info.enc_src_idx);

		inst->timestamp = src_buf->vb2_buf.timestamp;
		v4l2_m2m_buf_done(src_buf, VB2_BUF_STATE_DONE);
	}

	if (enc_output_info.recon_frame_index == RECON_IDX_FLAG_ENC_END) {
		static const struct v4l2_event vpu_event_eos = {
			.type = V4L2_EVENT_EOS
		};

		vb2_set_plane_payload(&dst_buf->vb2_buf, 0, 0);
		dst_buf->vb2_buf.timestamp = inst->timestamp;
		dst_buf->field = V4L2_FIELD_NONE;
		dst_buf->flags |= V4L2_BUF_FLAG_LAST;
		v4l2_m2m_dst_buf_remove_by_buf(inst->v4l2_fh.m2m_ctx, dst_buf);
		v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_DONE);

		inst->state = VPU_INST_STATE_PIC_RUN;
		v4l2_event_queue_fh(&inst->v4l2_fh, &vpu_event_eos);

		v4l2_m2m_job_finish(inst->v4l2_m2m_dev, inst->v4l2_fh.m2m_ctx);
	} else {
		vb2_set_plane_payload(&dst_buf->vb2_buf, 0, enc_output_info.bitstream_size);

		dst_buf->vb2_buf.timestamp = inst->timestamp;
		dst_buf->field = V4L2_FIELD_NONE;
		if (enc_output_info.pic_type == PIC_TYPE_I) {
			if (enc_output_info.enc_vcl_nut == 19 ||
			    enc_output_info.enc_vcl_nut == 20)
				dst_buf->flags |= V4L2_BUF_FLAG_KEYFRAME;
			else
				dst_buf->flags |= V4L2_BUF_FLAG_PFRAME;
		} else if (enc_output_info.pic_type == PIC_TYPE_P) {
			dst_buf->flags |= V4L2_BUF_FLAG_PFRAME;
		} else if (enc_output_info.pic_type == PIC_TYPE_B) {
			dst_buf->flags |= V4L2_BUF_FLAG_BFRAME;
		}

		v4l2_m2m_dst_buf_remove_by_buf(inst->v4l2_fh.m2m_ctx, dst_buf);
		v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_DONE);

		dev_dbg(inst->dev->dev, "%s: frame_cycle %8d\n",
			__func__, enc_output_info.frame_cycle);
	}
}

static int wave5_vpu_enc_querycap(struct file *file, void *fh, struct v4l2_capability *cap)
{
	strscpy(cap->driver, VPU_ENC_DRV_NAME, sizeof(cap->driver));
	strscpy(cap->card, VPU_ENC_DRV_NAME, sizeof(cap->card));
	strscpy(cap->bus_info, "platform:" VPU_ENC_DRV_NAME, sizeof(cap->bus_info));

	return 0;
}

static int wave5_vpu_enc_enum_framesizes(struct file *f, void *fh, struct v4l2_frmsizeenum *fsize)
{
	const struct vpu_format *vpu_fmt;

	if (fsize->index)
		return -EINVAL;

	vpu_fmt = wave5_find_vpu_fmt(fsize->pixel_format, enc_fmt_list[VPU_FMT_TYPE_CODEC]);
	if (!vpu_fmt) {
		vpu_fmt = wave5_find_vpu_fmt(fsize->pixel_format, enc_fmt_list[VPU_FMT_TYPE_RAW]);
		if (!vpu_fmt)
			return -EINVAL;
	}

	fsize->type = V4L2_FRMSIZE_TYPE_CONTINUOUS;
	fsize->stepwise.min_width = vpu_fmt->min_width;
	fsize->stepwise.max_width = vpu_fmt->max_width;
	fsize->stepwise.step_width = 1;
	fsize->stepwise.min_height = vpu_fmt->min_height;
	fsize->stepwise.max_height = vpu_fmt->max_height;
	fsize->stepwise.step_height = 1;

	return 0;
}

static int wave5_vpu_enc_enum_fmt_cap(struct file *file, void *fh, struct v4l2_fmtdesc *f)
{
	struct vpu_instance *inst = wave5_to_vpu_inst(fh);
	const struct vpu_format *vpu_fmt;

	dev_dbg(inst->dev->dev, "%s: index: %d\n", __func__, f->index);

	vpu_fmt = wave5_find_vpu_fmt_by_idx(f->index, enc_fmt_list[VPU_FMT_TYPE_CODEC]);
	if (!vpu_fmt)
		return -EINVAL;

	f->pixelformat = vpu_fmt->v4l2_pix_fmt;
	f->flags = 0;

	return 0;
}

static int wave5_vpu_enc_try_fmt_cap(struct file *file, void *fh, struct v4l2_format *f)
{
	struct vpu_instance *inst = wave5_to_vpu_inst(fh);
	const struct vpu_format *vpu_fmt;

	dev_dbg(inst->dev->dev, "%s: fourcc: %d width: %d height: %d num_planes: %d field: %d\n",
		__func__, f->fmt.pix_mp.pixelformat, f->fmt.pix_mp.width, f->fmt.pix_mp.height,
		f->fmt.pix_mp.num_planes, f->fmt.pix_mp.field);

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return -EINVAL;

	vpu_fmt = wave5_find_vpu_fmt(f->fmt.pix_mp.pixelformat, enc_fmt_list[VPU_FMT_TYPE_CODEC]);
	if (!vpu_fmt) {
		f->fmt.pix_mp.pixelformat = inst->dst_fmt.pixelformat;
		f->fmt.pix_mp.num_planes = inst->dst_fmt.num_planes;
		wave5_update_pix_fmt(&f->fmt.pix_mp, inst->dst_fmt.width, inst->dst_fmt.height);
	} else {
		int width = clamp(f->fmt.pix_mp.width, vpu_fmt->min_width, vpu_fmt->max_width);
		int height = clamp(f->fmt.pix_mp.height, vpu_fmt->min_height, vpu_fmt->max_height);

		f->fmt.pix_mp.pixelformat = vpu_fmt->v4l2_pix_fmt;
		f->fmt.pix_mp.num_planes = 1;
		wave5_update_pix_fmt(&f->fmt.pix_mp, width, height);
	}

	f->fmt.pix_mp.flags = 0;
	f->fmt.pix_mp.field = V4L2_FIELD_NONE;
	f->fmt.pix_mp.colorspace = inst->colorspace;
	f->fmt.pix_mp.ycbcr_enc = inst->ycbcr_enc;
	f->fmt.pix_mp.hsv_enc = inst->hsv_enc;
	f->fmt.pix_mp.quantization = inst->quantization;
	f->fmt.pix_mp.xfer_func = inst->xfer_func;
	memset(&f->fmt.pix_mp.reserved, 0, sizeof(f->fmt.pix_mp.reserved));

	return 0;
}

static int wave5_vpu_enc_s_fmt_cap(struct file *file, void *fh, struct v4l2_format *f)
{
	struct vpu_instance *inst = wave5_to_vpu_inst(fh);
	int i, ret;

	dev_dbg(inst->dev->dev, "%s: fourcc: %d width: %d height: %d num_planes: %d field: %d\n",
		__func__, f->fmt.pix_mp.pixelformat, f->fmt.pix_mp.width, f->fmt.pix_mp.height,
		f->fmt.pix_mp.num_planes, f->fmt.pix_mp.field);

	ret = wave5_vpu_enc_try_fmt_cap(file, fh, f);
	if (ret)
		return ret;

	inst->dst_fmt.width = f->fmt.pix_mp.width;
	inst->dst_fmt.height = f->fmt.pix_mp.height;
	inst->dst_fmt.pixelformat = f->fmt.pix_mp.pixelformat;
	inst->dst_fmt.field = f->fmt.pix_mp.field;
	inst->dst_fmt.flags = f->fmt.pix_mp.flags;
	inst->dst_fmt.num_planes = f->fmt.pix_mp.num_planes;
	for (i = 0; i < inst->dst_fmt.num_planes; i++) {
		inst->dst_fmt.plane_fmt[i].bytesperline = f->fmt.pix_mp.plane_fmt[i].bytesperline;
		inst->dst_fmt.plane_fmt[i].sizeimage = f->fmt.pix_mp.plane_fmt[i].sizeimage;
	}

	return 0;
}

static int wave5_vpu_enc_g_fmt_cap(struct file *file, void *fh, struct v4l2_format *f)
{
	struct vpu_instance *inst = wave5_to_vpu_inst(fh);
	int i;

	f->fmt.pix_mp.width = inst->dst_fmt.width;
	f->fmt.pix_mp.height = inst->dst_fmt.height;
	f->fmt.pix_mp.pixelformat = inst->dst_fmt.pixelformat;
	f->fmt.pix_mp.field = inst->dst_fmt.field;
	f->fmt.pix_mp.flags = inst->dst_fmt.flags;
	f->fmt.pix_mp.num_planes = inst->dst_fmt.num_planes;
	for (i = 0; i < f->fmt.pix_mp.num_planes; i++) {
		f->fmt.pix_mp.plane_fmt[i].bytesperline = inst->dst_fmt.plane_fmt[i].bytesperline;
		f->fmt.pix_mp.plane_fmt[i].sizeimage = inst->dst_fmt.plane_fmt[i].sizeimage;
	}

	f->fmt.pix_mp.colorspace = inst->colorspace;
	f->fmt.pix_mp.ycbcr_enc = inst->ycbcr_enc;
	f->fmt.pix_mp.hsv_enc = inst->hsv_enc;
	f->fmt.pix_mp.quantization = inst->quantization;
	f->fmt.pix_mp.xfer_func = inst->xfer_func;

	return 0;
}

static int wave5_vpu_enc_enum_fmt_out(struct file *file, void *fh, struct v4l2_fmtdesc *f)
{
	struct vpu_instance *inst = wave5_to_vpu_inst(fh);
	const struct vpu_format *vpu_fmt;

	dev_dbg(inst->dev->dev, "%s: index: %d\n", __func__, f->index);

	vpu_fmt = wave5_find_vpu_fmt_by_idx(f->index, enc_fmt_list[VPU_FMT_TYPE_RAW]);
	if (!vpu_fmt)
		return -EINVAL;

	f->pixelformat = vpu_fmt->v4l2_pix_fmt;
	f->flags = 0;

	return 0;
}

static int wave5_vpu_enc_try_fmt_out(struct file *file, void *fh, struct v4l2_format *f)
{
	struct vpu_instance *inst = wave5_to_vpu_inst(fh);
	const struct vpu_format *vpu_fmt;

	dev_dbg(inst->dev->dev, "%s: fourcc: %d width: %d height: %d num_planes: %d field: %d\n",
		__func__, f->fmt.pix_mp.pixelformat, f->fmt.pix_mp.width, f->fmt.pix_mp.height,
		f->fmt.pix_mp.num_planes, f->fmt.pix_mp.field);

	if (f->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return -EINVAL;

	vpu_fmt = wave5_find_vpu_fmt(f->fmt.pix_mp.pixelformat, enc_fmt_list[VPU_FMT_TYPE_RAW]);
	if (!vpu_fmt) {
		f->fmt.pix_mp.pixelformat = inst->src_fmt.pixelformat;
		f->fmt.pix_mp.num_planes = inst->src_fmt.num_planes;
		wave5_update_pix_fmt(&f->fmt.pix_mp, inst->src_fmt.width, inst->src_fmt.height);
	} else {
		int width = clamp(f->fmt.pix_mp.width, vpu_fmt->min_width, vpu_fmt->max_width);
		int height = clamp(f->fmt.pix_mp.height, vpu_fmt->min_height, vpu_fmt->max_height);
		const struct v4l2_format_info *info = v4l2_format_info(vpu_fmt->v4l2_pix_fmt);

		f->fmt.pix_mp.pixelformat = vpu_fmt->v4l2_pix_fmt;
		f->fmt.pix_mp.num_planes = info->mem_planes;
		wave5_update_pix_fmt(&f->fmt.pix_mp, width, height);
	}

	f->fmt.pix_mp.flags = 0;
	f->fmt.pix_mp.field = V4L2_FIELD_NONE;
	memset(&f->fmt.pix_mp.reserved, 0, sizeof(f->fmt.pix_mp.reserved));

	return 0;
}

static int wave5_vpu_enc_s_fmt_out(struct file *file, void *fh, struct v4l2_format *f)
{
	struct vpu_instance *inst = wave5_to_vpu_inst(fh);
	int i, ret;

	dev_dbg(inst->dev->dev, "%s: fourcc: %d width: %d height: %d num_planes: %d field: %d\n",
		__func__, f->fmt.pix_mp.pixelformat, f->fmt.pix_mp.width, f->fmt.pix_mp.height,
		f->fmt.pix_mp.num_planes, f->fmt.pix_mp.field);

	ret = wave5_vpu_enc_try_fmt_out(file, fh, f);
	if (ret)
		return ret;

	inst->src_fmt.width = f->fmt.pix_mp.width;
	inst->src_fmt.height = f->fmt.pix_mp.height;
	inst->src_fmt.pixelformat = f->fmt.pix_mp.pixelformat;
	inst->src_fmt.field = f->fmt.pix_mp.field;
	inst->src_fmt.flags = f->fmt.pix_mp.flags;
	inst->src_fmt.num_planes = f->fmt.pix_mp.num_planes;
	for (i = 0; i < inst->src_fmt.num_planes; i++) {
		inst->src_fmt.plane_fmt[i].bytesperline = f->fmt.pix_mp.plane_fmt[i].bytesperline;
		inst->src_fmt.plane_fmt[i].sizeimage = f->fmt.pix_mp.plane_fmt[i].sizeimage;
	}

	if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV12 ||
	    inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV12M) {
		inst->cbcr_interleave = true;
		inst->nv21 = false;
	} else if (inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV21 ||
		   inst->src_fmt.pixelformat == V4L2_PIX_FMT_NV21M) {
		inst->cbcr_interleave = true;
		inst->nv21 = true;
	} else {
		inst->cbcr_interleave = false;
		inst->nv21 = false;
	}

	inst->colorspace = f->fmt.pix_mp.colorspace;
	inst->ycbcr_enc = f->fmt.pix_mp.ycbcr_enc;
	inst->hsv_enc = f->fmt.pix_mp.hsv_enc;
	inst->quantization = f->fmt.pix_mp.quantization;
	inst->xfer_func = f->fmt.pix_mp.xfer_func;

	wave5_update_pix_fmt(&inst->dst_fmt, f->fmt.pix_mp.width, f->fmt.pix_mp.height);

	return 0;
}

static int wave5_vpu_enc_g_selection(struct file *file, void *fh, struct v4l2_selection *s)
{
	struct vpu_instance *inst = wave5_to_vpu_inst(fh);

	dev_dbg(inst->dev->dev, "%s: type: %d | target: %d\n", __func__, s->type, s->target);

	if (s->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		switch (s->target) {
		case V4L2_SEL_TGT_CROP_DEFAULT:
		case V4L2_SEL_TGT_CROP_BOUNDS:
			s->r.left = 0;
			s->r.top = 0;
			s->r.width = inst->dst_fmt.width;
			s->r.height = inst->dst_fmt.height;
			break;
		case V4L2_SEL_TGT_CROP:
			s->r.left = 0;
			s->r.top = 0;
			s->r.width = inst->dst_fmt.width;
			s->r.height = inst->dst_fmt.height;
			dev_dbg(inst->dev->dev, "%s: V4L2_SEL_TGT_CROP width: %d | height: %d\n",
				__func__, s->r.width, s->r.height);
			break;
		default:
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static int wave5_vpu_enc_s_selection(struct file *file, void *fh, struct v4l2_selection *s)
{
	struct vpu_instance *inst = wave5_to_vpu_inst(fh);

	if (s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT)
		return -EINVAL;

	if (s->target != V4L2_SEL_TGT_CROP)
		return -EINVAL;

	dev_dbg(inst->dev->dev, "%s: V4L2_SEL_TGT_CROP width: %d | height: %d\n",
		__func__, s->r.width, s->r.height);

	s->r.left = 0;
	s->r.top = 0;
	s->r.width = inst->src_fmt.width;
	s->r.height = inst->src_fmt.height;

	return 0;
}

static int wave5_vpu_enc_encoder_cmd(struct file *file, void *fh, struct v4l2_encoder_cmd *ec)
{
	struct vpu_instance *inst = wave5_to_vpu_inst(fh);
	int ret;

	ret = v4l2_m2m_ioctl_try_encoder_cmd(file, fh, ec);
	if (ret)
		return ret;

	if (!wave5_vpu_both_queues_are_streaming(inst))
		return 0;

	switch (ec->cmd) {
	case V4L2_ENC_CMD_STOP:
		inst->state = VPU_INST_STATE_STOP;
		inst->ops->start_process(inst);
		break;
	case V4L2_ENC_CMD_START:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int wave5_vpu_enc_g_parm(struct file *file, void *fh, struct v4l2_streamparm *a)
{
	struct vpu_instance *inst = wave5_to_vpu_inst(fh);

	dev_dbg(inst->dev->dev, "%s: type: %d\n", __func__, a->type);

	if (a->type != V4L2_BUF_TYPE_VIDEO_OUTPUT && a->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return -EINVAL;

	a->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
	a->parm.output.timeperframe.numerator = 1;
	a->parm.output.timeperframe.denominator = inst->frame_rate;

	dev_dbg(inst->dev->dev, "%s: numerator: %d | denominator: %d\n",
		__func__, a->parm.output.timeperframe.numerator,
		a->parm.output.timeperframe.denominator);

	return 0;
}

static int wave5_vpu_enc_s_parm(struct file *file, void *fh, struct v4l2_streamparm *a)
{
	struct vpu_instance *inst = wave5_to_vpu_inst(fh);

	dev_dbg(inst->dev->dev, "%s: type: %d\n", __func__, a->type);

	if (a->type != V4L2_BUF_TYPE_VIDEO_OUTPUT && a->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return -EINVAL;

	a->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
	if (a->parm.output.timeperframe.denominator && a->parm.output.timeperframe.numerator) {
		inst->frame_rate = a->parm.output.timeperframe.denominator /
				   a->parm.output.timeperframe.numerator;
	} else {
		a->parm.output.timeperframe.numerator = 1;
		a->parm.output.timeperframe.denominator = inst->frame_rate;
	}

	dev_dbg(inst->dev->dev, "%s: numerator: %d | denominator: %d\n",
		__func__, a->parm.output.timeperframe.numerator,
		a->parm.output.timeperframe.denominator);

	return 0;
}

static const struct v4l2_ioctl_ops wave5_vpu_enc_ioctl_ops = {
	.vidioc_querycap = wave5_vpu_enc_querycap,
	.vidioc_enum_framesizes = wave5_vpu_enc_enum_framesizes,

	.vidioc_enum_fmt_vid_cap	= wave5_vpu_enc_enum_fmt_cap,
	.vidioc_s_fmt_vid_cap_mplane = wave5_vpu_enc_s_fmt_cap,
	.vidioc_g_fmt_vid_cap_mplane = wave5_vpu_enc_g_fmt_cap,
	.vidioc_try_fmt_vid_cap_mplane = wave5_vpu_enc_try_fmt_cap,

	.vidioc_enum_fmt_vid_out	= wave5_vpu_enc_enum_fmt_out,
	.vidioc_s_fmt_vid_out_mplane = wave5_vpu_enc_s_fmt_out,
	.vidioc_g_fmt_vid_out_mplane = wave5_vpu_g_fmt_out,
	.vidioc_try_fmt_vid_out_mplane = wave5_vpu_enc_try_fmt_out,

	.vidioc_g_selection = wave5_vpu_enc_g_selection,
	.vidioc_s_selection = wave5_vpu_enc_s_selection,

	.vidioc_g_parm = wave5_vpu_enc_g_parm,
	.vidioc_s_parm = wave5_vpu_enc_s_parm,

	.vidioc_reqbufs = v4l2_m2m_ioctl_reqbufs,
	.vidioc_querybuf = v4l2_m2m_ioctl_querybuf,
	.vidioc_create_bufs = v4l2_m2m_ioctl_create_bufs,
	.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,
	.vidioc_qbuf = v4l2_m2m_ioctl_qbuf,
	.vidioc_expbuf = v4l2_m2m_ioctl_expbuf,
	.vidioc_dqbuf = v4l2_m2m_ioctl_dqbuf,
	.vidioc_streamon = v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff = v4l2_m2m_ioctl_streamoff,

	.vidioc_try_encoder_cmd = v4l2_m2m_ioctl_try_encoder_cmd,
	.vidioc_encoder_cmd = wave5_vpu_enc_encoder_cmd,

	.vidioc_subscribe_event = wave5_vpu_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static int wave5_vpu_enc_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct vpu_instance *inst = wave5_ctrl_to_vpu_inst(ctrl);

	dev_dbg(inst->dev->dev, "%s: name: %s | value: %d\n", __func__, ctrl->name, ctrl->val);

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		inst->mirror_direction |= (ctrl->val << 1);
		break;
	case V4L2_CID_VFLIP:
		inst->mirror_direction |= ctrl->val;
		break;
	case V4L2_CID_ROTATE:
		inst->rot_angle = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_VBV_SIZE:
		inst->vbv_buf_size = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_PROFILE:
		switch (ctrl->val) {
		case V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN:
			inst->profile = HEVC_PROFILE_MAIN;
			inst->bit_depth = 8;
			break;
		case V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN_STILL_PICTURE:
			inst->profile = HEVC_PROFILE_STILLPICTURE;
			inst->bit_depth = 8;
			break;
		case V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN_10:
			inst->profile = HEVC_PROFILE_MAIN10;
			inst->bit_depth = 10;
			break;
		default:
			return -EINVAL;
		}
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_LEVEL:
		switch (ctrl->val) {
		case V4L2_MPEG_VIDEO_HEVC_LEVEL_1:
			inst->level = 10 * 3;
			break;
		case V4L2_MPEG_VIDEO_HEVC_LEVEL_2:
			inst->level = 20 * 3;
			break;
		case V4L2_MPEG_VIDEO_HEVC_LEVEL_2_1:
			inst->level = 21 * 3;
			break;
		case V4L2_MPEG_VIDEO_HEVC_LEVEL_3:
			inst->level = 30 * 3;
			break;
		case V4L2_MPEG_VIDEO_HEVC_LEVEL_3_1:
			inst->level = 31 * 3;
			break;
		case V4L2_MPEG_VIDEO_HEVC_LEVEL_4:
			inst->level = 40 * 3;
			break;
		case V4L2_MPEG_VIDEO_HEVC_LEVEL_4_1:
			inst->level = 41 * 3;
			break;
		case V4L2_MPEG_VIDEO_HEVC_LEVEL_5:
			inst->level = 50 * 3;
			break;
		case V4L2_MPEG_VIDEO_HEVC_LEVEL_5_1:
			inst->level = 51 * 3;
			break;
		case V4L2_MPEG_VIDEO_HEVC_LEVEL_5_2:
			inst->level = 52 * 3;
			break;
		default:
			return -EINVAL;
		}
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP:
		inst->min_qp_i = ctrl->val;
		inst->min_qp_p = ctrl->val;
		inst->min_qp_b = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP:
		inst->max_qp_i = ctrl->val;
		inst->max_qp_p = ctrl->val;
		inst->max_qp_b = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_PROFILE:
		switch (ctrl->val) {
		case V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE:
		case V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE:
			inst->profile = H264_PROFILE_BP;
			inst->bit_depth = 8;
			break;
		case V4L2_MPEG_VIDEO_H264_PROFILE_MAIN:
			inst->profile = H264_PROFILE_MP;
			inst->bit_depth = 8;
			break;
		case V4L2_MPEG_VIDEO_H264_PROFILE_EXTENDED:
			inst->profile = H264_PROFILE_EXTENDED;
			inst->bit_depth = 8;
			break;
		case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH:
			inst->profile = H264_PROFILE_HP;
			inst->bit_depth = 8;
			break;
		case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_10:
			inst->profile = H264_PROFILE_HIGH10;
			inst->bit_depth = 10;
			break;
		case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_422:
			inst->profile = H264_PROFILE_HIGH422;
			inst->bit_depth = 10;
			break;
		case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_444_PREDICTIVE:
			inst->profile = H264_PROFILE_HIGH444;
			inst->bit_depth = 10;
			break;
		default:
			return -EINVAL;
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_LEVEL:
		switch (ctrl->val) {
		case V4L2_MPEG_VIDEO_H264_LEVEL_1_0:
			inst->level = 10;
			break;
		case V4L2_MPEG_VIDEO_H264_LEVEL_1B:
			inst->level = 9;
			break;
		case V4L2_MPEG_VIDEO_H264_LEVEL_1_1:
			inst->level = 11;
			break;
		case V4L2_MPEG_VIDEO_H264_LEVEL_1_2:
			inst->level = 12;
			break;
		case V4L2_MPEG_VIDEO_H264_LEVEL_1_3:
			inst->level = 13;
			break;
		case V4L2_MPEG_VIDEO_H264_LEVEL_2_0:
			inst->level = 20;
			break;
		case V4L2_MPEG_VIDEO_H264_LEVEL_2_1:
			inst->level = 21;
			break;
		case V4L2_MPEG_VIDEO_H264_LEVEL_2_2:
			inst->level = 22;
			break;
		case V4L2_MPEG_VIDEO_H264_LEVEL_3_0:
			inst->level = 30;
			break;
		case V4L2_MPEG_VIDEO_H264_LEVEL_3_1:
			inst->level = 31;
			break;
		case V4L2_MPEG_VIDEO_H264_LEVEL_3_2:
			inst->level = 32;
			break;
		case V4L2_MPEG_VIDEO_H264_LEVEL_4_0:
			inst->level = 40;
			break;
		case V4L2_MPEG_VIDEO_H264_LEVEL_4_1:
			inst->level = 41;
			break;
		case V4L2_MPEG_VIDEO_H264_LEVEL_4_2:
			inst->level = 42;
			break;
		case V4L2_MPEG_VIDEO_H264_LEVEL_5_0:
			inst->level = 50;
			break;
		case V4L2_MPEG_VIDEO_H264_LEVEL_5_1:
			inst->level = 51;
			break;
		default:
			return -EINVAL;
		}
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MIN_QP:
		inst->min_qp_i = ctrl->val;
		inst->min_qp_p = ctrl->val;
		inst->min_qp_b = ctrl->val;
		break;
	case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
		inst->max_qp_i = ctrl->val;
		inst->max_qp_p = ctrl->val;
		inst->max_qp_b = ctrl->val;
		break;
	case V4L2_CID_MIN_BUFFERS_FOR_OUTPUT:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct v4l2_ctrl_ops wave5_vpu_enc_ctrl_ops = {
	.s_ctrl = wave5_vpu_enc_s_ctrl,
};

static void wave5_set_default_enc_openparam(struct enc_open_param *open_param)
{
	unsigned int i;

	open_param->stream_endian = VPU_STREAM_ENDIAN;
	open_param->source_endian = VPU_SOURCE_ENDIAN;
	open_param->line_buf_int_en = true;

	open_param->wave_param.gop_preset_idx = PRESET_IDX_IPP_SINGLE;
	open_param->wave_param.decoding_refresh_type = DEC_REFRESH_TYPE_NON_IRAP;
	open_param->wave_param.intra_qp = 30;
	open_param->wave_param.tmvp_enable = 1;
	open_param->wave_param.max_num_merge = 2;
	open_param->wave_param.lf_cross_slice_boundary_enable = 1;
	open_param->wave_param.skip_intra_trans = 1;
	open_param->wave_param.sao_enable = 1;
	open_param->wave_param.transform8x8_enable = 1;
	open_param->wave_param.intra_nx_n_enable = 1;
	for (i = 0; i < MAX_GOP_NUM; i++)
		open_param->wave_param.fixed_bit_ratio[i] = 1;
	open_param->wave_param.cu_level_rc_enable = 1;
	open_param->wave_param.hvs_qp_enable = 1;
	open_param->wave_param.hvs_qp_scale = 2;
	open_param->wave_param.hvs_max_delta_qp = 10;
	open_param->wave_param.gop_param.custom_gop_size = 1;
	open_param->wave_param.initial_rc_qp = -1;
	open_param->wave_param.nr_intra_weight_y = 7;
	open_param->wave_param.nr_intra_weight_cb = 7;
	open_param->wave_param.nr_intra_weight_cr = 7;
	open_param->wave_param.nr_inter_weight_y = 4;
	open_param->wave_param.nr_inter_weight_cb = 4;
	open_param->wave_param.nr_inter_weight_cr = 4;
	open_param->wave_param.strong_intra_smooth_enable = 1;
	open_param->wave_param.bg_thr_diff = 8;
	open_param->wave_param.bg_thr_mean_diff = 1;
	open_param->wave_param.bg_lambda_qp = 32;
	open_param->wave_param.bg_delta_qp = 3;
	open_param->wave_param.rdo_skip = 1;
	open_param->wave_param.intra_mb_refresh_arg = 1;
	open_param->wave_param.entropy_coding_mode = 1;
	open_param->wave_param.rc_weight_param = 16;
	open_param->wave_param.rc_weight_buf = 128;
	open_param->wave_param.lambda_scaling_enable = 1;
}

static int wave5_vpu_enc_queue_setup(struct vb2_queue *q, unsigned int *num_buffers,
				     unsigned int *num_planes, unsigned int sizes[],
				     struct device *alloc_devs[])
{
	struct vpu_instance *inst = vb2_get_drv_priv(q);
	struct v4l2_pix_format_mplane inst_format =
		(q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ? inst->src_fmt : inst->dst_fmt;
	unsigned int i;
	int ret;

	dev_dbg(inst->dev->dev, "%s: num_buffers: %d | num_planes: %d | type: %d\n", __func__,
		*num_buffers, *num_planes, q->type);

	if (*num_planes) {
		if (inst_format.num_planes != *num_planes)
			return -EINVAL;

		for (i = 0; i < *num_planes; i++) {
			if (sizes[i] < inst_format.plane_fmt[i].sizeimage)
				return -EINVAL;
		}
	} else {
		*num_planes = inst_format.num_planes;
		for (i = 0; i < *num_planes; i++) {
			sizes[i] = inst_format.plane_fmt[i].sizeimage;
			dev_dbg(inst->dev->dev, "%s: size[%d]: %d\n", __func__, i, sizes[i]);
		}
	}

	dev_dbg(inst->dev->dev, "%s: size: %d\n", __func__, sizes[0]);

	if (inst->state == VPU_INST_STATE_NONE && q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		unsigned int non_linear_num = 0;
		u32 fb_stride = 0;
		u32 fb_height = 0;
		struct enc_open_param open_param;
		struct enc_initial_info initial_info;
		struct v4l2_ctrl *ctrl;

		memset(&open_param, 0, sizeof(struct enc_open_param));
		wave5_set_default_enc_openparam(&open_param);

		inst->std = wave5_to_vpu_wavestd(inst->dst_fmt.pixelformat);
		if (inst->std == STD_UNKNOWN) {
			dev_warn(inst->dev->dev, "unsupported pixelformat: %.4s\n",
				 (char *)&inst->dst_fmt.pixelformat);
			return -EINVAL;
		}
		open_param.pic_width = inst->dst_fmt.width;
		open_param.pic_height = inst->dst_fmt.height;
		open_param.frame_rate_info = inst->frame_rate;
		open_param.vbv_buffer_size = inst->vbv_buf_size;
		open_param.wave_param.profile = inst->profile;
		open_param.wave_param.level = inst->level;
		open_param.wave_param.internal_bit_depth = inst->bit_depth;
		open_param.wave_param.min_qp_i = inst->min_qp_i;
		open_param.wave_param.max_qp_i = inst->max_qp_i;
		open_param.wave_param.min_qp_p = inst->min_qp_p;
		open_param.wave_param.max_qp_p = inst->max_qp_p;
		open_param.wave_param.min_qp_b = inst->min_qp_b;
		open_param.wave_param.max_qp_b = inst->max_qp_b;

		ret = wave5_vpu_enc_open(inst, &open_param);
		if (ret) {
			dev_dbg(inst->dev->dev, "%s: wave5_vpu_enc_open, fail: %d\n",
				__func__, ret);
			return ret;
		}

		inst->state = VPU_INST_STATE_OPEN;

		if (inst->mirror_direction) {
			wave5_vpu_enc_give_command(inst, ENABLE_MIRRORING, NULL);
			wave5_vpu_enc_give_command(inst, SET_MIRROR_DIRECTION,
						   &inst->mirror_direction);
		}
		if (inst->rot_angle) {
			wave5_vpu_enc_give_command(inst, ENABLE_ROTATION, NULL);
			wave5_vpu_enc_give_command(inst, SET_ROTATION_ANGLE, &inst->rot_angle);
		}

		ret = wave5_vpu_enc_issue_seq_init(inst);
		if (ret) {
			dev_dbg(inst->dev->dev, "%s: wave5_vpu_enc_issue_seq_init, fail: %d\n",
				__func__, ret);
			return ret;
		}

		if (wave5_vpu_wait_interrupt(inst, VPU_ENC_TIMEOUT) < 0) {
			dev_dbg(inst->dev->dev, "%s: wave5_vpu_wait_interrupt failed\n", __func__);
			return -EINVAL;
		}

		ret = wave5_vpu_enc_complete_seq_init(inst, &initial_info);
		if (ret)
			return ret;

		dev_dbg(inst->dev->dev, "%s: min_frame_buffer: %d | min_source_buffer: %d\n",
			__func__, initial_info.min_frame_buffer_count,
			initial_info.min_src_frame_count);
		inst->state = VPU_INST_STATE_INIT_SEQ;
		inst->min_src_buf_count = initial_info.min_src_frame_count +
					  COMMAND_QUEUE_DEPTH;

		ctrl = v4l2_ctrl_find(&inst->v4l2_ctrl_hdl,
				      V4L2_CID_MIN_BUFFERS_FOR_OUTPUT);
		if (ctrl)
			v4l2_ctrl_s_ctrl(ctrl, inst->min_src_buf_count);

		inst->min_dst_buf_count = initial_info.min_frame_buffer_count;
		inst->src_buf_count = inst->min_src_buf_count;

		if (*num_buffers > inst->src_buf_count)
			inst->src_buf_count = *num_buffers;

		*num_buffers = inst->src_buf_count;
		non_linear_num = inst->min_dst_buf_count;

		fb_stride = ALIGN(inst->dst_fmt.width, 32);
		fb_height = ALIGN(inst->dst_fmt.height, 32);

		for (i = 0; i < non_linear_num; i++) {
			u32 luma_size = fb_stride * fb_height;
			u32 chroma_size = ALIGN(fb_stride / 2, 16) * fb_height;

			inst->frame_vbuf[i].size = luma_size + chroma_size;
			ret = wave5_vdi_allocate_dma_memory(inst->dev, &inst->frame_vbuf[i]);
			if (ret < 0) {
				dev_dbg(inst->dev->dev, "%s: failed to allocate FBC buffer %zu\n",
					__func__, inst->frame_vbuf[i].size);
				goto free_buffers;
			}

			inst->frame_buf[i].buf_y = inst->frame_vbuf[i].daddr;
			inst->frame_buf[i].buf_cb = (dma_addr_t)-1;
			inst->frame_buf[i].buf_cr = (dma_addr_t)-1;
			inst->frame_buf[i].update_fb_info = true;
			inst->frame_buf[i].size = inst->frame_vbuf[i].size;
		}

		ret = wave5_vpu_enc_register_frame_buffer(inst, non_linear_num, fb_stride,
							  fb_height, COMPRESSED_FRAME_MAP);
		if (ret) {
			dev_dbg(inst->dev->dev,
				"%s: wave5_vpu_enc_register_frame_buffer, fail: %d\n",
				__func__, ret);
			goto free_buffers;
		}

		inst->state = VPU_INST_STATE_PIC_RUN;
	}

	if (inst->state == VPU_INST_STATE_INIT_SEQ &&
	    q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		*num_buffers = inst->src_buf_count;
		dev_dbg(inst->dev->dev, "%s: src buf num: %d", __func__, *num_buffers);
	}

	return 0;

free_buffers:
	for (i = 0; i < inst->min_dst_buf_count; i++)
		wave5_vdi_free_dma_memory(inst->dev, &inst->frame_vbuf[i]);
	return ret;
}

static void wave5_vpu_enc_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct vpu_instance *inst = vb2_get_drv_priv(vb->vb2_queue);
	struct vpu_buffer *vpu_buf = wave5_to_vpu_buf(vbuf);

	dev_dbg(inst->dev->dev, "%s: type: %4d index: %4d size: ([0]=%4ld, [1]=%4ld, [2]=%4ld)\n",
		__func__, vb->type, vb->index, vb2_plane_size(&vbuf->vb2_buf, 0),
		vb2_plane_size(&vbuf->vb2_buf, 1), vb2_plane_size(&vbuf->vb2_buf, 2));

	if (vb->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		vbuf->sequence = inst->queued_src_buf_num++;
	else
		vbuf->sequence = inst->queued_dst_buf_num++;

	vpu_buf->consumed = FALSE;
	v4l2_m2m_buf_queue(inst->v4l2_fh.m2m_ctx, vbuf);

	if (vb2_start_streaming_called(vb->vb2_queue))
		inst->ops->start_process(inst);
}

static void wave5_vpu_enc_stop_streaming(struct vb2_queue *q)
{
	struct vpu_instance *inst = vb2_get_drv_priv(q);
	struct vb2_v4l2_buffer *buf;
	bool check_cmd = true;

	dev_dbg(inst->dev->dev, "%s: type: %d\n", __func__, q->type);

	if (wave5_vpu_both_queues_are_streaming(inst))
		inst->state = VPU_INST_STATE_STOP;

	while (check_cmd) {
		struct queue_status_info q_status;
		struct enc_output_info enc_output_info;
		int ret;

		wave5_vpu_enc_give_command(inst, ENC_GET_QUEUE_STATUS, &q_status);

		if (q_status.instance_queue_count + q_status.report_queue_count == 0)
			break;

		if (wave5_vpu_wait_interrupt(inst, VPU_ENC_TIMEOUT) < 0)
			break;

		ret = wave5_vpu_enc_get_output_info(inst, &enc_output_info);
	}

	if (q->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		while ((buf = v4l2_m2m_src_buf_remove(inst->v4l2_fh.m2m_ctx))) {
			dev_dbg(inst->dev->dev, "%s: buf type %4d | index %4d\n",
				__func__, buf->vb2_buf.type, buf->vb2_buf.index);
			v4l2_m2m_buf_done(buf, VB2_BUF_STATE_ERROR);
		}
	} else {
		while ((buf = v4l2_m2m_dst_buf_remove(inst->v4l2_fh.m2m_ctx))) {
			dev_dbg(inst->dev->dev, "%s: buf type %4d | index %4d\n",
				__func__, buf->vb2_buf.type, buf->vb2_buf.index);
			vb2_set_plane_payload(&buf->vb2_buf, 0, 0);
			v4l2_m2m_buf_done(buf, VB2_BUF_STATE_ERROR);
		}
	}
}

static const struct vb2_ops wave5_vpu_enc_vb2_ops = {
	.queue_setup = wave5_vpu_enc_queue_setup,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
	.buf_queue = wave5_vpu_enc_buf_queue,
	.stop_streaming = wave5_vpu_enc_stop_streaming,
};

static void wave5_set_default_format(struct v4l2_pix_format_mplane *src_fmt,
				     struct v4l2_pix_format_mplane *dst_fmt)
{
	unsigned int src_pix_fmt = enc_fmt_list[VPU_FMT_TYPE_RAW][0].v4l2_pix_fmt;
	const struct v4l2_format_info *src_fmt_info = v4l2_format_info(src_pix_fmt);

	src_fmt->pixelformat = src_pix_fmt;
	src_fmt->field = V4L2_FIELD_NONE;
	src_fmt->flags = 0;
	src_fmt->num_planes = src_fmt_info->mem_planes;
	wave5_update_pix_fmt(src_fmt, 416, 240);

	dst_fmt->pixelformat = enc_fmt_list[VPU_FMT_TYPE_CODEC][0].v4l2_pix_fmt;
	dst_fmt->field = V4L2_FIELD_NONE;
	dst_fmt->flags = 0;
	dst_fmt->num_planes = 1;
	wave5_update_pix_fmt(dst_fmt, 416, 240);
}

static int wave5_vpu_enc_queue_init(void *priv, struct vb2_queue *src_vq, struct vb2_queue *dst_vq)
{
	return wave5_vpu_queue_init(priv, src_vq, dst_vq, &wave5_vpu_enc_vb2_ops);
}

static const struct vpu_instance_ops wave5_vpu_enc_inst_ops = {
	.start_process = wave5_vpu_enc_start_encode,
	.stop_process = wave5_vpu_enc_stop_encode,
	.finish_process = wave5_vpu_enc_finish_encode,
};

static void wave5_vpu_enc_device_run(void *priv)
{
	struct vpu_instance *inst = priv;

	inst->ops->start_process(inst);
}

static int wave5_vpu_enc_job_ready(void *priv)
{
	struct vpu_instance *inst = priv;

	if (inst->state == VPU_INST_STATE_STOP)
		return 0;

	return 1;
}

static void wave5_vpu_enc_job_abort(void *priv)
{
	struct vpu_instance *inst = priv;

	inst->ops->stop_process(inst);
}

static const struct v4l2_m2m_ops wave5_vpu_enc_m2m_ops = {
	.device_run = wave5_vpu_enc_device_run,
	.job_ready = wave5_vpu_enc_job_ready,
	.job_abort = wave5_vpu_enc_job_abort,
};

static int wave5_vpu_open_enc(struct file *filp)
{
	struct video_device *vdev = video_devdata(filp);
	struct vpu_device *dev = video_drvdata(filp);
	struct vpu_instance *inst = NULL;
	struct v4l2_ctrl_handler *v4l2_ctrl_hdl;
	int ret = 0;

	inst = kzalloc(sizeof(*inst), GFP_KERNEL);
	if (!inst)
		return -ENOMEM;
	v4l2_ctrl_hdl = &inst->v4l2_ctrl_hdl;

	inst->dev = dev;
	inst->type = VPU_INST_TYPE_ENC;
	inst->ops = &wave5_vpu_enc_inst_ops;

	v4l2_fh_init(&inst->v4l2_fh, vdev);
	filp->private_data = &inst->v4l2_fh;
	v4l2_fh_add(&inst->v4l2_fh);

	INIT_LIST_HEAD(&inst->list);
	list_add_tail(&inst->list, &dev->instances);

	inst->v4l2_m2m_dev = v4l2_m2m_init(&wave5_vpu_enc_m2m_ops);
	if (IS_ERR(inst->v4l2_m2m_dev)) {
		ret = PTR_ERR(inst->v4l2_m2m_dev);
		dev_err(inst->dev->dev, "v4l2_m2m_init, fail: %d\n", ret);
		goto cleanup_inst;
	}

	inst->v4l2_fh.m2m_ctx =
		v4l2_m2m_ctx_init(inst->v4l2_m2m_dev, inst, wave5_vpu_enc_queue_init);
	if (IS_ERR(inst->v4l2_fh.m2m_ctx)) {
		ret = PTR_ERR(inst->v4l2_fh.m2m_ctx);
		goto cleanup_inst;
	}

	v4l2_ctrl_handler_init(v4l2_ctrl_hdl, 30);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave5_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_HEVC_PROFILE,
			       V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN_10, 0,
			       V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave5_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_HEVC_LEVEL, V4L2_MPEG_VIDEO_HEVC_LEVEL_5_1, 0,
			       V4L2_MPEG_VIDEO_HEVC_LEVEL_1);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave5_vpu_enc_ctrl_ops, V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP,
			  0, 63, 1, 8);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave5_vpu_enc_ctrl_ops, V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP,
			  0, 63, 1, 51);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave5_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_H264_PROFILE,
			       V4L2_MPEG_VIDEO_H264_PROFILE_HIGH_444_PREDICTIVE, 0,
			       V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE);
	v4l2_ctrl_new_std_menu(v4l2_ctrl_hdl, &wave5_vpu_enc_ctrl_ops,
			       V4L2_CID_MPEG_VIDEO_H264_LEVEL, V4L2_MPEG_VIDEO_H264_LEVEL_5_1, 0,
			       V4L2_MPEG_VIDEO_H264_LEVEL_1_0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave5_vpu_enc_ctrl_ops, V4L2_CID_MPEG_VIDEO_H264_MIN_QP,
			  0, 63, 1, 8);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave5_vpu_enc_ctrl_ops, V4L2_CID_MPEG_VIDEO_H264_MAX_QP,
			  0, 63, 1, 51);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave5_vpu_enc_ctrl_ops, V4L2_CID_HFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave5_vpu_enc_ctrl_ops, V4L2_CID_VFLIP, 0, 1, 1, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave5_vpu_enc_ctrl_ops, V4L2_CID_ROTATE, 0, 270, 90, 0);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave5_vpu_enc_ctrl_ops, V4L2_CID_MPEG_VIDEO_VBV_SIZE, 10,
			  3000, 1, 3000);
	v4l2_ctrl_new_std(v4l2_ctrl_hdl, &wave5_vpu_enc_ctrl_ops,
			  V4L2_CID_MIN_BUFFERS_FOR_OUTPUT, 1, 32, 1, 1);

	if (v4l2_ctrl_hdl->error) {
		ret = -ENODEV;
		goto cleanup_inst;
	}

	inst->v4l2_fh.ctrl_handler = v4l2_ctrl_hdl;
	v4l2_ctrl_handler_setup(v4l2_ctrl_hdl);

	wave5_set_default_format(&inst->src_fmt, &inst->dst_fmt);
	inst->colorspace = V4L2_COLORSPACE_REC709;
	inst->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	inst->hsv_enc = 0;
	inst->quantization = V4L2_QUANTIZATION_DEFAULT;
	inst->xfer_func = V4L2_XFER_FUNC_DEFAULT;
	inst->frame_rate = 30;

	init_completion(&inst->irq_done);
	ret = kfifo_alloc(&inst->irq_status, 16 * sizeof(int), GFP_KERNEL);
	if (ret) {
		dev_err(inst->dev->dev, "unable to allocate fifo\n");
		goto cleanup_inst;
	}

	inst->id = ida_alloc(&inst->dev->inst_ida, GFP_KERNEL);
	if (inst->id < 0) {
		dev_warn(inst->dev->dev, "unable to allocate instance ID: %d\n", inst->id);
		ret = inst->id;
		goto cleanup_inst;
	}

	return 0;

cleanup_inst:
	wave5_cleanup_instance(inst);
	return ret;
}

static int wave5_vpu_enc_release(struct file *filp)
{
	return wave5_vpu_release_device(filp, wave5_vpu_enc_close, "encoder");
}

static const struct v4l2_file_operations wave5_vpu_enc_fops = {
	.owner = THIS_MODULE,
	.open = wave5_vpu_open_enc,
	.release = wave5_vpu_enc_release,
	.unlocked_ioctl = video_ioctl2,
	.poll = v4l2_m2m_fop_poll,
	.mmap = v4l2_m2m_fop_mmap,
};

int wave5_vpu_enc_register_device(struct vpu_device *dev)
{
	struct video_device *vdev_enc;
	int ret;

	vdev_enc = devm_kzalloc(dev->v4l2_dev.dev, sizeof(*vdev_enc), GFP_KERNEL);
	if (!vdev_enc)
		return -ENOMEM;

	dev->video_dev_enc = vdev_enc;

	strscpy(vdev_enc->name, VPU_ENC_DEV_NAME, sizeof(vdev_enc->name));
	vdev_enc->fops = &wave5_vpu_enc_fops;
	vdev_enc->ioctl_ops = &wave5_vpu_enc_ioctl_ops;
	vdev_enc->release = video_device_release_empty;
	vdev_enc->v4l2_dev = &dev->v4l2_dev;
	vdev_enc->vfl_dir = VFL_DIR_M2M;
	vdev_enc->device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
	vdev_enc->lock = &dev->dev_lock;

	ret = video_register_device(vdev_enc, VFL_TYPE_VIDEO, -1);
	if (ret)
		return ret;

	video_set_drvdata(vdev_enc, dev);

	return 0;
}

void wave5_vpu_enc_unregister_device(struct vpu_device *dev)
{
	video_unregister_device(dev->video_dev_enc);
}
