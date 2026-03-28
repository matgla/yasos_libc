#include "utest.h"

#include <sys/resource.h>
#include <sys/syscall.h>

#include "syscalls_stub.h"

int sut_prlimit(pid_t pid, int resource, const struct rlimit *new_limit,
                struct rlimit *old_limit);
int sut_getrlimit(int resource, struct rlimit *old_limit);
int sut_setrlimit(int resource, const struct rlimit *new_limit);

static void capture_success(int number, const void *args,
                            syscall_result *result) {
  (void)args;
  result->result = 0;
  result->err = -1;
}

UTEST(resource, prlimit_uses_prlimit_syscall_context) {
  struct rlimit new_limit = {.rlim_cur = 16, .rlim_max = 32};
  struct rlimit old_limit = {0};

  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = capture_success;

  ASSERT_EQ(0, sut_prlimit(123, RLIMIT_NOFILE, &new_limit, &old_limit));
  ASSERT_EQ(1u, trigger_supervisor_call_fake.call_count);
  ASSERT_EQ(sys_prlimit, trigger_supervisor_call_fake.arg0_val);

  const prlimit_context *ctx =
      (const prlimit_context *)trigger_supervisor_call_fake.arg1_val;
  ASSERT_EQ(123, ctx->pid);
  ASSERT_EQ(RLIMIT_NOFILE, ctx->resource);
  ASSERT_EQ(&new_limit, ctx->new_limit);
  ASSERT_EQ(&old_limit, ctx->old_limit);
}

UTEST(resource, getrlimit_routes_through_prlimit_for_current_process) {
  struct rlimit old_limit = {0};

  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = capture_success;

  ASSERT_EQ(0, sut_getrlimit(RLIMIT_STACK, &old_limit));
  ASSERT_EQ(1u, trigger_supervisor_call_fake.call_count);
  ASSERT_EQ(sys_prlimit, trigger_supervisor_call_fake.arg0_val);

  const prlimit_context *ctx =
      (const prlimit_context *)trigger_supervisor_call_fake.arg1_val;
  ASSERT_EQ(0, ctx->pid);
  ASSERT_EQ(RLIMIT_STACK, ctx->resource);
  ASSERT_EQ((const struct rlimit *)0, ctx->new_limit);
  ASSERT_EQ(&old_limit, ctx->old_limit);
}

UTEST(resource, setrlimit_routes_through_prlimit_for_current_process) {
  struct rlimit new_limit = {.rlim_cur = 1, .rlim_max = 2};

  RESET_FAKE(trigger_supervisor_call);
  trigger_supervisor_call_fake.custom_fake = capture_success;

  ASSERT_EQ(0, sut_setrlimit(RLIMIT_CPU, &new_limit));
  ASSERT_EQ(1u, trigger_supervisor_call_fake.call_count);
  ASSERT_EQ(sys_prlimit, trigger_supervisor_call_fake.arg0_val);

  const prlimit_context *ctx =
      (const prlimit_context *)trigger_supervisor_call_fake.arg1_val;
  ASSERT_EQ(0, ctx->pid);
  ASSERT_EQ(RLIMIT_CPU, ctx->resource);
  ASSERT_EQ(&new_limit, ctx->new_limit);
  ASSERT_EQ((struct rlimit *)0, ctx->old_limit);
}