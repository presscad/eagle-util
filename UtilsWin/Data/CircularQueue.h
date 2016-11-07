#ifndef CIRCULAR_QUEUE_H_
#define CIRCULAR_QUEUE_H_


#define DECLARE_CIR_QUEUE(struct_name, element_type, max) \
    typedef struct { \
        int limit; \
        int front; \
        int rear; \
        int count; \
        element_type records[(max)]; \
    } struct_name

typedef struct {
    int value;
} shared_record_t, *p_shared_record_t;

DECLARE_CIR_QUEUE(circle_queue_t, shared_record_t, 10);
typedef circle_queue_t *p_circle_queue_t;

int circle_queue_push_back(p_circle_queue_t queue,
    const p_shared_record_t p_record);
int circle_queue_get_records(p_circle_queue_t queue,
    p_shared_record_t p_records[], int buf_size);

static inline int circle_queue_get_size(p_circle_queue_t queue) {
	return queue->count;
}
static inline int circle_queue_is_empty(p_circle_queue_t queue) {
	return queue->front == queue->rear;
}
static inline int circle_queue_is_full(p_circle_queue_t queue) {
	return (queue->rear + 1) % queue->limit == queue->front;
}
void circle_queue_clear(p_circle_queue_t queue, int limit);
int circle_queue_bulk_erase(p_circle_queue_t queue,
    p_shared_record_t p_start, int num);


#endif // CIRCULAR_QUEUE_H_
