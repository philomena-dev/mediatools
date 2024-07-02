#include <libavformat/avformat.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "validation.h"
#include "util.h"

static int64_t start_time(AVStream *stream)
{
    if (stream->start_time == AV_NOPTS_VALUE) {
        return 0;
    }

    return stream->start_time;
}

static void correct_aspect_ratio(int *width, int *height, AVRational aspect_ratio)
{
    const AVRational ONE = av_make_q(1, 1);
    const int cmp = av_cmp_q(aspect_ratio, ONE);

    if (cmp == 0 || aspect_ratio.num == 0 || aspect_ratio.den == 0) {
        // Return width and height unchanged
    } else if (cmp < 0) {
        // Reduce width, leave height unchanged
        *width = *width * av_q2d(aspect_ratio);
    } else {
        // Reduce height, leave width unchanged
        *height = *height * av_q2d(av_inv_q(aspect_ratio));
    }
}

int main(int argc, char *argv[])
{
    AVFormatContext *format = NULL;
    struct stat statbuf;
    AVPacket pkt;

    if (argc != 2) {
        printf("No input specified\n");
        return -1;
    }

    av_log_set_level(AV_LOG_QUIET);

    if (stat(argv[1], &statbuf) != 0) {
        printf("Couldn't read file\n");
        return -1;
    }

    if (open_input_correct_demuxer(&format, argv[1]) != 0) {
        printf("Couldn't read file\n");
        return -1;
    }

    if (avformat_find_stream_info(format, NULL) < 0) {
        printf("Couldn't read file \n");
        return -1;
    }

    if (!mediatools_validate_video(format)) {
        // Error is printed by validation function
        return -1;
    }

    int vstream_idx = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (vstream_idx < 0) {
        printf("Couldn't read file\n");
        return -1;
    }

    uint64_t frames = 0;
    int64_t last_pts = 0;
    int last_stream = 0;

    while (av_read_frame(format, &pkt) >= 0) {
        int64_t new_pts = pkt.pts + pkt.duration;

        if (last_pts < new_pts) {
            last_pts = new_pts;
            last_stream = pkt.stream_index;
        }

        if (pkt.stream_index == vstream_idx)
            ++frames;

        av_packet_unref(&pkt);
    }

    AVStream *stream = format->streams[last_stream];
    AVRational dur   = av_mul_q(av_make_q(last_pts - start_time(stream), 1), stream->time_base);

    if (!mediatools_validate_duration(dur))
        return -1;

    AVStream *vstream = format->streams[vstream_idx];
    AVCodecParameters *vpar = vstream->codecpar;
    AVRational aspect_ratio = vstream->sample_aspect_ratio;
    int width = vpar->width;
    int height = vpar->height;
    correct_aspect_ratio(&width, &height, aspect_ratio);

    printf("%ld %lu %d %d %d %d\n", statbuf.st_size, frames, width, height, dur.num, dur.den);

    avformat_close_input(&format);

    return 0;
}
