/*
    ChibiOS - Copyright (C) 2026 Xael South.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    IMX23/IMX23xx/hal_usb_lld.h
 * @brief   i.MX23 USB (ChipIdea ci13xxx) low level driver header.
 *
 * @details Ported from U-Boot drivers/usb/gadget/ci_udc.c and Linux
 *          drivers/usb/chipidea + drivers/usb/phy/phy-mxs-usb.c.
 *          The controller is the ci13xxx OTG core at 0x80080000 with the
 *          analog PHY at 0x8007C000, IRQ 11. Endpoint transfers are
 *          queue-head (QH) + transfer-descriptor (TD) lists in SDRAM.
 *
 * @addtogroup IMX23
 * @{
 */

#ifndef HAL_USB_LLD_H
#define HAL_USB_LLD_H

#include "imx23.h"
#include "icoll.h"

#if (HAL_USE_USB == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/* Number of device endpoints (ci13xxx on i.MX23). */
#define IMX23_USB_NUM_EP        8U

/* Endpoint queue head (64 bytes) and transfer descriptor (32 bytes). */
typedef struct {
  volatile uint32_t config;
  volatile uint32_t current;
  volatile uint32_t next;
  volatile uint32_t info;
  volatile uint32_t page0;
  volatile uint32_t page1;
  volatile uint32_t page2;
  volatile uint32_t page3;
  volatile uint32_t page4;
  volatile uint32_t reserved_0;
  volatile uint8_t  setup_data[8];
  volatile uint32_t reserved_1;
  volatile uint32_t reserved_2;
  volatile uint32_t reserved_3;
  volatile uint32_t reserved_4;
} imx23_usb_qh_t;

typedef struct {
  volatile uint32_t next;
  volatile uint32_t info;
  volatile uint32_t page0;
  volatile uint32_t page1;
  volatile uint32_t page2;
  volatile uint32_t page3;
  volatile uint32_t page4;
  volatile uint32_t reserved;
} imx23_usb_td_t;

/* QH config bits. */
#define IMX23_USB_QH_MAX_PKT(n) ((uint32_t)(n) << 16)
#define IMX23_USB_QH_ZLT        (1U << 29)
#define IMX23_USB_QH_IOS        (1U << 15)

/* TD info bits. */
#define IMX23_USB_TD_TERMINATE  1U
#define IMX23_USB_TD_BYTES(n)   ((uint32_t)(n) << 16)
#define IMX23_USB_TD_IOC        (1U << 15)
#define IMX23_USB_TD_ACTIVE     (1U << 7)
#define IMX23_USB_TD_HALTED     (1U << 6)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

#if !defined(IMX23_USB_USE_USB1) || defined(__DOXYGEN__)
#define IMX23_USB_USE_USB1      TRUE
#endif

#if !defined(IMX23_USB1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define IMX23_USB1_IRQ_PRIORITY 2U
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !IMX23_USB_USE_USB1
#error "USB driver activated but IMX23_USB_USE_USB1 is FALSE"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

#define USB_MAX_ENDPOINTS       IMX23_USB_NUM_EP

/**
 * @brief   Type of an IN endpoint state structure.
 */
typedef struct {
  size_t                        txsize;   /**< Requested transmit size.     */
  size_t                        txcnt;    /**< Transmitted bytes so far.    */
  const uint8_t                 *txbuf;   /**< Transmission buffer.         */
#if (USB_USE_WAIT == TRUE) || defined(__DOXYGEN__)
  thread_reference_t            thread;   /**< Waiting thread.              */
#endif
  size_t                        txlast;   /**< Size of the last packet.     */
} USBInEndpointState;

/**
 * @brief   Type of an OUT endpoint state structure.
 */
typedef struct {
  size_t                        rxsize;   /**< Requested receive size.      */
  size_t                        rxcnt;    /**< Received bytes so far.       */
  uint8_t                       *rxbuf;   /**< Receive buffer.              */
#if (USB_USE_WAIT == TRUE) || defined(__DOXYGEN__)
  thread_reference_t            thread;   /**< Waiting thread.              */
#endif
  size_t                        rxpkts;   /**< Packets still to receive.    */
} USBOutEndpointState;

/**
 * @brief   Type of an USB endpoint configuration structure.
 */
