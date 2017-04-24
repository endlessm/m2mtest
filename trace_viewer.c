#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/limits.h>

struct data_request {
	bool returned;
	int request;
	char buf[1024];
};

int main(int argc, char *argv[])
{
	struct data_request data = { 0 };
	char *f_data = argv[1];
	int fd_data;
	int frame_cnt = 0;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
		return 1;
	}

	if ((fd_data = open(f_data, O_RDONLY)) == -1) {
		perror("fd_data error");
		return 1;
	}

	while (read(fd_data, &data, sizeof(data)) > 0) {
		struct v4l2_buffer *vbuf;
		struct v4l2_requestbuffers *rbuf;
		struct v4l2_format *fbuf;
		struct v4l2_event_subscription *ebuf;
		struct v4l2_exportbuffer *xbuf;
		struct v4l2_fmtdesc *dbuf;
		struct v4l2_event *evbuf;
		int size, dir, nr, type;

		size = _IOC_SIZE(data.request);
		nr = _IOC_NR(data.request);
		dir = _IOC_DIR(data.request);

		switch (nr) {
		case _IOC_NR(VIDIOC_QUERYCAP):
			printf("{%s} [VIDIOC_QUERYCAP]\n", (data.returned) ? "R" : " ");
			break;
		case _IOC_NR(VIDIOC_ENUM_FMT):
			dbuf = (struct v4l2_fmtdesc *) &data.buf;
			printf("%s {%s} [VIDIOC_ENUM_FMT (type: %d)]\n",
			       (dbuf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ? "\t" : "\t\t",
			       (data.returned) ? "R" : " ",
			       dbuf->type);
			break;
		case _IOC_NR(VIDIOC_G_FMT):
			fbuf = (struct v4l2_format *) &data.buf;
			printf("%s {%s} [VIDIOC_G_FMT (type: %d, format: %dx%d, plane sizes: %d, %d)]\n",
			       (fbuf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ? "\t" : "\t\t",
			       (data.returned) ? "R" : " ",
			       fbuf->type, fbuf->fmt.pix_mp.width, fbuf->fmt.pix_mp.height,
			       fbuf->fmt.pix_mp.plane_fmt[0].sizeimage, fbuf->fmt.pix_mp.plane_fmt[1].sizeimage);
			break;
		case _IOC_NR(VIDIOC_S_FMT):
			fbuf = (struct v4l2_format *) &data.buf;
			printf("%s {%s} [VIDIOC_S_FMT (type: %d, sizeimage: %d, pixelformat: %d)]\n",
			       (fbuf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ? "\t" : "\t\t",
			       (data.returned) ? "R" : " ",
			       fbuf->type, fbuf->fmt.pix_mp.plane_fmt[0].sizeimage,
			       fbuf->fmt.pix_mp.pixelformat);
			break;
		case _IOC_NR(VIDIOC_REQBUFS):
			rbuf = (struct v4l2_requestbuffers *) &data.buf;
			printf("%s {%s} [VIDIOC_REQBUFS (type: %d, output buffers: %d)]\n",
			       (rbuf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ? "\t" : "\t\t",
			       (data.returned) ? "R" : " ",
			       rbuf->type, rbuf->count);
		case _IOC_NR(VIDIOC_QUERYBUF):
			vbuf = (struct v4l2_buffer *) &data.buf;
			printf("%s {%s} [VIDIOC_QUERYBUF (type: %d, index: %d, length: %d)]\n",
			       (vbuf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ? "\t" : "\t\t",
			       (data.returned) ? "R" : " ",
			       vbuf->type, vbuf->index, vbuf->length);
			break;
		case _IOC_NR(VIDIOC_QBUF):
			vbuf = (struct v4l2_buffer *) &data.buf;
			printf("%s {%s} [VIDIOC_QBUF (type: %d, index: %d, length: %d)]\n",
			       (vbuf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ? "\t" : "\t\t",
			       (data.returned) ? "R" : " ",
			       vbuf->type, vbuf->index, vbuf->length);
			if (!data.returned && vbuf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
				printf("\t~~~ Frame: %03d ~~~\n", ++frame_cnt);
			break;
		case _IOC_NR(VIDIOC_EXPBUF):
			xbuf = (struct v4l2_exportbuffer *) &data.buf;
			printf("%s {%s} [VIDIOC_EXPBUF (type: %d, index: %d, plane: %d)]\n",
			       (xbuf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ? "\t" : "\t\t",
			       (data.returned) ? "R" : " ",
			       xbuf->type, xbuf->index, xbuf->plane);
			break;
		case _IOC_NR(VIDIOC_DQBUF):
			vbuf = (struct v4l2_buffer *) &data.buf;
			printf("%s {%s} [VIDIOC_DQBUF (type: %d, index: %d, length: %d)]\n",
			       (vbuf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ? "\t" : "\t\t",
			       (data.returned) ? "R" : " ",
			       vbuf->type, vbuf->index, vbuf->length);
			break;
		case _IOC_NR(VIDIOC_STREAMON):
			type = (int) data.buf[0];
			printf("%s {%s} [VIDIOC_STREAMON type: %d]\n",
			       (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ? "\t" : "\t\t",
			       (data.returned) ? "R" : " ",
			       type);
			break;
		case _IOC_NR(VIDIOC_STREAMOFF):
			type = (int) data.buf[0];
			printf("%s {%s} [VIDIOC_STREAMOFF type: %d]\n",
			       (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ? "\t" : "\t\t",
			       (data.returned) ? "R" : " ",
			       type);
			break;
		case _IOC_NR(VIDIOC_SUBSCRIBE_EVENT):
			ebuf = (struct v4l2_event_subscription *) &data.buf;
			printf("%s {%s} [VIDIOC_SUBSCRIBE_EVENT (event: %d)]\n",
			       (ebuf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ? "\t" : "\t\t",
			       (data.returned) ? "R" : " ",
			       ebuf->type);
			break;
		case _IOC_NR(VIDIOC_G_CROP):
			printf("{%s} [VIDIOC_G_CROP]\n",
			       (data.returned) ? "R" : " ");
			break;
		case _IOC_NR(VIDIOC_ENUM_FRAMESIZES):
			printf("{%s} [VIDIOC_ENUM_FRAMESIZES]\n",
			       (data.returned) ? "R" : " ");
			break;
		case _IOC_NR(VIDIOC_TRY_DECODER_CMD):
			printf("{%s} [VIDIOC_TRY_DECODER_CMD]\n",
			       (data.returned) ? "R" : " ");
			break;
		case _IOC_NR(VIDIOC_DQEVENT):
			evbuf = (struct v4l2_event *) &data.buf;
			printf("%s {%s} [VIDIOC_DQEVENT (type: %d)]\n",
			       (evbuf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ? "\t" : "\t\t",
			       (data.returned) ? "R" : " ",
			       evbuf->type);
			break;
		default:
			printf("{%s} IOCTL 0x%08x => nr:%3d, size:%4d, dir: %s%s\n",
			       (data.returned) ? "R" : " ",
			       data.request, nr, size,
			       (dir & _IOC_READ) ? "R" : "",
			       (dir & _IOC_WRITE) ? "W" : "");
			break;

		}
	}

	return 0;
}
