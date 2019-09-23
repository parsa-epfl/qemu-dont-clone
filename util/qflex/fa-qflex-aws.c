// Amazon FPGA Hardware Development Kit
//
// Copyright 2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Amazon Software License (the "License"). You may not use
// this file except in compliance with the License. A copy of the License is
// located at
//
//    http://aws.amazon.com/asl/
//
// or in the "license" file accompanying this file. This file is distributed on
// an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, express or
// implied. See the License for the specific language governing permissions and
// limitations under the License.

#ifdef CONFIG_FPGA

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <fpga_pci.h>
#include <fpga_mgmt.h>
#include <utils/lcd.h>

/* Constants determined by the CL */

#define GPIO_REG_ADDR UINT64_C(0x0)

#define BASE_ADDR UINT64_C(0x0C0000000)

/*
 * pci_vendor_id and pci_device_id values below are Amazon's and avaliable to use for a given FPGA slot.
 * Users may replace these with their own if allocated to them by PCI SIG
 */
static uint16_t pci_vendor_id = 0x1D0F; /* Amazon PCI Vendor ID */
static uint16_t pci_device_id = 0xF000; /* PCI Device ID preassigned by Amazon for F1 applications */

static pci_bar_handle_t pci_bar_handle = PCI_BAR_HANDLE_INIT;
static pci_bar_handle_t pci_bar_handle_pcis = PCI_BAR_HANDLE_INIT;

/* Declaring the local functions */

int peek_poke_example(int slot, int pf_id, int bar_id);
int vled_example(int slot);

/* Declating auxilary house keeping functions */
int initialize_log(char* log_name);
int check_afi_ready(int slot);

/*
 * check if the corresponding AFI for test_cl is loaded
 */

static int check_afi_ready(int slot_id) {
    struct fpga_mgmt_image_info info = {0};
    int rc;

    /* get local image description, contains status, vendor id, and device id. */
    rc = fpga_mgmt_describe_local_image(slot_id, &info,0);
    fail_on(rc, out, "Unable to get AFI information from slot %d. Are you running as root?",slot_id);

    /* check to see if the slot is ready */
    if (info.status != FPGA_STATUS_LOADED) {
        rc = 1;
        fail_on(rc, out, "AFI in Slot %d is not in READY state !", slot_id);
    }

    printf("AFI PCI  Vendor ID: 0x%x, Device ID 0x%x\n",
        info.spec.map[FPGA_APP_PF].vendor_id,
        info.spec.map[FPGA_APP_PF].device_id);

    /* confirm that the AFI that we expect is in fact loaded */
    if (info.spec.map[FPGA_APP_PF].vendor_id != pci_vendor_id ||
        info.spec.map[FPGA_APP_PF].device_id != pci_device_id) {
        printf("AFI does not show expected PCI vendor id and device ID. If the AFI "
               "was just loaded, it might need a rescan. Rescanning now.\n");

        rc = fpga_pci_rescan_slot_app_pfs(slot_id);
        fail_on(rc, out, "Unable to update PF for slot %d",slot_id);
        /* get local image description, contains status, vendor id, and device id. */
        rc = fpga_mgmt_describe_local_image(slot_id, &info,0);
        fail_on(rc, out, "Unable to get AFI information from slot %d",slot_id);

        printf("AFI PCI  Vendor ID: 0x%x, Device ID 0x%x\n",
            info.spec.map[FPGA_APP_PF].vendor_id,
            info.spec.map[FPGA_APP_PF].device_id);

        /* confirm that the AFI that we expect is in fact loaded after rescan */
        if (info.spec.map[FPGA_APP_PF].vendor_id != pci_vendor_id ||
             info.spec.map[FPGA_APP_PF].device_id != pci_device_id) {
            rc = 1;
            fail_on(rc, out, "The PCI vendor id and device of the loaded AFI are not "
                             "the expected values.");
        }
    }

    return rc;

out:
    return 1;
}

static int fpga_clean_up(void) {
    int rc, rc_pcis;
    /* clean up */
    if (pci_bar_handle >= 0) {
        rc = fpga_pci_detach(pci_bar_handle);
        if (rc) {
            fprintf(stderr, "Failure while detaching from the fpga.\n");
        }
    }

    if (pci_bar_handle_pcis >= 0) {
        rc_pcis = fpga_pci_detach(pci_bar_handle_pcis);
        if (rc_pcis) {
            fprintf(stderr, "Failure while detaching from the fpga.\n");
        }
    }
}


int init_aws(void) {
    int rc;
    int slot_id;
    int pf_id = FPGA_APP_PF;
    int bar_id = APP_PF_BAR1;

    /* initialize the fpga_pci library so we could have access to FPGA PCIe from this applications */
    rc = fpga_pci_init();
    fail_on(rc, out, "Unable to initialize the fpga_pci library");

    /* This demo works with single FPGA slot, we pick slot #0 as it works for both f1.2xl and f1.16xl */

    slot_id = 0;

    rc = check_afi_ready(slot_id);
    if(rc) {
        fprintf(stderr, "AFI not ready");
        exit(1);
    }


    /* pci_bar_handle_t is a handler for an address space exposed by one PCI BAR on one of the PCI PFs of the FPGA */

    /* attach to the fpga, with a pci_bar_handle out param
     * To attach to multiple slots or BARs, call this function multiple times,
     * saving the pci_bar_handle to specify which address space to interact with in
     * other API calls.
     * This function accepts the slot_id, physical function, and bar number
     */
    rc = fpga_pci_attach(slot_id, pf_id, bar_id, 0, &pci_bar_handle);
    fail_on(rc, out, "Unable to attach to the AFI on slot id %d", slot_id);

    rc_pcis = fpga_pci_attach(slot_id, pf_id, 4, 0, &pci_bar_handle_pcis);
    fail_on(rc_pcis, out, "Unable to attach to the AFI on slot id %d", slot_id);

    printf("\n");
}

int poke_fpga(uint64_t base_addr, uint64_t offst, uint32_t val) {
    int rc_pcis;
    rc_pcis = fpga_pci_poke(pci_bar_handle_pcis, (base_addr + offst), val);
    if(rc_pcis) {
        fprintf(stderr, "Unable to write to the fpga !");
        fpga_clean_up();
    }
}

uint32_t peek_fpga(uint64_t base_addr, uint64_t offst) {
    uint32_t value;
    rc_pcis = fpga_pci_peek(pci_bar_handle_pcis, (base_addr + offst), &value);
    if(rc_pcis) {
        fprintf(stderr, "Unable to write to the fpga !");
        fpga_clean_up();
    }
    return value;
}


#endif /* CONFIG_FPGA */
