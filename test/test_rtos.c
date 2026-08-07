#include "unity.h"
#include "rtos.h"
#include "rtos_internal.h"

void rtos_host_set_isr(uint32_t ipsr, int kernel_aware);

static RTOS_Config_t config = {
    .tick_hz = 1000U,
    .max_syscall_irq_priority = 5U,
};

static void empty_task(void *argument) { (void)argument; }

void setUp(void) {
  rtos_test_reset();
  rtos_test_set_tick(0U);
  rtos_host_set_isr(0U, 0);
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_init(&config));
}

void tearDown(void) {}

void test_task_creation_validates_and_builds_initial_frame(void) {
  RTOS_Task_t task;
  RTOS_STACK_DEFINE(stack, RTOS_MIN_STACK_WORDS);

  TEST_ASSERT_EQUAL_INT(RTOS_ERR_INVALID,
                        rtos_task_create_static(&task, empty_task, 0, stack,
                                                RTOS_MIN_STACK_WORDS, 0U,
                                                "invalid"));
  TEST_ASSERT_EQUAL_INT(RTOS_OK,
                        rtos_task_create_static(&task, empty_task, 0, stack,
                                                RTOS_MIN_STACK_WORDS, 3U,
                                                "valid"));
  TEST_ASSERT_EQUAL_PTR(&stack[RTOS_MIN_STACK_WORDS - 16U],
                        task.stack_pointer);
  TEST_ASSERT_EQUAL_HEX32(RTOS_STACK_CANARY, stack[0]);
  TEST_ASSERT_EQUAL_HEX32(0x01000000UL,
                          stack[RTOS_MIN_STACK_WORDS - 1U]);
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFDUL, task.exc_return);
}

void test_scheduler_selects_highest_priority_and_round_robins(void) {
  RTOS_Task_t low;
  RTOS_Task_t high_a;
  RTOS_Task_t high_b;
  RTOS_STACK_DEFINE(low_stack, RTOS_MIN_STACK_WORDS);
  RTOS_STACK_DEFINE(high_a_stack, RTOS_MIN_STACK_WORDS);
  RTOS_STACK_DEFINE(high_b_stack, RTOS_MIN_STACK_WORDS);

  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &low, empty_task, 0, low_stack, RTOS_MIN_STACK_WORDS, 1U, "low"));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &high_a, empty_task, 0, high_a_stack, RTOS_MIN_STACK_WORDS, 5U, "a"));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &high_b, empty_task, 0, high_b_stack, RTOS_MIN_STACK_WORDS, 5U, "b"));

  TEST_ASSERT_EQUAL_PTR(&high_a, rtos_test_select_next());
  TEST_ASSERT_EQUAL_PTR(&high_b, rtos_test_select_next());
  TEST_ASSERT_EQUAL_PTR(&high_a, rtos_test_select_next());
}

void test_delay_expiry_handles_tick_wrap(void) {
  RTOS_Task_t task;
  RTOS_STACK_DEFINE(stack, RTOS_MIN_STACK_WORDS);
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &task, empty_task, 0, stack, RTOS_MIN_STACK_WORDS, 2U, "delay"));
  rtos_test_set_running(&task);
  rtos_test_set_tick(0xFFFFFFFEUL);

  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_delay(4U));
  TEST_ASSERT_EQUAL_INT(RTOS_TASK_DELAYED, task.state);
  rtos_test_set_tick(1U);
  rtos_test_tick();
  TEST_ASSERT_EQUAL_INT(RTOS_TASK_DELAYED, task.state);
  rtos_test_set_tick(2U);
  rtos_test_tick();
  TEST_ASSERT_EQUAL_INT(RTOS_TASK_READY, task.state);
}

