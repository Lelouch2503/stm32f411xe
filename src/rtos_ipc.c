/**
 * @file    rtos_ipc.c
 * @brief   Static semaphore, mutex, and queue primitives.
 */

#include "rtos_internal.h"

static void rtos_copy_bytes(void *destination,
                            const void *source,
                            uint32_t length) {
  uint8_t *dst = (uint8_t *)destination;
  const uint8_t *src = (const uint8_t *)source;
  uint32_t index;
  for (index = 0U; index < length; index++) {
    dst[index] = src[index];
  }
}

static int rtos_timeout_valid(uint32_t timeout_ticks) {
  return (timeout_ticks == RTOS_WAIT_FOREVER) ||
         (timeout_ticks <= (uint32_t)INT32_MAX);
}

static void rtos_maybe_preempt(const RTOS_Task_t *task) {
  if (rtos_kernel_should_preempt(task) != 0) {
    rtos_port_pend_context_switch();
  }
}

int rtos_semaphore_init(RTOS_Semaphore_t *semaphore,
                        uint32_t initial_count,
                        uint32_t maximum_count) {
  if ((semaphore == (RTOS_Semaphore_t *)0) || (maximum_count == 0U) ||
      (initial_count > maximum_count)) {
    return RTOS_ERR_INVALID;
  }
  if ((rtos_kernel_is_initialized() == 0) ||
      (rtos_kernel_is_running() != 0)) {
    return RTOS_ERR_STATE;
  }
  semaphore->magic = RTOS_SEMAPHORE_MAGIC;
  semaphore->count = initial_count;
  semaphore->maximum_count = maximum_count;
  return RTOS_OK;
}

int rtos_semaphore_take(RTOS_Semaphore_t *semaphore,
                        uint32_t timeout_ticks) {
  RTOS_Task_t *self;
  uint32_t key;

  if ((semaphore == (RTOS_Semaphore_t *)0) ||
      (semaphore->magic != RTOS_SEMAPHORE_MAGIC) ||
      (rtos_timeout_valid(timeout_ticks) == 0)) {
    return RTOS_ERR_INVALID;
  }
  if (rtos_kernel_thread_api_valid() == 0) {
    return RTOS_ERR_STATE;
  }

  self = rtos_current_task;
  key = rtos_port_enter_critical();
  if (semaphore->count != 0U) {
    semaphore->count--;
    rtos_port_exit_critical(key);
    return RTOS_OK;
  }
  if (timeout_ticks == 0U) {
    rtos_port_exit_critical(key);
    return RTOS_ERR_BUSY;
  }
  (void)rtos_kernel_block_current(RTOS_WAIT_SEMAPHORE, semaphore,
                                  (void *)0, timeout_ticks);
  rtos_port_exit_critical(key);
  return self->wait_result;
}

static int rtos_semaphore_give_common(RTOS_Semaphore_t *semaphore) {
  RTOS_Task_t *waiter =
      rtos_kernel_find_waiter(RTOS_WAIT_SEMAPHORE, semaphore);
  if (waiter != (RTOS_Task_t *)0) {
    rtos_kernel_wake_task(waiter, RTOS_OK);
    rtos_maybe_preempt(waiter);
    return RTOS_OK;
  }
  if (semaphore->count >= semaphore->maximum_count) {
    return RTOS_ERR_BUSY;
  }
  semaphore->count++;
  return RTOS_OK;
}

int rtos_semaphore_give(RTOS_Semaphore_t *semaphore) {
  uint32_t key;
  int rc;
  if ((semaphore == (RTOS_Semaphore_t *)0) ||
      (semaphore->magic != RTOS_SEMAPHORE_MAGIC)) {
    return RTOS_ERR_INVALID;
  }
  if (rtos_kernel_thread_api_valid() == 0) {
    return RTOS_ERR_STATE;
  }
  key = rtos_port_enter_critical();
  rc = rtos_semaphore_give_common(semaphore);
  rtos_port_exit_critical(key);
  return rc;
}

int rtos_semaphore_give_from_isr(RTOS_Semaphore_t *semaphore) {
  uint32_t key;
  int rc;
  if ((semaphore == (RTOS_Semaphore_t *)0) ||
      (semaphore->magic != RTOS_SEMAPHORE_MAGIC)) {
    return RTOS_ERR_INVALID;
  }
  if (rtos_port_is_kernel_aware_isr() == 0) {
    return RTOS_ERR_STATE;
  }
  key = rtos_port_enter_critical();
  rc = rtos_semaphore_give_common(semaphore);
  rtos_port_exit_critical(key);
  return rc;
}

