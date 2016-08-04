/*
 * Adapted from https://ffmpeg.org/doxygen/2.1/doc_2examples_2demuxing_8c-example.html
 */

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>

#define UPDATE_HEADER

static AVFormatContext *fmt_ctx = NULL;
static int video_stream_idx = -1;
static AVStream *video_stream = NULL;
static AVCodecContext *video_dec_ctx = NULL;
static AVPacket pkt;

void vp9_input_open(const char *in_file)
{
	/* register all formats and codecs */
	av_register_all();

	/* open input file, and allocate format context */
	if (avformat_open_input(&fmt_ctx, in_file, NULL, NULL) < 0) {
		fprintf(stderr, "Could not open source file %s\n", in_file);
		exit(1);
	}

	/* retrieve stream information */
	if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
		fprintf(stderr, "Could not find stream information\n");
		exit(1);
	}

	video_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (video_stream_idx < 0) {
		fprintf(stderr, "Could not find video stream in input file\n");
		exit(1);
	}

	/* dump input information to stderr */
	av_dump_format(fmt_ctx, 0, in_file, 0);

	/* initialize packet, set data to NULL, let the demuxer fill it */
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	return;
}

#ifdef UPDATE_HEADER
static void vp9_update_frame_header(AVPacket *pkt)
{
	int ret;
	int framesize;
	uint8_t *fdata = pkt->data;

	framesize = pkt->size;

	ret = av_grow_packet(pkt, 16);
	if (ret < 0) {
		fprintf(stderr, "Error in updating the frame header\n");
		exit(1);
	}

	memmove(fdata + 16, fdata, framesize);
	framesize += 4;

	fdata[0] = (framesize >> 24) & 0xff;
	fdata[1] = (framesize >> 16) & 0xff;
	fdata[2] = (framesize >> 8) & 0xff;
	fdata[3] = (framesize >> 0) & 0xff;
	fdata[4] = ((framesize >> 24) & 0xff) ^0xff;
	fdata[5] = ((framesize >> 16) & 0xff) ^0xff;
	fdata[6] = ((framesize >> 8) & 0xff) ^0xff;
	fdata[7] = ((framesize >> 0) & 0xff) ^0xff;
	fdata[8] = 0;
	fdata[9] = 0;
	fdata[10] = 0;
	fdata[11] = 1;
	fdata[12] = 'A';
	fdata[13] = 'M';
	fdata[14] = 'L';
	fdata[15] = 'V';

	return;
}
#endif

int vp9_parse_stream(char *out, int out_size)
{
	if (av_read_frame(fmt_ctx, &pkt) < 0)
		return 0;

#ifdef UPDATE_HEADER
	vp9_update_frame_header(&pkt);
#endif

	if (pkt.stream_index == video_stream_idx)
        {
		if (out_size < pkt.size) {
			fprintf(stderr, "Output buffer too small for current frame\n");
			exit(1);
		}
		memcpy(out, pkt.data, pkt.size);
	}

	return pkt.size;
}
