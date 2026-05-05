#ifndef _VALIDATION_H_DEFINED
#define _VALIDATION_H_DEFINED

#include <libavformat/avformat.h>

int mediatools_validate_video(AVFormatContext *format, bool *format_is_animated);
int mediatools_validate_duration(AVRational dur);

#endif