int rtos_mutex_init(RTOS_Mutex_t *mutex) {
  if (mutex == (RTOS_Mutex_t *)0) {
    return RTOS_ERR_INVALID;
  }
  if ((rtos_kernel_is_initialized() == 0) ||
      (rtos_kernel_is_running() != 0)) {
    return RTOS_ERR_STATE;
  }
  mutex->magic = RTOS_MUTEX_MAGIC;
  mutex->owner = (RTOS_Task_t *)0;
  return RTOS_OK;
}

int rtos_mutex_lock(RTOS_Mutex_t *mutex, uint32_t timeout_ticks) {
  RTOS_Task_t *self;
  uint32_t key;

  if ((mutex == (RTOS_Mutex_t *)0) ||
      (mutex->magic != RTOS_MUTEX_MAGIC) ||
      (rtos_timeout_valid(timeout_ticks) == 0)) {
    return RTOS_ERR_INVALID;
  }
  if (rtos_kernel_thread_api_valid() == 0) {
    return RTOS_ERR_STATE;
  }

  self = rtos_current_task;
  key = rtos_port_enter_critical();
  if (mutex->owner == (RTOS_Task_t *)0) {
    mutex->owner = self;
    self->mutex_hold_count++;
    rtos_port_exit_critical(key);
    return RTOS_OK;
  }
  if (mutex->owner == self) {
    rtos_port_exit_critical(key);
    return RTOS_ERR_BUSY;
  }
  if (timeout_ticks == 0U) {
    rtos_port_exit_critical(key);
    return RTOS_ERR_BUSY;
  }

  (void)rtos_kernel_block_current(RTOS_WAIT_MUTEX, mutex, (void *)0,
                                  timeout_ticks);
  rtos_kernel_recompute_priorities();
  rtos_port_exit_critical(key);
  return self->wait_result;
}

int rtos_mutex_unlock(RTOS_Mutex_t *mutex) {
  RTOS_Task_t *self;
  RTOS_Task_t *waiter;
  uint32_t key;

  if ((mutex == (RTOS_Mutex_t *)0) ||
      (mutex->magic != RTOS_MUTEX_MAGIC)) {
    return RTOS_ERR_INVALID;
  }
  if (rtos_kernel_thread_api_valid() == 0) {
    return RTOS_ERR_STATE;
  }

  self = rtos_current_task;
  key = rtos_port_enter_critical();
  if (mutex->owner != self) {
    rtos_port_exit_critical(key);
    return RTOS_ERR_NOT_OWNER;
  }

  if (self->mutex_hold_count != 0U) {
    self->mutex_hold_count--;
  }
  waiter = rtos_kernel_find_waiter(RTOS_WAIT_MUTEX, mutex);
  if (waiter != (RTOS_Task_t *)0) {
    mutex->owner = waiter;
    waiter->mutex_hold_count++;
    rtos_kernel_wake_task(waiter, RTOS_OK);
  } else {
    mutex->owner = (RTOS_Task_t *)0;
  }
  rtos_kernel_recompute_priorities();
  rtos_maybe_preempt(waiter);
  rtos_port_exit_critical(key);
  return RTOS_OK;
}

int rtos_queue_init(RTOS_Queue_t *queue,
                    void *storage,
                    uint32_t capacity,
                    uint32_t item_size) {
  if ((queue == (RTOS_Queue_t *)0) || (storage == (void *)0) ||
      (capacity == 0U) || (item_size == 0U) ||
      (item_size > RTOS_QUEUE_MAX_ITEM_SIZE) ||
      (capacity > (UINT32_MAX / item_size))) {
    return RTOS_ERR_INVALID;
  }
  if ((rtos_kernel_is_initialized() == 0) ||
      (rtos_kernel_is_running() != 0)) {
    return RTOS_ERR_STATE;
  }
  queue->magic = RTOS_QUEUE_MAGIC;
  queue->storage = (uint8_t *)storage;
  queue->capacity = capacity;
  queue->item_size = item_size;
  queue->head = 0U;
  queue->tail = 0U;
  queue->count = 0U;
  return RTOS_OK;
}

static void rtos_queue_push(RTOS_Queue_t *queue, const void *item) {
  rtos_copy_bytes(&queue->storage[queue->tail * queue->item_size], item,
                  queue->item_size);
  queue->tail = (queue->tail + 1U) % queue->capacity;
  queue->count++;
}

static void rtos_queue_pop(RTOS_Queue_t *queue, void *item) {
  rtos_copy_bytes(item, &queue->storage[queue->head * queue->item_size],
                  queue->item_size);
  queue->head = (queue->head + 1U) % queue->capacity;
  queue->count--;
}

