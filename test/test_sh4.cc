#include <limits.h>
#include <memory>
#include <unordered_map>
#include <gtest/gtest.h>
#include "core/math.h"
#include "hw/sh4/sh4.h"
#include "hw/dreamcast.h"
#include "hw/memory.h"
#include "hw/scheduler.h"
#include "sys/exception_handler.h"

static const uint32_t UNINITIALIZED_REG = 0xbaadf00d;

typedef struct {
  const char *name;
  const uint8_t *buffer;
  int buffer_size;
  int buffer_offset;
  sh4_context_t in;
  sh4_context_t out;
} sh4_test_t;

typedef struct {
  const char *name;
  size_t offset;
  int size;
} sh4_test_reg_t;

// as per the notes in sh4_context.h, the fr / xf register pairs are swapped
static sh4_test_reg_t sh4_test_regs[] = {
    {"fpscr", offsetof(sh4_context_t, fpscr), 4},
    {"r0", offsetof(sh4_context_t, r[0]), 4},
    {"r1", offsetof(sh4_context_t, r[1]), 4},
    {"r2", offsetof(sh4_context_t, r[2]), 4},
    {"r3", offsetof(sh4_context_t, r[3]), 4},
    {"r4", offsetof(sh4_context_t, r[4]), 4},
    {"r5", offsetof(sh4_context_t, r[5]), 4},
    {"r6", offsetof(sh4_context_t, r[6]), 4},
    {"r7", offsetof(sh4_context_t, r[7]), 4},
    {"r8", offsetof(sh4_context_t, r[8]), 4},
    {"r9", offsetof(sh4_context_t, r[9]), 4},
    {"r10", offsetof(sh4_context_t, r[10]), 4},
    {"r11", offsetof(sh4_context_t, r[11]), 4},
    {"r12", offsetof(sh4_context_t, r[12]), 4},
    {"r13", offsetof(sh4_context_t, r[13]), 4},
    {"r14", offsetof(sh4_context_t, r[14]), 4},
    {"r15", offsetof(sh4_context_t, r[15]), 4},
    {"fr0", offsetof(sh4_context_t, fr[1]), 4},
    {"fr1", offsetof(sh4_context_t, fr[0]), 4},
    {"fr2", offsetof(sh4_context_t, fr[3]), 4},
    {"fr3", offsetof(sh4_context_t, fr[2]), 4},
    {"fr4", offsetof(sh4_context_t, fr[5]), 4},
    {"fr5", offsetof(sh4_context_t, fr[4]), 4},
    {"fr6", offsetof(sh4_context_t, fr[7]), 4},
    {"fr7", offsetof(sh4_context_t, fr[6]), 4},
    {"fr8", offsetof(sh4_context_t, fr[9]), 4},
    {"fr9", offsetof(sh4_context_t, fr[8]), 4},
    {"fr10", offsetof(sh4_context_t, fr[11]), 4},
    {"fr11", offsetof(sh4_context_t, fr[10]), 4},
    {"fr12", offsetof(sh4_context_t, fr[13]), 4},
    {"fr13", offsetof(sh4_context_t, fr[12]), 4},
    {"fr14", offsetof(sh4_context_t, fr[15]), 4},
    {"fr15", offsetof(sh4_context_t, fr[14]), 4},
    {"xf0", offsetof(sh4_context_t, xf[1]), 4},
    {"xf1", offsetof(sh4_context_t, xf[0]), 4},
    {"xf2", offsetof(sh4_context_t, xf[3]), 4},
    {"xf3", offsetof(sh4_context_t, xf[2]), 4},
    {"xf4", offsetof(sh4_context_t, xf[5]), 4},
    {"xf5", offsetof(sh4_context_t, xf[4]), 4},
    {"xf6", offsetof(sh4_context_t, xf[7]), 4},
    {"xf7", offsetof(sh4_context_t, xf[6]), 4},
    {"xf8", offsetof(sh4_context_t, xf[9]), 4},
    {"xf9", offsetof(sh4_context_t, xf[8]), 4},
    {"xf10", offsetof(sh4_context_t, xf[11]), 4},
    {"xf11", offsetof(sh4_context_t, xf[10]), 4},
    {"xf12", offsetof(sh4_context_t, xf[13]), 4},
    {"xf13", offsetof(sh4_context_t, xf[12]), 4},
    {"xf14", offsetof(sh4_context_t, xf[15]), 4},
    {"xf15", offsetof(sh4_context_t, xf[14]), 4},
};
int sh4_num_test_regs =
    static_cast<int>(sizeof(sh4_test_regs) / sizeof(sh4_test_regs[0]));

