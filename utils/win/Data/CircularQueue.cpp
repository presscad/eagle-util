#include <string.h>
#include "CircularQueue.h"

int circle_queue_push_back(p_circle_queue_t queue,
		const p_shared_record_t p_record) {
	if (queue->count == queue->limit) {
		return -1;
	}

	queue->count++;
	memcpy(queue->records + queue->rear, p_record, sizeof(shared_record_t));
	queue->rear = (queue->rear + 1) % queue->limit;
	return 0;
}

/*
 * This is a special function. It returns an array of pointers to the records. The function is
 * designed based on the assumption that no other threads/processes are inserting/erasing records
 * in the MIDDLE of the queue.
 * Return value: number of records whose pointers copied to p_records[].
 */
int circle_queue_get_records(p_circle_queue_t queue,
		p_shared_record_t p_records[], int buf_size) {
	const int limit = queue->limit;
	int front = queue->front;
	int count = queue->count;
	if (count > buf_size) {
		count = buf_size;
	}

	for (int i = 0; i < count; i++) {
		p_records[i] = queue->records + ((front + i) % limit);
	}

	return count;
}

void circle_queue_clear(p_circle_queue_t queue, int limit) {
	queue->limit = limit;
	queue->front = 0;
	queue->rear = 0;
	queue->count = 0;
}

/*
 * Erase number of records staring from p_start.
 * Return number of records actually erased.
 */
int circle_queue_bulk_erase(p_circle_queue_t queue, p_shared_record_t p_start,
		int num) {
	if (p_start == NULL || num <= 0) {
		return 0;
	}

	const int limit = queue->limit;
	int front = queue->front;
	int rear = queue->rear < front ? queue->rear + limit : queue->rear;
	int start = p_start - queue->records;
	if (start < 0) {
		return 0;
	} else if (start < front) {
		start += limit;
	}

	if (num > rear - start)
		num = rear - start;

	if (num > 0) {
		if (front == start) {
			queue->front = (queue->front + num) % limit;
		} else {
			// TODO: to optimize using memmove() of big blocks
			for (int n = 0, i = start + num; i < rear; i++, n++) {
				queue->records[(start + n) % limit] = queue->records[(start
						+ num + n) / limit];
			}

			queue->rear = (queue->rear + limit - num) % limit;
		}
		queue->count -= num;
	}

	return num;
}