void test_semaphore_wakes_highest_priority_waiter(void) {
  RTOS_Task_t low;
  RTOS_Task_t high;
  RTOS_Task_t giver;
  RTOS_Semaphore_t semaphore;
  RTOS_STACK_DEFINE(low_stack, RTOS_MIN_STACK_WORDS);
  RTOS_STACK_DEFINE(high_stack, RTOS_MIN_STACK_WORDS);
  RTOS_STACK_DEFINE(giver_stack, RTOS_MIN_STACK_WORDS);

  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_semaphore_init(&semaphore, 0U, 1U));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &low, empty_task, 0, low_stack, RTOS_MIN_STACK_WORDS, 2U, "low"));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &high, empty_task, 0, high_stack, RTOS_MIN_STACK_WORDS, 6U, "high"));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &giver, empty_task, 0, giver_stack, RTOS_MIN_STACK_WORDS, 1U, "give"));

  rtos_test_set_running(&low);
  TEST_ASSERT_EQUAL_INT(RTOS_OK,
                        rtos_semaphore_take(&semaphore, RTOS_WAIT_FOREVER));
  rtos_test_set_running(&high);
  TEST_ASSERT_EQUAL_INT(RTOS_OK,
                        rtos_semaphore_take(&semaphore, RTOS_WAIT_FOREVER));
  rtos_test_set_running(&giver);
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_semaphore_give(&semaphore));

  TEST_ASSERT_EQUAL_INT(RTOS_TASK_READY, high.state);
  TEST_ASSERT_EQUAL_INT(RTOS_TASK_BLOCKED, low.state);
  TEST_ASSERT_EQUAL_UINT32(0U, semaphore.count);
}

void test_mutex_priority_inheritance_is_transitive(void) {
  RTOS_Task_t low;
  RTOS_Task_t middle;
  RTOS_Task_t high;
  RTOS_Mutex_t mutex_a;
  RTOS_Mutex_t mutex_b;
  RTOS_STACK_DEFINE(low_stack, RTOS_MIN_STACK_WORDS);
  RTOS_STACK_DEFINE(middle_stack, RTOS_MIN_STACK_WORDS);
  RTOS_STACK_DEFINE(high_stack, RTOS_MIN_STACK_WORDS);

  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_mutex_init(&mutex_a));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_mutex_init(&mutex_b));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &low, empty_task, 0, low_stack, RTOS_MIN_STACK_WORDS, 1U, "low"));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &middle, empty_task, 0, middle_stack, RTOS_MIN_STACK_WORDS, 3U, "mid"));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &high, empty_task, 0, high_stack, RTOS_MIN_STACK_WORDS, 6U, "high"));

  rtos_test_set_running(&low);
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_mutex_lock(&mutex_a, 0U));
  rtos_test_set_running(&middle);
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_mutex_lock(&mutex_b, 0U));
  TEST_ASSERT_EQUAL_INT(RTOS_OK,
                        rtos_mutex_lock(&mutex_a, RTOS_WAIT_FOREVER));
  TEST_ASSERT_EQUAL_UINT8(3U, low.effective_priority);
  rtos_test_set_running(&high);
  TEST_ASSERT_EQUAL_INT(RTOS_OK,
                        rtos_mutex_lock(&mutex_b, RTOS_WAIT_FOREVER));
  TEST_ASSERT_EQUAL_UINT8(6U, middle.effective_priority);
  TEST_ASSERT_EQUAL_UINT8(6U, low.effective_priority);

  rtos_test_set_running(&low);
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_mutex_unlock(&mutex_a));
  TEST_ASSERT_EQUAL_PTR(&middle, mutex_a.owner);
  TEST_ASSERT_EQUAL_UINT8(1U, low.effective_priority);
  TEST_ASSERT_EQUAL_UINT8(6U, middle.effective_priority);
}

