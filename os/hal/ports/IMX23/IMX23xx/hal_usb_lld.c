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
 * @file    IMX23/IMX23xx/hal_usb_lld.c
 * @brief   i.MX23 USB (ChipIdea ci13xxx) low level driver.
 *
 * @details Ported from U-Boot drivers/usb/gadget/ci_udc.c (QH/TD model,
 *          register bits) and Linux drivers/usb/phy/phy-mxs-usb.c +
 *          drivers/usb/chipidea/ci_hdrc_imx.c (PHY and clock bring-up).
 *
 * @addtogroup IMX23
 * @{
 */

#include "hal.h"
#include "hal_usb.h"

#if (HAL_USE_USB == TRUE) && (IMX23_USB_USE_USB1 == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/* EP0 max packet size. */
#define EP0_MAX_PACKET_SIZE     64U
/* Non-control endpoint max packet size (high speed bulk). */
#define EP_MAX_PACKET_SIZE      512U
/* Max bytes per TD (5 buffer pages of 4KB). */
#define EP_MAX_LENGTH_TRANSFER  0x5000U

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

USBDriver USBD1;

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/* QH list: 2 per endpoint (even = OUT, odd = IN), 2048-byte aligned. */
static imx23_usb_qh_t usb_qh[2U * IMX23_USB_NUM_EP] __attribute__((aligned(2048)));

/* EP0 state and configuration (callbacks are provided by hal_usb.c). */
static struct {
  USBInEndpointState  in;
  USBOutEndpointState out;
} ep0_state;

static uint8_t ep0setup_buffer[8];

static const USBEndpointConfig ep0config = {
  .ep_mode          = USB_EP_MODE_TYPE_CTRL,
  .setup_cb         = _usb_ep0setup,
  .in_cb            = _usb_ep0in,
  .out_cb           = _usb_ep0out,
  .in_maxsize       = EP0_MAX_PACKET_SIZE,
  .out_maxsize      = EP0_MAX_PACKET_SIZE,
  .in_state         = &ep0_state.in,
  .out_state        = &ep0_state.out,
  .ep_buffers       = 1U,
  .setup_buf        = ep0setup_buffer
};

/* Last transferred packet size per endpoint/direction (for
   usb_lld_get_transaction_size). */
static uint16_t usb_last_tx[IMX23_USB_NUM_EP];
static uint16_t usb_last_rx[IMX23_USB_NUM_EP];

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static imx23_usb_qh_t *usb_qh_get(unsigned ep, bool in) {

  return &usb_qh[2U * ep + (in ? 1U : 0U)];
}

/* PHY + clocks bring-up (phy-mxs-usb.c mxs_phy_hw_init for i.MX23). */
static void usb_phy_init(void) {

  /* usb_phy clock = PLL0 (480MHz) via PLL0CTRL0 bit 18; "usb" clock via
     DIGCTRL bit 2. */
  IMX23_REG_SET(IMX23_CLKCTRL_PLL0CTRL0, IMX23_CLKCTRL_PLL0CTRL0_EN_USB_CLKS);
  IMX23_REG_CLR(IMX23_DIGCTRL_CTRL, IMX23_DIGCTRL_CTRL_USB_CLKGATE);

  /* Reset the PHY block. */
  IMX23_REG_SET(IMX23_USBPHY_CTRL, IMX23_USBPHY_CTRL_SFTRST |
                                     IMX23_USBPHY_CTRL_CLKGATE);
  IMX23_REG_CLR(IMX23_USBPHY_CTRL, IMX23_USBPHY_CTRL_SFTRST);
  while ((IMX23_REG(IMX23_USBPHY_CTRL) & IMX23_USBPHY_CTRL_SFTRST) != 0U) {
  }
  IMX23_REG_CLR(IMX23_USBPHY_CTRL, IMX23_USBPHY_CTRL_CLKGATE);

  /* Power up the PHY. */
  IMX23_REG(IMX23_USBPHY_PWD) = 0U;

  /* Auto clock/power configuration + full/low speed UTMI levels. */
  IMX23_REG_SET(IMX23_USBPHY_CTRL,
                IMX23_USBPHY_CTRL_ENAUTOSET_USBCLKS |
                IMX23_USBPHY_CTRL_ENAUTOCLR_USBCLKGATE |
                IMX23_USBPHY_CTRL_ENAUTOCLR_PHY_PWD |
                IMX23_USBPHY_CTRL_ENAUTOCLR_CLKGATE |
                IMX23_USBPHY_CTRL_ENAUTO_PWRON_PLL |
                IMX23_USBPHY_CTRL_ENUTMILEVEL2 |
                IMX23_USBPHY_CTRL_ENUTMILEVEL3);
}

/* Initialize all QHs (called at start and on bus reset). */
static void usb_qh_init_all(USBDriver *usbp) {
  unsigned i;

  (void)usbp;
  for (i = 0U; i < 2U * IMX23_USB_NUM_EP; i++) {
    imx23_usb_qh_t *qh = &usb_qh[i];

    qh->config = (i < 2U) ? (IMX23_USB_QH_MAX_PKT(EP0_MAX_PACKET_SIZE) |
                             IMX23_USB_QH_ZLT | IMX23_USB_QH_IOS)
                          : (IMX23_USB_QH_MAX_PKT(EP_MAX_PACKET_SIZE) |
                             IMX23_USB_QH_ZLT);
    qh->next   = IMX23_USB_TD_TERMINATE;
    qh->info   = 0U;
    qh->current = 0U;
  }
}

/* Arm a transfer: one TD covering up to EP_MAX_LENGTH_TRANSFER bytes. */
static void usb_ep_arm(USBDriver *usbp, usbep_t ep, bool in,
                       uint8_t *buf, size_t n) {
  imx23_usb_qh_t *qh = usb_qh_get(ep, in);
  imx23_usb_td_t *td = &usbp->td[2U * ep + (in ? 1U : 0U)];
  uint32_t addr = (uint32_t)(uintptr_t)buf;

  if (n > EP_MAX_LENGTH_TRANSFER) {
    n = EP_MAX_LENGTH_TRANSFER;
  }

  td->next  = IMX23_USB_TD_TERMINATE;
  td->info  = IMX23_USB_TD_BYTES(n) | IMX23_USB_TD_IOC | IMX23_USB_TD_ACTIVE;
  td->page0 = addr;
  td->page1 = (addr & 0xFFFFF000U) + 0x1000U;
  td->page2 = (addr & 0xFFFFF000U) + 0x2000U;
  td->page3 = (addr & 0xFFFFF000U) + 0x3000U;
  td->page4 = (addr & 0xFFFFF000U) + 0x4000U;
  td->reserved = 0U;

  if (in) {
    usbp->td_in[ep] = td;
  }
  else {
    usbp->td_out[ep] = td;
  }

  qh->next  = (uint32_t)(uintptr_t)td;
  qh->info  = 0U;

  /* Prime the endpoint. */
  IMX23_REG(IMX23_USBC_ENDPTPRIME) = in ? IMX23_EPT_TX(ep) : IMX23_EPT_RX(ep);
}

/* USB0 interrupt handler (icoll). */
static bool usb_isr(void) {
  USBDriver *usbp = &USBD1;
  uint32_t sts = IMX23_REG(IMX23_USBC_USBSTS) & IMX23_REG(IMX23_USBC_USBINTR);

  if (sts == 0U) {
    return false;
  }

  /* Bus reset. */
  if ((sts & IMX23_USBSTS_URI) != 0U) {
    IMX23_REG(IMX23_USBC_USBSTS) = IMX23_USBSTS_URI;
    usb_lld_reset(usbp);
    _usb_isr_invoke_event_cb(usbp, USB_EVENT_RESET);
  }

  /* Port change (connect/resume): just acknowledge. */
  if ((sts & IMX23_USBSTS_PCI) != 0U) {
    IMX23_REG(IMX23_USBC_USBSTS) = IMX23_USBSTS_PCI;
  }

  /* Suspend / resume. */
  if ((sts & IMX23_USBSTS_SLI) != 0U) {
    IMX23_REG(IMX23_USBC_USBSTS) = IMX23_USBSTS_SLI;
    _usb_isr_invoke_event_cb(usbp, USB_EVENT_SUSPEND);
  }

  /* Setup packets. */
  {
    uint32_t ss = IMX23_REG(IMX23_USBC_ENDPTSETUPSTAT);
    if (ss != 0U) {
      unsigned ep;
      for (ep = 0U; ep < IMX23_USB_NUM_EP; ep++) {
        if ((ss & (1U << ep)) != 0U) {
          usb_lld_read_setup(usbp, (usbep_t)ep, usbp->setup);
          IMX23_REG(IMX23_USBC_ENDPTSETUPSTAT) = 1U << ep;
          _usb_isr_invoke_setup_cb(usbp, (usbep_t)ep);
        }
      }
    }
  }

  /* Endpoint transfers complete. */
  if ((sts & IMX23_USBSTS_UI) != 0U) {
    uint32_t comp = IMX23_REG(IMX23_USBC_ENDPTCOMPLETE);
    unsigned ep;

    for (ep = 0U; ep < IMX23_USB_NUM_EP; ep++) {
      const USBEndpointConfig *epcp = usbp->epc[ep];

      /* IN (TX) complete. */
      if ((comp & IMX23_EPT_TX(ep)) != 0U) {
        imx23_usb_td_t *td = usbp->td_in[ep];

        if (td != NULL && epcp != NULL && epcp->in_state != NULL) {
          USBInEndpointState *isp = epcp->in_state;
          uint32_t armed = isp->txsize - isp->txcnt;
          uint32_t remaining = (td->info >> 16) & 0x7FFFU;
          uint32_t xfer = (remaining < armed) ? (armed - remaining) : 0U;

          usb_last_tx[ep] = (uint16_t)xfer;
          isp->txcnt += xfer;
          if (isp->txcnt < isp->txsize) {
            /* More to send: re-arm the next chunk. */
            usb_ep_arm(usbp, (usbep_t)ep, true,
                       (uint8_t *)isp->txbuf + isp->txcnt,
                       isp->txsize - isp->txcnt);
          }
          else {
            usbp->transmitting &= (uint16_t)~(1U << ep);
            _usb_isr_invoke_in_cb(usbp, (usbep_t)ep);
          }
        }
      }

      /* OUT (RX) complete. */
      if ((comp & IMX23_EPT_RX(ep)) != 0U) {
        imx23_usb_td_t *td = usbp->td_out[ep];

        if (td != NULL && epcp != NULL && epcp->out_state != NULL) {
          USBOutEndpointState *osp = epcp->out_state;
          uint32_t remaining, n;

          remaining = (td->info >> 16) & 0x7FFFU;
          n = (osp->rxsize > remaining) ? (osp->rxsize - remaining) : 0U;
          usb_last_rx[ep] = (uint16_t)n;
          osp->rxbuf  += n;
          osp->rxcnt  += n;
          osp->rxsize -= n;
          usbp->receiving &= (uint16_t)~(1U << ep);
          /* Short packet or buffer full: transfer done. */
          if ((n < epcp->out_maxsize) || (osp->rxsize == 0U)) {
            _usb_isr_invoke_out_cb(usbp, (usbep_t)ep);
          }
          else {
            /* More to receive: re-arm. */
            usbp->receiving |= (uint16_t)(1U << ep);
            usb_ep_arm(usbp, (usbep_t)ep, false, osp->rxbuf, osp->rxsize);
          }
        }
      }
    }
    IMX23_REG(IMX23_USBC_ENDPTCOMPLETE) = comp;
  }

  return true;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

void usb_lld_init(void) {

}

void usb_lld_start(USBDriver *usbp) {
  unsigned i;

  if (usbp->state != USB_STOP) {
    return;
  }

  /* Clocks + PHY. */
  usb_phy_init();

  /* Controller soft reset. */
  IMX23_REG(IMX23_USBC_USBCMD) = IMX23_USBCMD_RST;
  while ((IMX23_REG(IMX23_USBC_USBCMD) & IMX23_USBCMD_RST) != 0U) {
  }

  /* Device mode. */
  IMX23_REG(IMX23_USBC_USBMODE) = IMX23_USBMODE_DEVICE;

  /* QH list. */
  usb_qh_init_all(usbp);
  IMX23_REG(IMX23_USBC_ENDPTLISTADDR) = (uint32_t)(uintptr_t)usb_qh;

  /* All endpoints disabled. */
  for (i = 0U; i < IMX23_USB_NUM_EP; i++) {
    IMX23_REG(IMX23_USBC_ENDPTCTRL(i)) = 0U;
  }

  /* Enable the PHY clock in the port. */
  IMX23_REG_CLR(IMX23_USBC_PORTSC1, (1U << 23));   /* PHCD */

  /* Interrupts: endpoint, error, port change, reset, suspend. */
  IMX23_REG(IMX23_USBC_USBSTS) = 0xFFU;             /* ack all */
  IMX23_REG(IMX23_USBC_ENDPTCOMPLETE) = 0xFFFFFFFFU;
  IMX23_REG(IMX23_USBC_ENDPTSETUPSTAT) = 0xFFU;
  IMX23_REG(IMX23_USBC_USBINTR) = IMX23_USBSTS_UI | IMX23_USBSTS_UEI |
                                  IMX23_USBSTS_PCI | IMX23_USBSTS_URI |
                                  IMX23_USBSTS_SLI;

  /* Run. */
  IMX23_REG(IMX23_USBC_USBCMD) = IMX23_USBCMD_ITC(8U) | IMX23_USBCMD_RUN;

  /* IRQ. */
  icoll_register_handler(IMX23_IRQ_USB0, usb_isr);
  icoll_irq_set_priority(IMX23_IRQ_USB0, IMX23_USB1_IRQ_PRIORITY);
  icoll_irq_enable(IMX23_IRQ_USB0);

  /* Post-reset style init (EP0). */
  usb_lld_reset(usbp);
}

void usb_lld_stop(USBDriver *usbp) {

  if (usbp->state == USB_STOP) {
    return;
  }
  icoll_irq_disable(IMX23_IRQ_USB0);
  IMX23_REG(IMX23_USBC_USBINTR) = 0U;
  IMX23_REG(IMX23_USBC_USBCMD) = 0U;    /* stop */
  IMX23_REG(IMX23_USBC_USBCMD) = IMX23_USBCMD_RST;
  while ((IMX23_REG(IMX23_USBC_USBCMD) & IMX23_USBCMD_RST) != 0U) {
  }
}

void usb_lld_reset(USBDriver *usbp) {
  unsigned i;

  /* Flush all endpoints, clear address and status. */
  IMX23_REG(IMX23_USBC_ENDPTFLUSH) = 0xFFFFFFFFU;
  while ((IMX23_REG(IMX23_USBC_ENDPTPRIME) & 0xFFFFFFFFU) != 0U) {
  }
  IMX23_REG(IMX23_USBC_ENDPTCOMPLETE) = 0xFFFFFFFFU;
  IMX23_REG(IMX23_USBC_ENDPTSETUPSTAT) = 0xFFU;
  IMX23_REG(IMX23_USBC_DEVICEADDR) = 0U;

  for (i = 0U; i < IMX23_USB_NUM_EP; i++) {
    IMX23_REG(IMX23_USBC_ENDPTCTRL(i)) = 0U;
    usbp->td_in[i] = NULL;
    usbp->td_out[i] = NULL;
  }

  usb_qh_init_all(usbp);

  /* EP0 as control, both directions. */
  usbp->epc[0] = &ep0config;
  usb_lld_init_endpoint(usbp, 0U);
}

void usb_lld_set_address(USBDriver *usbp) {

  IMX23_REG(IMX23_USBC_DEVICEADDR) = (uint32_t)usbp->address << 25;
}

void usb_lld_init_endpoint(USBDriver *usbp, usbep_t ep) {
  const USBEndpointConfig *epcp = usbp->epc[ep];
  uint32_t type;
  imx23_usb_qh_t *qh_out, *qh_in;

  switch (epcp->ep_mode & USB_EP_MODE_TYPE) {
  case USB_EP_MODE_TYPE_ISOC: type = 1U; break;
  case USB_EP_MODE_TYPE_BULK: type = 2U; break;
  case USB_EP_MODE_TYPE_INTR: type = 3U; break;
  default:                    type = 0U; break;   /* control */
  }

  qh_out = usb_qh_get(ep, false);
  qh_in  = usb_qh_get(ep, true);
  qh_out->config = IMX23_USB_QH_MAX_PKT(epcp->out_maxsize) | IMX23_USB_QH_ZLT;
  qh_in->config  = IMX23_USB_QH_MAX_PKT(epcp->in_maxsize)  | IMX23_USB_QH_ZLT;
  if (ep == 0U) {
    qh_out->config |= IMX23_USB_QH_IOS;
    qh_in->config  |= IMX23_USB_QH_IOS;
  }

  IMX23_REG(IMX23_USBC_ENDPTCTRL(ep)) =
      IMX23_ENDPTCTRL_TXE | IMX23_ENDPTCTRL_TXT(type) |
      IMX23_ENDPTCTRL_RXE | IMX23_ENDPTCTRL_RXT(type);
}

void usb_lld_disable_endpoints(USBDriver *usbp) {
  unsigned ep;

  for (ep = 1U; ep < IMX23_USB_NUM_EP; ep++) {
    IMX23_REG_CLR(IMX23_USBC_ENDPTCTRL(ep),
                  IMX23_ENDPTCTRL_TXE | IMX23_ENDPTCTRL_RXE);
    (void)usbp;
  }
}

usbepstatus_t usb_lld_get_status_in(USBDriver *usbp, usbep_t ep) {

  if ((IMX23_REG(IMX23_USBC_ENDPTCTRL(ep)) & IMX23_ENDPTCTRL_TXR) != 0U) {
    return EP_STATUS_STALLED;
  }
  return (usbp->transmitting & (1U << ep)) ? EP_STATUS_ACTIVE
                                            : EP_STATUS_DISABLED;
}

usbepstatus_t usb_lld_get_status_out(USBDriver *usbp, usbep_t ep) {

  if ((IMX23_REG(IMX23_USBC_ENDPTCTRL(ep)) & IMX23_ENDPTCTRL_RXR) != 0U) {
    return EP_STATUS_STALLED;
  }
  return (usbp->receiving & (1U << ep)) ? EP_STATUS_ACTIVE
                                         : EP_STATUS_DISABLED;
}

void usb_lld_read_setup(USBDriver *usbp, usbep_t ep, uint8_t *buf) {
  imx23_usb_qh_t *qh = usb_qh_get(ep, false);
  unsigned i;

  (void)usbp;
  for (i = 0U; i < 8U; i++) {
    buf[i] = qh->setup_data[i];
  }
}

void usb_lld_start_out(USBDriver *usbp, usbep_t ep) {
  const USBEndpointConfig *epcp = usbp->epc[ep];
  USBOutEndpointState *osp = epcp->out_state;

  osp->rxpkts = (osp->rxsize + epcp->out_maxsize - 1U) / epcp->out_maxsize;
  usb_ep_arm(usbp, ep, false, osp->rxbuf, osp->rxsize);
}

void usb_lld_start_in(USBDriver *usbp, usbep_t ep) {
  const USBEndpointConfig *epcp = usbp->epc[ep];
  USBInEndpointState *isp = epcp->in_state;

  usb_ep_arm(usbp, ep, true, (uint8_t *)isp->txbuf, isp->txsize);
}

void usb_lld_stall_out(USBDriver *usbp, usbep_t ep) {

  (void)usbp;
  IMX23_REG_SET(IMX23_USBC_ENDPTCTRL(ep), IMX23_ENDPTCTRL_RXR);
}

void usb_lld_stall_in(USBDriver *usbp, usbep_t ep) {

  (void)usbp;
  IMX23_REG_SET(IMX23_USBC_ENDPTCTRL(ep), IMX23_ENDPTCTRL_TXR);
}

void usb_lld_clear_out(USBDriver *usbp, usbep_t ep) {

  (void)usbp;
  IMX23_REG_CLR(IMX23_USBC_ENDPTCTRL(ep), IMX23_ENDPTCTRL_RXR);
}

void usb_lld_clear_in(USBDriver *usbp, usbep_t ep) {

  (void)usbp;
  IMX23_REG_CLR(IMX23_USBC_ENDPTCTRL(ep), IMX23_ENDPTCTRL_TXR);
}

#endif /* HAL_USE_USB && IMX23_USB_USE_USB1 */

/** @} */
