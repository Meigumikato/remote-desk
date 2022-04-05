#include <iostream>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <SDL2/SDL.h>
}


void SaveMp4(AVFrame *frame);

int main() {

	avdevice_register_all();

	const AVInputFormat *grab_ifmt = av_find_input_format("x11grab");
	if (!grab_ifmt) {
		std::cout << "Could not find x11grab input format" << std::endl;
		return 1;
	}


	AVFormatContext *fmt_ctx = avformat_alloc_context();
	if (!fmt_ctx) {
		std::cout << "Could not allocate format context" << std::endl;
		return 1;
	}

	AVDictionary *options = nullptr;
	av_dict_set(&options, "video_size", "1024x768", 0);
	av_dict_set(&options, "framerate", "30", 0);
	
	int res = avformat_open_input(&fmt_ctx, ":0.0+0,0", grab_ifmt, &options);
	if (res < 0) {
		std::cout << "Could not open input" << std::endl;
		return 1;
	}
	av_dict_free(&options);

	if(avformat_find_stream_info(fmt_ctx, NULL) < 0) {
			printf("Couldn't find stream information.\n");
			return -1;
	}

	int videoindex = -1;
	for(int i = 0; i < fmt_ctx->nb_streams; i++) {
		if(fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
				videoindex=i;
				break;
		}
	}

	if(videoindex == -1) {
			printf("Didn't find a video stream.\n");
			return -1;
	}

	AVCodecParameters *code_param = fmt_ctx->streams[videoindex]->codecpar;

	const AVCodec *codec = avcodec_find_decoder(code_param->codec_id);
	if (!codec) {
		std::cout << "Could not find codec" << std::endl;
		return 1;
	}

	AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(codec_ctx, code_param);
	avcodec_open2(codec_ctx, codec, nullptr);

	AVPacket *pPacket = av_packet_alloc();
	AVFrame *pFrame = av_frame_alloc();

	while (av_read_frame(fmt_ctx, pPacket) >= 0) {

		if (pPacket->stream_index == videoindex) {
			avcodec_send_packet(codec_ctx, pPacket);
			while (avcodec_receive_frame(codec_ctx, pFrame) == 0) {

				printf(
					"Frame %c (%d) pts %ld dts %ld key_frame %d [coded_picture_number %d, display_picture_number %d]\n",
					av_get_picture_type_char(pFrame->pict_type),
					codec_ctx->frame_number,
					pFrame->pts,
					pFrame->pkt_dts,
					pFrame->key_frame,
					pFrame->coded_picture_number,
					pFrame->display_picture_number
				);


			}
		}
	}

	avformat_close_input(&fmt_ctx);
	avcodec_close(codec_ctx);
	avformat_free_context(fmt_ctx);
	avcodec_free_context(&codec_ctx);
	av_packet_free(&pPacket);
	av_frame_free(&pFrame);

	return 0;
}


void SaveMp4(AVFrame *frame) {


	AVFormatContext *ofmt_ctx;
	const AVOutputFormat *ofmt = av_guess_format("mp4", NULL, NULL);
	if (!ofmt) {
		std::cout << "Could not find mp4 output format" << std::endl;
		return;
	}

	avformat_alloc_output_context2(&ofmt_ctx, ofmt, "mp4", "test.mp4");
	if (!ofmt_ctx) {
		std::cout << "Could not allocate output context" << std::endl;
		return;
	}
	
	if (avio_open(&ofmt_ctx->pb, "test.mp4", AVIO_FLAG_WRITE) < 0) {
		std::cout << "Could not open output file" << std::endl;
		return;
	}


	AVStream *out_stream = avformat_new_stream(ofmt_ctx, NULL);
	if (!out_stream) {
		std::cout << "Could not allocate output stream" << std::endl;
		return;
	}

	if (avformat_write_header(ofmt_ctx, nullptr) < 0) {
		std::cout << "Could not write header" << std::endl;
		return;
	}
}