void test_queue_directly_hands_item_to_waiting_receiver(void) {
  RTOS_Task_t receiver;
  RTOS_Task_t sender;
  RTOS_Queue_t queue;
  uint32_t storage[2];
  uint32_t received = 0U;
  uint32_t sent = 0x12345678UL;
  RTOS_STACK_DEFINE(receiver_stack, RTOS_MIN_STACK_WORDS);
  RTOS_STACK_DEFINE(sender_stack, RTOS_MIN_STACK_WORDS);

  TEST_ASSERT_EQUAL_INT(RTOS_OK,
                        rtos_queue_init(&queue, storage, 2U, sizeof(uint32_t)));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &receiver, empty_task, 0, receiver_stack, RTOS_MIN_STACK_WORDS, 4U,
      "rx"));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &sender, empty_task, 0, sender_stack, RTOS_MIN_STACK_WORDS, 2U, "tx"));

  rtos_test_set_running(&receiver);
  TEST_ASSERT_EQUAL_INT(RTOS_OK,
                        rtos_queue_receive(&queue, &received,
                                           RTOS_WAIT_FOREVER));
  rtos_test_set_running(&sender);
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_queue_send(&queue, &sent, 0U));
  TEST_ASSERT_EQUAL_HEX32(sent, received);
  TEST_ASSERT_EQUAL_INT(RTOS_TASK_READY, receiver.state);
  TEST_ASSERT_EQUAL_UINT32(0U, queue.count);
}

void test_queue_fifo_wrap_and_busy_errors(void) {
  RTOS_Task_t task;
  RTOS_Queue_t queue;
  uint8_t storage[2];
  uint8_t value;
  uint8_t one = 1U;
  uint8_t two = 2U;
  uint8_t three = 3U;
  RTOS_STACK_DEFINE(stack, RTOS_MIN_STACK_WORDS);

  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_queue_init(&queue, storage, 2U, 1U));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &task, empty_task, 0, stack, RTOS_MIN_STACK_WORDS, 2U, "queue"));
  rtos_test_set_running(&task);

  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_queue_send(&queue, &one, 0U));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_queue_send(&queue, &two, 0U));
  TEST_ASSERT_EQUAL_INT(RTOS_ERR_BUSY,
                        rtos_queue_send(&queue, &three, 0U));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_queue_receive(&queue, &value, 0U));
  TEST_ASSERT_EQUAL_UINT8(one, value);
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_queue_send(&queue, &three, 0U));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_queue_receive(&queue, &value, 0U));
  TEST_ASSERT_EQUAL_UINT8(two, value);
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_queue_receive(&queue, &value, 0U));
  TEST_ASSERT_EQUAL_UINT8(three, value);
  TEST_ASSERT_EQUAL_INT(RTOS_ERR_BUSY,
                        rtos_queue_receive(&queue, &value, 0U));
}

void test_blocked_wait_times_out_at_wrap_safe_deadline(void) {
  RTOS_Task_t task;
  RTOS_Semaphore_t semaphore;
  RTOS_STACK_DEFINE(stack, RTOS_MIN_STACK_WORDS);

  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_semaphore_init(&semaphore, 0U, 1U));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &task, empty_task, 0, stack, RTOS_MIN_STACK_WORDS, 3U, "timeout"));
  rtos_test_set_running(&task);
  rtos_test_set_tick(0xFFFFFFFEUL);
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_semaphore_take(&semaphore, 4U));
  TEST_ASSERT_EQUAL_INT(RTOS_TASK_BLOCKED, task.state);
  rtos_test_set_tick(1U);
  rtos_test_tick();
  TEST_ASSERT_EQUAL_INT(RTOS_TASK_BLOCKED, task.state);
  rtos_test_set_tick(2U);
  rtos_test_tick();
  TEST_ASSERT_EQUAL_INT(RTOS_TASK_READY, task.state);
  TEST_ASSERT_EQUAL_INT(RTOS_ERR_TIMEOUT, task.wait_result);
}

void test_from_isr_api_enforces_kernel_aware_context(void) {
  RTOS_Task_t task;
  RTOS_Semaphore_t semaphore;
  RTOS_STACK_DEFINE(stack, RTOS_MIN_STACK_WORDS);

  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_semaphore_init(&semaphore, 0U, 1U));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &task, empty_task, 0, stack, RTOS_MIN_STACK_WORDS, 2U, "isr"));
  rtos_test_set_running(&task);
  rtos_host_set_isr(18U, 0);
  TEST_ASSERT_EQUAL_INT(RTOS_ERR_STATE,
                        rtos_semaphore_give_from_isr(&semaphore));
  rtos_host_set_isr(18U, 1);
  TEST_ASSERT_EQUAL_INT(RTOS_OK,
                        rtos_semaphore_give_from_isr(&semaphore));
  TEST_ASSERT_EQUAL_INT(RTOS_ERR_BUSY,
                        rtos_semaphore_give_from_isr(&semaphore));
}