static void run_sh4_test(const sh4_test_t &test) {
  dreamcast_t *dc = dc_create(nullptr);
  CHECK_NOTNULL(dc);

  // setup in registers
  for (int i = 0; i < sh4_num_test_regs; i++) {
    sh4_test_reg_t &reg = sh4_test_regs[i];

    uint32_t input = *reinterpret_cast<const uint32_t *>(
        reinterpret_cast<const uint8_t *>(&test.in) + reg.offset);

    if (input == UNINITIALIZED_REG) {
      continue;
    }

    *reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(&dc->sh4->ctx) +
                                  reg.offset) = input;
  }

  // setup initial stack pointer
  dc->sh4->ctx.r[15] = 0x8d000000;

  // load binary. note, Memory::Memcpy only support 4 byte aligned sizes
  int aligned_size = align_up(test.buffer_size, 4);
  uint8_t *aligned_buffer = reinterpret_cast<uint8_t *>(alloca(aligned_size));
  memcpy(aligned_buffer, test.buffer, test.buffer_size);
  as_memcpy_to_guest(dc->sh4->base.memory->space, 0x8c010000, aligned_buffer,
                     aligned_size);

  // skip to the test's offset
  sh4_set_pc(dc->sh4, 0x8c010000 + test.buffer_offset);

  // run until the function returns
  while (dc->sh4->ctx.pc) {
    sh4_run(dc->sh4, 1);
  }

  // validate out registers
  for (int i = 0; i < sh4_num_test_regs; i++) {
    sh4_test_reg_t &reg = sh4_test_regs[i];

    uint32_t expected = *reinterpret_cast<const uint32_t *>(
        reinterpret_cast<const uint8_t *>(&test.out) + reg.offset);

    if (expected == UNINITIALIZED_REG) {
      continue;
    }

    uint32_t actual = *reinterpret_cast<const uint32_t *>(
        reinterpret_cast<const uint8_t *>(&dc->sh4->ctx) + reg.offset);

    ASSERT_EQ(expected, actual) << reg.name << " expected: 0x" << std::hex
                                << expected << ", actual 0x" << actual;
  }

  dc_destroy(dc);
}

// clang-format off
#define INIT_CONTEXT(fpscr, r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, \
                     r12, r13, r14, r15, fr0, fr1, fr2, fr3, fr4, fr5, fr6,   \
                     fr7, fr8, fr9, fr10, fr11, fr12, fr13, fr14, fr15, xf0,  \
                     xf1, xf2, xf3, xf4, xf5, xf6, xf7, xf8, xf9, xf10, xf11, \
                     xf12, xf13, xf14, xf15)                                  \
  sh4_context_t {                                                                \
    nullptr, nullptr, nullptr, nullptr, nullptr,                              \
    0, 0,                                                                     \
    0, 0, 0, 0, fpscr,                                                        \
    0, 0, 0,                                                                  \
    0, 0, 0,                                                                  \
    0, 0, 0,                                                                  \
    { {0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0 } },                  \
    {r0, r1, r2,  r3,  r4,  r5,  r6,  r7,                                     \
     r8, r9, r10, r11, r12, r13, r14, r15},                                   \
    {0, 0, 0, 0, 0, 0, 0, 0},                                                 \
    {fr1, fr0, fr3,  fr2,  fr5,  fr4,  fr7,  fr6,                             \
     fr9, fr8, fr11, fr10, fr13, fr12, fr15, fr14},                           \
    {xf1, xf0, xf3,  xf2,  xf5,  xf4,  xf7,  xf6,                             \
     xf9, xf8, xf11, xf10, xf13, xf12, xf15, xf14},                           \
  }