typedef struct {
  uint32_t                      ep_mode;      /**< Type and mode.           */
  usbepcallback_t               setup_cb;     /**< Setup callback.          */
  usbepcallback_t               in_cb;        /**< IN callback.             */
  usbepcallback_t               out_cb;       /**< OUT callback.            */
  uint16_t                      in_maxsize;   /**< IN max packet size.      */
  uint16_t                      out_maxsize;  /**< OUT max packet size.     */
  USBInEndpointState            *in_state;    /**< IN state.                */
  USBOutEndpointState           *out_state;   /**< OUT state.               */
  uint16_t                      ep_buffers;   /**< Reserved.                */
  uint8_t                       *setup_buf;   /**< Setup packet buffer.     */
} USBEndpointConfig;

/**
 * @brief   Type of an USB driver configuration structure.
 */
typedef struct {
  usbeventcb_t                  event_cb;         /**< Events callback.     */
  usbgetdescriptor_t            get_descriptor_cb;/**< GET_DESCRIPTOR cb.   */
#if (USB_USE_EP0_THREAD == FALSE) || defined(__DOXYGEN__)
  usbreqhandler_t               requests_hook_cb; /**< Requests hook.       */
#endif
  usbcallback_t                 sof_cb;           /**< SOF callback.        */
} USBConfig;

/**
 * @brief   Type of a structure representing an USB driver.
 */
struct USBDriver {
  /* Mandatory fields (see hal_usb.h). */
  usbstate_t                    state;
  const USBConfig               *config;
  uint16_t                      transmitting;
  uint16_t                      receiving;
  const USBEndpointConfig       *epc[USB_MAX_ENDPOINTS + 1];
  void                          *in_params[USB_MAX_ENDPOINTS];
  void                          *out_params[USB_MAX_ENDPOINTS];
  usbep0state_t                 ep0state;
  uint8_t                       *ep0next;
  size_t                        ep0n;
  usbcallback_t                 ep0endcb;
  uint8_t                       setup[8];
  uint16_t                      status;
  uint8_t                       address;
  uint8_t                       configuration;
  usbstate_t                    saved_state;
  /* ci13xxx specific fields (QH list is a static 2048-aligned array in
     the .c file). */
  imx23_usb_td_t                td[2U * IMX23_USB_NUM_EP];
  imx23_usb_td_t                *td_in[IMX23_USB_NUM_EP];
  imx23_usb_td_t                *td_out[IMX23_USB_NUM_EP];
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

#define usb_lld_connect_bus(usbp)   do { } while (0)
#define usb_lld_disconnect_bus(usbp) do { } while (0)
#define usb_lld_wakeup_host(usbp)   do { } while (0)
#define usb_lld_get_frame_number(usbp) \
  ((uint16_t)(IMX23_REG(IMX23_USBC_FRINDEX) & 0x7FFU))

/**
 * @brief   Returns the size of the last OUT transaction.
 *
 * @notapi
 */
#define usb_lld_get_transaction_size(usbp, ep)                              \
  ((usbp)->epc[ep]->out_state->rxcnt)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if IMX23_USB_USE_USB1 && !defined(__DOXYGEN__)
extern USBDriver USBD1;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void usb_lld_init(void);
  void usb_lld_start(USBDriver *usbp);
  void usb_lld_stop(USBDriver *usbp);
  void usb_lld_reset(USBDriver *usbp);
  void usb_lld_set_address(USBDriver *usbp);
  void usb_lld_init_endpoint(USBDriver *usbp, usbep_t ep);
  void usb_lld_disable_endpoints(USBDriver *usbp);
  usbepstatus_t usb_lld_get_status_in(USBDriver *usbp, usbep_t ep);
  usbepstatus_t usb_lld_get_status_out(USBDriver *usbp, usbep_t ep);
  void usb_lld_read_setup(USBDriver *usbp, usbep_t ep, uint8_t *buf);
  void usb_lld_start_out(USBDriver *usbp, usbep_t ep);
  void usb_lld_start_in(USBDriver *usbp, usbep_t ep);
  void usb_lld_stall_out(USBDriver *usbp, usbep_t ep);
  void usb_lld_stall_in(USBDriver *usbp, usbep_t ep);
  void usb_lld_clear_out(USBDriver *usbp, usbep_t ep);
  void usb_lld_clear_in(USBDriver *usbp, usbep_t ep);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_USB */

#endif /* HAL_USB_LLD_H */

/** @} */
