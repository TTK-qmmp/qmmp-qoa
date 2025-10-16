#include "qoaplay.h"

#define QOA_IMPLEMENTATION
#include "qoa.h"
#include <string.h>

qoaplay_desc *qoaplay_open(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }

    /* Read and decode the file header */

    unsigned char header[QOA_MIN_FILESIZE];
    int read = fread(header, QOA_MIN_FILESIZE, 1, file);
    if (!read) {
        return NULL;
    }

    qoa_desc qoa;
    unsigned int first_frame_pos = qoa_decode_header(header, QOA_MIN_FILESIZE, &qoa);
    if (!first_frame_pos) {
        return NULL;
    }

    /* Rewind the file back to beginning of the first frame */

    fseek(file, first_frame_pos, SEEK_SET);

    /* Allocate one chunk of memory for the qoaplay_desc struct, the sample data
    for one frame and a buffer to hold one frame of encoded data. */

    unsigned int buffer_size = qoa_max_frame_size(&qoa);
    unsigned int sample_data_size = qoa.channels * QOA_FRAME_LEN * sizeof(short) * 2;

    qoaplay_desc *qp = (qoaplay_desc *)malloc(sizeof(qoaplay_desc) + buffer_size + sample_data_size);
    memset(qp, 0, sizeof(qoaplay_desc));

    qp->first_frame_pos = first_frame_pos;
    qp->file = file;
    qp->buffer = ((unsigned char *)qp) + sizeof(qoaplay_desc);
    qp->sample_data = (short *)(((unsigned char *)qp) + sizeof(qoaplay_desc) + buffer_size);

    qp->info.channels = qoa.channels;
    qp->info.samplerate = qoa.samplerate;
    qp->info.samples = qoa.samples;
    return qp;
}

void qoaplay_close(qoaplay_desc *qp) {
    fclose(qp->file);
    free(qp);
}

unsigned int qoaplay_decode_frame(qoaplay_desc *qp) {
    qp->buffer_len = fread(qp->buffer, 1, qoa_max_frame_size(&qp->info), qp->file);

    unsigned int frame_len;
    qoa_decode_frame(qp->buffer, qp->buffer_len, &qp->info, qp->sample_data, &frame_len);
    qp->sample_data_pos = 0;
    qp->sample_data_len = frame_len;
    return frame_len;
}

void qoaplay_rewind(qoaplay_desc *qp) {
    fseek(qp->file, qp->first_frame_pos, SEEK_SET);
    qp->sample_pos = 0;
    qp->sample_data_len = 0;
    qp->sample_data_pos = 0;
}

unsigned int qoaplay_decode(qoaplay_desc *qp, float *sample_data, int num_samples) {
    int src_index = qp->sample_data_pos * qp->info.channels;
    int dst_index = 0;
    for (int i = 0; i < num_samples; i++) {

        /* Do we have to decode more samples? */
        if (qp->sample_data_len - qp->sample_data_pos == 0) {
            if (!qoaplay_decode_frame(qp)) {
                // Loop to the beginning
                //qoaplay_rewind(qp);
                //qoaplay_decode_frame(qp);
            }
            src_index = 0;
        }

        /* Normalize to -1..1 floats and write to dest */
        for (int c = 0; c < qp->info.channels; c++) {
            sample_data[dst_index++] = qp->sample_data[src_index++] / 32768.0;
        }
        qp->sample_data_pos++;
        qp->sample_pos++;
    }
    return num_samples;
}

double qoaplay_get_duration(qoaplay_desc *qp) {
    return (double)qp->info.samples / (double)qp->info.samplerate;
}

double qoaplay_get_time(qoaplay_desc *qp) {
    return (double)qp->sample_pos / (double)qp->info.samplerate;
}

int qoaplay_get_frame(qoaplay_desc *qp) {
    return qp->sample_pos / QOA_FRAME_LEN;
}

void qoaplay_seek_frame(qoaplay_desc *qp, int frame) {
    if (frame < 0) {
        frame = 0;
    }
    if (frame > qp->info.samples / QOA_FRAME_LEN) {
        frame = qp->info.samples / QOA_FRAME_LEN;
    }

    qp->sample_pos = frame * QOA_FRAME_LEN;
    qp->sample_data_len = 0;
    qp->sample_data_pos = 0;

    unsigned int offset = qp->first_frame_pos + frame * qoa_max_frame_size(&qp->info);
    fseek(qp->file, offset, SEEK_SET);
}
