/*
 * Adapted from https://ffmpeg.org/doxygen/2.1/doc_2examples_2demuxing_8c-example.html
 */

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavformat/avformat.h>

//#define UPDATE_HEADER

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

	return;
}

#ifdef UPDATE_HEADER
static void vp9_update_frame_header(AVPacket *pkt)
{

	unsigned char *buf = pkt->data;
	unsigned char *old_header = NULL;
	unsigned char marker;
	int dsize = pkt->size;
	int frame_number;
	int cur_frame, cur_mag, mag, index_sz, offset[9], size[8], tframesize[9];
	int mag_ptr;
	int ret;
	int total_datasize = 0;

	marker = buf[dsize - 1];
	if ((marker & 0xe0) == 0xc0) {
		frame_number = (marker & 0x7) + 1;
		mag = ((marker >> 3) & 0x3) + 1;
		index_sz = 2 + mag * frame_number;
		printf(" frame_number : %d, mag : %d; index_sz : %d\n", frame_number, mag, index_sz);
		offset[0] = 0;
		mag_ptr = dsize - mag * frame_number - 2;
		if (buf[mag_ptr] != marker) {
			fprintf(stderr, " Wrong marker2 : 0x%X --> 0x%X\n", marker, buf[mag_ptr]);
			exit(1);
		}
		mag_ptr++;
		for (cur_frame = 0; cur_frame < frame_number; cur_frame++) {
			size[cur_frame] = 0;
			for (cur_mag = 0; cur_mag < mag; cur_mag++) {
				size[cur_frame] = size[cur_frame]  | (buf[mag_ptr] << (cur_mag*8) );
				mag_ptr++;
			}
			offset[cur_frame+1] = offset[cur_frame] + size[cur_frame];
			if (cur_frame == 0)
				tframesize[cur_frame] = size[cur_frame];
			else
				tframesize[cur_frame] = tframesize[cur_frame - 1] + size[cur_frame];
			total_datasize += size[cur_frame];
		}
	} else {
		frame_number = 1;
		offset[0] = 0;
		size[0] = dsize;
		total_datasize += dsize;
		tframesize[0] = dsize;
	}

	if (total_datasize > dsize) {
		fprintf(stderr, "DATA overflow : 0x%X --> 0x%X\n", total_datasize, dsize);
		exit(1);
	}

	if (frame_number >= 1) {
		int need_more = total_datasize + frame_number * 16 - dsize;
		ret = av_grow_packet(pkt, need_more);
		if (ret < 0) {
			fprintf(stderr, "ERROR!!! grow_packet for apk failed\n");
			exit(1);
		}
	}

	for (cur_frame = frame_number - 1; cur_frame >= 0; cur_frame--) {
		int framesize = size[cur_frame];
		int oldframeoff = tframesize[cur_frame] - framesize;
		int outheaderoff = oldframeoff + cur_frame * 16;
		uint8_t *fdata = pkt->data + outheaderoff;
		uint8_t *old_framedata = pkt->data + oldframeoff;

		memmove(fdata + 16, old_framedata, framesize);
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

		framesize -= 4;
		if (!old_header) {
			// nothing
		} else if (old_header > fdata + 16 + framesize) {
			fprintf(stderr, "data has gaps,set to 0\n");
			memset(fdata + 16 + framesize, 0, (old_header - fdata + 16 + framesize));
		} else if (old_header < fdata + 16 + framesize) {
			fprintf(stderr, "ERROR. data overwrited. overwrite %d\n", fdata + 16 + framesize - old_header); 
		}
		old_header = fdata;
	}

	return;
}
#endif

int vp9_parse_stream(char *out, int out_size)
{
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

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
