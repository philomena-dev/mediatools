#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "stat.h"

typedef struct RsvgHandle RsvgHandle;
typedef struct RsvgDimensionData {
    int width;
    int height;
    double em;
    double ex;
} RsvgDimensionData;

extern RsvgHandle *
rsvg_handle_new_from_file(const char *file_name, void **error);

extern void
rsvg_handle_get_dimensions(RsvgHandle *handle, RsvgDimensionData *dimension_data);

// SVG is a special snowflake format that needs different treatment.
//
// Other tools will try to actually render the file just to get
// the dimensions. librsvg allows getting the dimensions without
// rendering, which important because the stat tools should run
// quickly.

int main(int argc, char *argv[])
{
    RsvgHandle *handle;
    RsvgDimensionData dimensions;
    struct stat statbuf;

    if (argc != 2) {
        printf("No input specified\n");
        return -1;
    }

    if (stat(argv[1], &statbuf) != 0) {
        printf("Couldn't read file\n");
        return -1;
    }

    handle = rsvg_handle_new_from_file(argv[1], NULL);
    if (handle == NULL) {
        printf("Couldn't read file\n");
        return -1;
    }

    rsvg_handle_get_dimensions(handle, &dimensions);

    if (dimensions.width < 1 || dimensions.width > 32767) {
        printf("Invalid width %d\n", dimensions.width);
        return -1;
    }

    if (dimensions.height < 1 || dimensions.height > 32767) {
        printf("Invalid height %d\n", dimensions.height);
        return -1;
    }

    const MediaStat result = {
        .is_animated = false,
        .frames = 1,
        .width = dimensions.width,
        .height = dimensions.height,
        .duration_num = 1,
        .duration_den = 25,
    };

    print_stat(&result);

    return 0;
}
