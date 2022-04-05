extern "C" {
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}


#define CHECK_POINTER(res) \
		if (res == nullptr) { \
			fprintf(stderr, "Error while %s: \n", __func__); \
			return -1; \
		}

#define CHECK_ERROR(res) \
		if (res < 0) { \
			fprintf(stderr, "Error while %s: \n", __func__); \
			return -1; \
		}

class Capture {

 public:

	int InitEnCoder() {
		AVFormatContext *fmt_ctx_ = avformat_alloc_context();
		// find x11grab device
		const AVInputFormat *in_fmt = av_find_input_format("x11grab");
		CHECK_POINTER(in_fmt);

		AVDictionary *options = nullptr;

		// set options

		// open device
		int ret = avformat_open_input(&fmt_ctx_, ":0.0", in_fmt, nullptr);
		CHECK_ERROR(ret);

		ret = avformat_find_stream_info(fmt_ctx_, nullptr);
		CHECK_ERROR(ret);

		int stream_index = -1;
		for (int i = 0; i < fmt_ctx_->nb_streams; i++) {
			if (fmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
				stream_index = i;
				break;
			}
		}
		CHECK_ERROR(stream_index);

		// find decoder
		AVCodecParameters *codec_param = fmt_ctx_->streams[stream_index]->codecpar;
	 	const AVCodec *codec = avcodec_find_decoder(codec_param->codec_id);
		CHECK_POINTER(codec);

		AVCodecContext *codec_ctx_ = avcodec_alloc_context3(codec);
		avcodec_parameters_to_context(codec_ctx_, codec_param);
		CHECK_POINTER(codec_ctx_);

		ret = avcodec_open2(codec_ctx_, codec, nullptr);
		CHECK_ERROR(ret);

		return ret;
	}

	int capture() {

		// create frame
		AVFrame *frame = av_frame_alloc();
		AVPacket *packet = av_packet_alloc();
		CHECK_POINTER(frame);

		while (av_read_frame(fmt_ctx_, packet)) {

			avcodec_send_packet(codec_ctx_, packet);

			while (avcodec_receive_frame(codec_ctx_, frame) == 0) {
				// do something with frame
				codec(frame);
			}

		}
		return 0;
	}
 private:
	AVCodecContext *codec_ctx_;
	AVFormatContext *fmt_ctx_;

	int codec(AVFrame *decode_frame) {

		// init encodec context
		AVFormatContext *fmt_ctx = avformat_alloc_context();
		const AVOutputFormat *out_fmt = av_guess_format("h264", nullptr, nullptr);
		int ret = avformat_alloc_output_context2(&fmt_ctx, out_fmt, "h264", "test.h264");
		CHECK_ERROR(ret);

		// find encoder
		const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		CHECK_POINTER(codec);

		// io open
		ret = avio_open(&fmt_ctx->pb, "udp://localhost:7000", AVIO_FLAG_WRITE);
		CHECK_ERROR(ret);

		// init stream
		AVStream *stream = avformat_new_stream(fmt_ctx, codec);
		CHECK_POINTER(stream);

		stream->time_base = AVRational{1, 25};

		AVCodecParameters *codec_param = stream->codecpar;
		codec_param->codec_id = AV_CODEC_ID_H264;
		codec_param->codec_type = AVMEDIA_TYPE_VIDEO;
		codec_param->width = decode_frame->width;
		codec_param->height = decode_frame->height;
		codec_param->bit_rate = 11000;

		AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
		CHECK_POINTER(codec_ctx);
		ret = avcodec_parameters_to_context(codec_ctx, codec_param);
		CHECK_ERROR(ret);

		codec_ctx->time_base = stream->time_base;
		codec_ctx->framerate = AVRational{25, 1};
		codec_ctx->gop_size = 10;


		ret = avcodec_open2(codec_ctx, codec, nullptr);
		CHECK_ERROR(ret);


		AVPacket *packet = av_packet_alloc();
		// collect frames 
		while (1) {
			avcodec_send_frame(codec_ctx, decode_frame);
			while (avcodec_receive_packet(codec_ctx, packet) == 0) {
				// do something with packet
				av_write_frame(fmt_ctx, packet);
			}
		}

		return 0;
	}
};