#define TEST_SH4(name, buffer, buffer_size, buffer_offset,                                                                                                               \
  fpscr_in,                                                                                                                                                              \
  r0_in,  r1_in,  r2_in,  r3_in,  r4_in,  r5_in,   r6_in,   r7_in,  r8_in,  r9_in,  r10_in,  r11_in,  r12_in,  r13_in,  r14_in,  r15_in,                                 \
  fr0_in, fr1_in, fr2_in, fr3_in, fr4_in, fr5_in,  fr6_in,  fr7_in, fr8_in, fr9_in, fr10_in, fr11_in, fr12_in, fr13_in, fr14_in, fr15_in,                                \
  xf0_in, xf1_in, xf2_in, xf3_in, xf4_in, xf5_in,  xf6_in,  xf7_in, xf8_in, xf9_in, xf10_in, xf11_in, xf12_in, xf13_in, xf14_in, xf15_in,                                \
  fpscr_out,                                                                                                                                                             \
  r0_out,  r1_out,  r2_out,  r3_out,  r4_out,  r5_out,   r6_out,   r7_out,  r8_out,  r9_out,  r10_out,  r11_out,  r12_out,  r13_out,  r14_out,  r15_out,                 \
  fr0_out, fr1_out, fr2_out, fr3_out, fr4_out, fr5_out,  fr6_out,  fr7_out, fr8_out, fr9_out, fr10_out, fr11_out, fr12_out, fr13_out, fr14_out, fr15_out,                \
  xf0_out, xf1_out, xf2_out, xf3_out, xf4_out, xf5_out,  xf6_out,  xf7_out, xf8_out, xf9_out, xf10_out, xf11_out, xf12_out, xf13_out, xf14_out, xf15_out)                \
  static sh4_test_t test_##name = {                                                                                                                                         \
    #name, buffer, buffer_size, buffer_offset,                                                                                                                           \
    INIT_CONTEXT(fpscr_in,                                                                                                                                               \
                 r0_in,  r1_in,  r2_in,  r3_in,  r4_in,  r5_in,   r6_in,   r7_in,  r8_in,  r9_in,  r10_in,  r11_in,  r12_in,  r13_in,  r14_in,  r15_in,                  \
                 fr0_in, fr1_in, fr2_in, fr3_in, fr4_in, fr5_in,  fr6_in,  fr7_in, fr8_in, fr9_in, fr10_in, fr11_in, fr12_in, fr13_in, fr14_in, fr15_in,                 \
                 xf0_in, xf1_in, xf2_in, xf3_in, xf4_in, xf5_in,  xf6_in,  xf7_in, xf8_in, xf9_in, xf10_in, xf11_in, xf12_in, xf13_in, xf14_in, xf15_in),                \
    INIT_CONTEXT(fpscr_out,                                                                                                                                              \
                 r0_out,  r1_out,  r2_out,  r3_out,  r4_out,  r5_out,   r6_out,   r7_out,  r8_out,  r9_out,  r10_out,  r11_out,  r12_out,  r13_out,  r14_out,  r15_out,  \
                 fr0_out, fr1_out, fr2_out, fr3_out, fr4_out, fr5_out,  fr6_out,  fr7_out, fr8_out, fr9_out, fr10_out, fr11_out, fr12_out, fr13_out, fr14_out, fr15_out, \
                 xf0_out, xf1_out, xf2_out, xf3_out, xf4_out, xf5_out,  xf6_out,  xf7_out, xf8_out, xf9_out, xf10_out, xf11_out, xf12_out, xf13_out, xf14_out, xf15_out) \
  };                                                                                                                                                                     \
  TEST(sh4_x64, name) {                                                                                                                                                  \
    exception_handler_install();                                                                                                                                         \
    run_sh4_test(test_##name);                                                                                                                                           \
    exception_handler_uninstall();                                                                                                                                       \
  }
#include "test_sh4.inc"
#undef TEST_SH4
// clang-format on
