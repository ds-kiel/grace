#ifndef CHUNKED_OUTPUT_H
#define CHUNKED_OUTPUT_H

#define CHUNK_SIZE 10000000 //in Byte

typedef struct chunked_output chunked_output_t;

chunked_output_t* chunked_output_new();
int chunked_output_init(chunked_output_t *process, const gchar* data_pat);
int chunked_output_deinit(chunked_output_t *process);


#endif /* CHUNKED_OUTPUT_H */
