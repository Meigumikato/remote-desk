extern "C" {
#include <stdio.h>
#include <unistd.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}


int main() {
	avdevice_register_all();

    /////////////解码器部分//////////////////////
    AVFormatContext *inFmtCtx = avformat_alloc_context();
    AVCodecContext  *inCodecCtx = NULL;
    const AVCodec   *inCodec = NULL;
    AVPacket        *inPkt =av_packet_alloc();
    AVFrame         *srcFrame =av_frame_alloc();
    AVFrame         *yuvFrame =av_frame_alloc();

    struct SwsContext *img_ctx = NULL;

    int inVideoStreamIndex = -1;

    // capture 
    const AVInputFormat *inFmt = av_find_input_format("x11grab");
    if(avformat_open_input(&inFmtCtx,":0.0",inFmt,NULL)<0){
        printf("Cannot open camera.\n");
        return -1;
    }

    if(avformat_find_stream_info(inFmtCtx,NULL)<0){
        printf("Cannot find any stream in file.\n");
        return -1;
    }

    for(size_t i=0;i<inFmtCtx->nb_streams;i++){
        if(inFmtCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO){
            inVideoStreamIndex=i;
            break;
        }
    }
    if(inVideoStreamIndex==-1){
        printf("Cannot find video stream in file.\n");
        return -1;
    }

    AVCodecParameters *inVideoCodecPara = inFmtCtx->streams[inVideoStreamIndex]->codecpar;
    if(!(inCodec = avcodec_find_decoder(inVideoCodecPara->codec_id))){
        printf("Cannot find valid video decoder.\n");
        return -1;
    }
    if(!(inCodecCtx = avcodec_alloc_context3(inCodec))){
        printf("Cannot alloc valid decode codec context.\n");
        return -1;
    }
    if(avcodec_parameters_to_context(inCodecCtx,inVideoCodecPara)<0){
        printf("Cannot initialize parameters.\n");
        return -1;
    }

    if(avcodec_open2(inCodecCtx,inCodec,NULL)<0){
        printf("Cannot open codec.\n");
        return -1;
    }

    img_ctx = sws_getContext(inCodecCtx->width,
                             inCodecCtx->height,
                             inCodecCtx->pix_fmt,
                             inCodecCtx->width,
                             inCodecCtx->height,
                             AV_PIX_FMT_YUV420P,
                             SWS_BICUBIC,
                             NULL,NULL,NULL);

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
                                            inCodecCtx->width,
                                            inCodecCtx->height,1);
    uint8_t* out_buffer = (unsigned char*)av_malloc(numBytes*sizeof(unsigned char));

    int ret = av_image_fill_arrays(yuvFrame->data,
                               yuvFrame->linesize,
                               out_buffer,
                               AV_PIX_FMT_YUV420P,
                               inCodecCtx->width,
                               inCodecCtx->height,
                               1);
    if(ret<0){
        printf("Fill arrays failed.\n");
        return -1;
    }
    //////////////解码器部分结束/////////////////////

	const char* outFile = "result.h264";

    const AVOutputFormat *outFmt = NULL;
    AVCodecContext *outCodecCtx=NULL;
    const AVCodec        *outCodec = NULL;
    AVStream *outVStream     = NULL;

    AVPacket *outPkt = av_packet_alloc();

    //打开输出文件，并填充fmtCtx数据
    AVFormatContext *outFmtCtx = avformat_alloc_context();
    if(avformat_alloc_output_context2(&outFmtCtx, NULL, "h264", outFile)<0){
        printf("Cannot alloc output file context.\n");
        return -1;
    }
    outFmt = outFmtCtx->oformat;

    //打开输出文件
    if(avio_open(&outFmtCtx->pb, "tcp://127.0.0.1:7000", AVIO_FLAG_READ_WRITE)<0){
        printf("output file open failed.\n");
        return -1;
    }

    //创建h264视频流，并设置参数
    outVStream = avformat_new_stream(outFmtCtx,outCodec);
    if(outVStream==nullptr){
        printf("create new video stream fialed.\n");
        return -1;
    }
    outVStream->time_base.den=30;
    outVStream->time_base.num=1;

    //编码参数相关
    AVCodecParameters *outCodecPara = outFmtCtx->streams[outVStream->index]->codecpar;
    outCodecPara->codec_type=AVMEDIA_TYPE_VIDEO;
    outCodecPara->codec_id = outFmt->video_codec;
    outCodecPara->width = 480;
    outCodecPara->height = 360;
    outCodecPara->bit_rate = 110000;

    //查找编码器
    outCodec = avcodec_find_encoder(outFmt->video_codec);
    if(outCodec==NULL){
        printf("Cannot find any encoder.\n");
        return -1;
    }

    //设置编码器内容
    outCodecCtx = avcodec_alloc_context3(outCodec);
    avcodec_parameters_to_context(outCodecCtx,outCodecPara);
    if(outCodecCtx==NULL){
        printf("Cannot alloc output codec content.\n");
        return -1;
    }
    outCodecCtx->codec_id = outFmt->video_codec;
    outCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    outCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    outCodecCtx->width = inCodecCtx->width;
    outCodecCtx->height = inCodecCtx->height;
    outCodecCtx->time_base.num=1;
    outCodecCtx->time_base.den=30;
    outCodecCtx->bit_rate=110000;
    outCodecCtx->gop_size=10;

    if(outCodecCtx->codec_id==AV_CODEC_ID_H264){
        outCodecCtx->qmin=10;
        outCodecCtx->qmax=51;
        outCodecCtx->qcompress=(float)0.6;
    }else if(outCodecCtx->codec_id==AV_CODEC_ID_MPEG2VIDEO){
        outCodecCtx->max_b_frames=2;
    }else if(outCodecCtx->codec_id==AV_CODEC_ID_MPEG1VIDEO){
        outCodecCtx->mb_decision=2;
    }

    //打开编码器
    if(avcodec_open2(outCodecCtx,outCodec,NULL)<0){
        printf("Open encoder failed.\n");
        return -1;
    }
    ///////////////编码器部分结束////////////////////

	///////////////编解码部分//////////////////////
    yuvFrame->format = outCodecCtx->pix_fmt;
    yuvFrame->width = outCodecCtx->width;
    yuvFrame->height = outCodecCtx->height;

    ret = avformat_write_header(outFmtCtx,NULL);

    int count = 0;
    while(av_read_frame(inFmtCtx,inPkt)>=0 && count<1000){
        if(inPkt->stream_index == inVideoStreamIndex){
            if(avcodec_send_packet(inCodecCtx,inPkt)>=0){
                while((ret=avcodec_receive_frame(inCodecCtx,srcFrame))>=0){
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        return -1;
                    else if (ret < 0) {
                        fprintf(stderr, "Error during decoding\n");
                        exit(1);
                    }
                    sws_scale(img_ctx,
                              srcFrame->data,srcFrame->linesize,
                              0,inCodecCtx->height,
                              yuvFrame->data,yuvFrame->linesize);

                    yuvFrame->pts=srcFrame->pts;
                    //encode
                    if(avcodec_send_frame(outCodecCtx, yuvFrame) >= 0){
                        if(avcodec_receive_packet(outCodecCtx, outPkt) >= 0){
                            printf("encode one frame.\n");
                            ++count;
                            outPkt->stream_index = outVStream->index;
                            av_packet_rescale_ts(outPkt,outCodecCtx->time_base,
                                                 outVStream->time_base);
                            outPkt->pos=-1;
                            av_interleaved_write_frame(outFmtCtx,outPkt);

                            av_packet_unref(outPkt);
                        }
                    }
                    usleep(1000*24);
                }
            }
            av_packet_unref(inPkt);
        }
    }
	return 0;
}