void test_task_limit_and_duplicate_storage_are_rejected(void) {
  RTOS_Task_t tasks[RTOS_MAX_TASKS + 1U];
  static RTOS_StackWord_t stacks[RTOS_MAX_TASKS + 1U]
                                      [RTOS_MIN_STACK_WORDS]
      __attribute__((aligned(8)));
  uint32_t index;

  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &tasks[0], empty_task, 0, stacks[0], RTOS_MIN_STACK_WORDS, 1U, "0"));
  TEST_ASSERT_EQUAL_INT(RTOS_ERR_INVALID, rtos_task_create_static(
      &tasks[0], empty_task, 0, stacks[1], RTOS_MIN_STACK_WORDS, 1U, "dup"));
  TEST_ASSERT_EQUAL_INT(RTOS_ERR_INVALID, rtos_task_create_static(
      &tasks[1], empty_task, 0, stacks[0], RTOS_MIN_STACK_WORDS, 1U, "stack"));

  for (index = 1U; index < RTOS_MAX_TASKS; index++) {
    TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
        &tasks[index], empty_task, 0, stacks[index], RTOS_MIN_STACK_WORDS,
        1U, "task"));
  }
  TEST_ASSERT_EQUAL_INT(RTOS_ERR_LIMIT, rtos_task_create_static(
      &tasks[RTOS_MAX_TASKS], empty_task, 0, stacks[RTOS_MAX_TASKS],
      RTOS_MIN_STACK_WORDS, 1U, "overflow"));
}

void test_mutex_is_non_recursive_and_checks_owner(void) {
  RTOS_Task_t owner;
  RTOS_Task_t other;
  RTOS_Mutex_t mutex;
  RTOS_STACK_DEFINE(owner_stack, RTOS_MIN_STACK_WORDS);
  RTOS_STACK_DEFINE(other_stack, RTOS_MIN_STACK_WORDS);

  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_mutex_init(&mutex));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &owner, empty_task, 0, owner_stack, RTOS_MIN_STACK_WORDS, 2U, "owner"));
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_task_create_static(
      &other, empty_task, 0, other_stack, RTOS_MIN_STACK_WORDS, 3U, "other"));
  rtos_test_set_running(&owner);
  TEST_ASSERT_EQUAL_INT(RTOS_OK, rtos_mutex_lock(&mutex, 0U));
  TEST_ASSERT_EQUAL_INT(RTOS_ERR_BUSY, rtos_mutex_lock(&mutex, 0U));
  rtos_test_set_running(&other);
  TEST_ASSERT_EQUAL_INT(RTOS_ERR_NOT_OWNER, rtos_mutex_unlock(&mutex));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_task_creation_validates_and_builds_initial_frame);
  RUN_TEST(test_scheduler_selects_highest_priority_and_round_robins);
  RUN_TEST(test_delay_expiry_handles_tick_wrap);
  RUN_TEST(test_semaphore_wakes_highest_priority_waiter);
  RUN_TEST(test_mutex_priority_inheritance_is_transitive);
  RUN_TEST(test_queue_directly_hands_item_to_waiting_receiver);
  RUN_TEST(test_queue_fifo_wrap_and_busy_errors);
  RUN_TEST(test_blocked_wait_times_out_at_wrap_safe_deadline);
  RUN_TEST(test_from_isr_api_enforces_kernel_aware_context);
  RUN_TEST(test_task_limit_and_duplicate_storage_are_rejected);
  RUN_TEST(test_mutex_is_non_recursive_and_checks_owner);
  return UNITY_END();
}
