#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <stdio.h>
#include "png.h"
#include "util.h"

void error_exit() {
    printf("Couldn't read file\n");
    exit(-1);
}

int main(int argc, char *argv[])
{
    const AVCodec *vcodec = NULL;
    AVFormatContext *format = NULL;
    AVCodecContext *vctx = NULL;
    AVStream *vstream = NULL;
    AVFrame *copyframe = NULL;
    AVFrame *frame = NULL;
    AVPacket *pkt = NULL;

    int found = 0;

    if (argc != 4) {
        printf("Expected an input, a time, and an output\n");
        return -1;
    }

    av_log_set_level(AV_LOG_QUIET);

    const char *input = argv[1];
    const char *output = argv[3];
    AVRational time = av_d2q(atof(argv[2]), INT_MAX);

    if (open_input_correct_demuxer(&format, input) != 0) {
        error_exit();
    }

    if (avformat_find_stream_info(format, NULL) < 0) {
        error_exit();
    }

    int vstream_idx = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, &vcodec, 0);
    if (vstream_idx < 0) {
        error_exit();
    }

    vstream = format->streams[vstream_idx];

    // Set up decoding context
    vctx = avcodec_alloc_context3(vcodec);
    if (!vctx) {
        error_exit();
    }

    if (avcodec_parameters_to_context(vctx, vstream->codecpar) < 0) {
        error_exit();
    }

    if (avcodec_open2(vctx, vcodec, NULL) < 0) {
        error_exit();
    }

    pkt = av_packet_alloc();
    if (!pkt) {
        error_exit();
    }

    copyframe = av_frame_alloc();
    frame = av_frame_alloc();
    if (!copyframe || !frame) {
        error_exit();
    }

    // Loop until we get to the first video frame past the intended pts,
    // decoding all video frames along the way.

    while (av_read_frame(format, pkt) >= 0) {
        if (pkt->stream_index == vstream_idx) {
            AVRational cur_time  = { pkt->pts, vstream->time_base.den };
            AVRational next_time = { pkt->pts + pkt->duration, vstream->time_base.den };

            if (avcodec_send_packet(vctx, pkt) != 0) {
                // Decoder returned an error
                error_exit();
            }

            int ret = avcodec_receive_frame(vctx, frame);
            av_packet_unref(pkt);

            if (ret == AVERROR(EAGAIN)) {
                // Need more data, can't receive frame yet
                continue;
            }

            if (ret != 0) {
                // Decoder returned an error
                error_exit();
            }

            // We have a candidate frame. Keep a reference to it until and unless we can
            // find a better candidate frame.
            found = 1;

            av_frame_unref(copyframe);
            if (av_frame_ref(copyframe, frame) < 0) {
                // Failed to re-reference frame.
                error_exit();
            }

            // If this is the first frame past the requested time or the
            // current frame contains the requested time, pick this frame.
            if (av_cmp_q(cur_time, time) >= 0 || (av_cmp_q(cur_time, time) <= 0 && av_cmp_q(next_time, time) >= 0)) {
                break;
            }

            av_frame_unref(frame);
        } else {
            av_packet_unref(pkt);
        }
    }

    // Found the frame; write to the provided file
    if (!found || mediatools_write_frame_to_png(copyframe, output) < 0) {
        error_exit();
    }

    av_frame_free(&frame);
    av_frame_free(&copyframe);
    av_packet_free(&pkt);
    avcodec_free_context(&vctx);
    avformat_close_input(&format);

    return 0;
}
