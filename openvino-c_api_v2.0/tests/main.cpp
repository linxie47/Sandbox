#include <stdio.h>
#include <string.h>
#include <iostream>

#include "test.h"
#include "ff_base_inference.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

int main(int argc, char const *argv[])
{
    gflags::ParseCommandLineFlags(&argc, (char ***)&argv, true);
    
    // FLAGS_i.c_str();
    std::cout << "test start!" << std::endl;
    
    FFBaseInference base = { };
    memset(&base, 0, sizeof(FFBaseInference));
    base.model = (char *)FLAGS_m.c_str();
    base.device = (char *)FLAGS_infer_device.c_str();
    base.inference_id = (char *)"ie_wrapper-test";
    base.is_full_frame = TRUE;
    base.nireq = 2;
    base.batch_size = 1;
    base.every_nth_frame = 1;
    base.infer_config = strdup("");
    base.object_class = strdup("");

    ff_base_inference_init(&base);
    
    std::cout << "inference init success!" << std::endl;

    AVHWDeviceType decode_type = AV_HWDEVICE_TYPE_NONE;
    AVStream *video = NULL;
    AVCodec *decoder = NULL;
    AVInputFormat *inputFormat = NULL;
    AVFormatContext *input_ctx = NULL;
    AVCodecContext *decoder_ctx = NULL;
    AVBufferRef *hw_device_ctx = NULL;
    struct SwsContext *sws_context = NULL;

    int video_stream = 0;

    if (!FLAGS_decode_device.empty()) {
        decode_type = av_hwdevice_find_type_by_name(FLAGS_decode_device.c_str());
        RET_IF_ZERO(decode_type);
        RET_IF_FAIL(av_hwdevice_ctx_create(&hw_device_ctx, decode_type, NULL, NULL, 0));
    }

    if (!FLAGS_f.empty()) {
        inputFormat = av_find_input_format(FLAGS_f.c_str());
    }
    if (FLAGS_i.empty()) {
        std::cout << "input file is needed! e.g. -i xxxx" << std::endl;
        return -1;
    }
    RET_IF_FAIL(avformat_open_input(&input_ctx, FLAGS_i.c_str(), inputFormat, NULL));

    /* find the video stream information */
    RET_IF_FAIL(avformat_find_stream_info(input_ctx, NULL));
    video_stream = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    RET_IF_FAIL(video_stream);

    RET_IF_ZERO(decoder_ctx = avcodec_alloc_context3(decoder));

    video = input_ctx->streams[video_stream];
    RET_IF_FAIL(avcodec_parameters_to_context(decoder_ctx, video->codecpar));

    if (hw_device_ctx) {
        decoder_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    }

    RET_IF_FAIL(avcodec_open2(decoder_ctx, decoder, NULL));
    
    static int processed_frame = 0;

    while (true) {
        AVPacket packet = {};

        if (av_read_frame(input_ctx, &packet) < 0) {
            break;
        }

        if (video_stream == packet.stream_index) {
            RET_IF_FAIL(avcodec_send_packet(decoder_ctx, &packet));

            while (true) {
                AVFrame *frame = av_frame_alloc();
                RET_IF_ZERO(frame);

                int ret = avcodec_receive_frame(decoder_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    av_frame_free(&frame);
                    break;
                }
                RET_IF_FAIL(ret);

                if (!frame->format) frame->format = AV_PIX_FMT_YUV420P; //TODO

                // CSC
                sws_context = sws_getCachedContext(sws_context,
                                           frame->width, frame->height, (AVPixelFormat)frame->format,
                                           frame->width, frame->height, AV_PIX_FMT_BGR0,
                                           SWS_BICUBIC, NULL, NULL, NULL);
                if (!sws_context) {
                    throw std::runtime_error("Error creating FFMPEG sws_scale");
                }

                AVFrame *frame_input = av_frame_alloc();
                RET_IF_ZERO(frame_input);

                frame_input->width = frame->width;
                frame_input->height = frame->height;
                frame_input->format = AV_PIX_FMT_BGR0;
                ret = av_frame_get_buffer(frame_input, 0);
                RET_IF_FAIL(ret);

                if (!sws_scale(sws_context, frame->data, frame->linesize, 0, frame->height, frame_input->data, frame_input->linesize)) {
                    throw std::runtime_error("Error on FFMPEG sws_scale");
                }

                // Submit inference request
                try {
                    if (0 != ff_base_inference_send(&base, frame_input)) {
                        throw std::runtime_error("Error inference send");
                    }
                } catch (const std::exception &exc) {
                    printf("Exception: %s\n", exc.what());
                }

                ProcessedFrame out_frame = { };
                ff_base_inference_fetch(&base, &out_frame);
                if (out_frame.frame != 0) {
                    processed_frame++;
                }

                av_frame_free(&frame);
                av_frame_free(&frame_input);
            }
        }

        av_packet_unref(&packet);
    }

    printf("Exiting: processed %d\n", processed_frame);

    avcodec_free_context(&decoder_ctx);
    avformat_close_input(&input_ctx);
    av_buffer_unref(&hw_device_ctx);

    return 0;
}