static int rtos_queue_send_common(RTOS_Queue_t *queue, const void *item) {
  RTOS_Task_t *receiver =
      rtos_kernel_find_waiter(RTOS_WAIT_QUEUE_RECEIVE, queue);
  if (receiver != (RTOS_Task_t *)0) {
    rtos_copy_bytes(receiver->wait_buffer, item, queue->item_size);
    rtos_kernel_wake_task(receiver, RTOS_OK);
    rtos_maybe_preempt(receiver);
    return RTOS_OK;
  }
  if (queue->count >= queue->capacity) {
    return RTOS_ERR_BUSY;
  }
  rtos_queue_push(queue, item);
  return RTOS_OK;
}

static int rtos_queue_receive_common(RTOS_Queue_t *queue, void *item) {
  RTOS_Task_t *sender;
  if (queue->count == 0U) {
    return RTOS_ERR_BUSY;
  }

  rtos_queue_pop(queue, item);
  sender = rtos_kernel_find_waiter(RTOS_WAIT_QUEUE_SEND, queue);
  if (sender != (RTOS_Task_t *)0) {
    rtos_queue_push(queue, sender->wait_buffer);
    rtos_kernel_wake_task(sender, RTOS_OK);
    rtos_maybe_preempt(sender);
  }
  return RTOS_OK;
}

int rtos_queue_send(RTOS_Queue_t *queue,
                    const void *item,
                    uint32_t timeout_ticks) {
  RTOS_Task_t *self;
  uint32_t key;
  int rc;

  if ((queue == (RTOS_Queue_t *)0) || (item == (const void *)0) ||
      (queue->magic != RTOS_QUEUE_MAGIC) ||
      (rtos_timeout_valid(timeout_ticks) == 0)) {
    return RTOS_ERR_INVALID;
  }
  if (rtos_kernel_thread_api_valid() == 0) {
    return RTOS_ERR_STATE;
  }

  self = rtos_current_task;
  key = rtos_port_enter_critical();
  rc = rtos_queue_send_common(queue, item);
  if ((rc == RTOS_ERR_BUSY) && (timeout_ticks != 0U)) {
    (void)rtos_kernel_block_current(RTOS_WAIT_QUEUE_SEND, queue,
                                    (void *)item, timeout_ticks);
    rtos_port_exit_critical(key);
    return self->wait_result;
  }
  rtos_port_exit_critical(key);
  return rc;
}

int rtos_queue_receive(RTOS_Queue_t *queue,
                       void *item,
                       uint32_t timeout_ticks) {
  RTOS_Task_t *self;
  uint32_t key;
  int rc;

  if ((queue == (RTOS_Queue_t *)0) || (item == (void *)0) ||
      (queue->magic != RTOS_QUEUE_MAGIC) ||
      (rtos_timeout_valid(timeout_ticks) == 0)) {
    return RTOS_ERR_INVALID;
  }
  if (rtos_kernel_thread_api_valid() == 0) {
    return RTOS_ERR_STATE;
  }

  self = rtos_current_task;
  key = rtos_port_enter_critical();
  rc = rtos_queue_receive_common(queue, item);
  if ((rc == RTOS_ERR_BUSY) && (timeout_ticks != 0U)) {
    (void)rtos_kernel_block_current(RTOS_WAIT_QUEUE_RECEIVE, queue, item,
                                    timeout_ticks);
    rtos_port_exit_critical(key);
    return self->wait_result;
  }
  rtos_port_exit_critical(key);
  return rc;
}

int rtos_queue_send_from_isr(RTOS_Queue_t *queue, const void *item) {
  uint32_t key;
  int rc;
  if ((queue == (RTOS_Queue_t *)0) || (item == (const void *)0) ||
      (queue->magic != RTOS_QUEUE_MAGIC)) {
    return RTOS_ERR_INVALID;
  }
  if (rtos_port_is_kernel_aware_isr() == 0) {
    return RTOS_ERR_STATE;
  }
  key = rtos_port_enter_critical();
  rc = rtos_queue_send_common(queue, item);
  rtos_port_exit_critical(key);
  return rc;
}

int rtos_queue_receive_from_isr(RTOS_Queue_t *queue, void *item) {
  uint32_t key;
  int rc;
  if ((queue == (RTOS_Queue_t *)0) || (item == (void *)0) ||
      (queue->magic != RTOS_QUEUE_MAGIC)) {
    return RTOS_ERR_INVALID;
  }
  if (rtos_port_is_kernel_aware_isr() == 0) {
    return RTOS_ERR_STATE;
  }
  key = rtos_port_enter_critical();
  rc = rtos_queue_receive_common(queue, item);
  rtos_port_exit_critical(key);
  return rc;
}
