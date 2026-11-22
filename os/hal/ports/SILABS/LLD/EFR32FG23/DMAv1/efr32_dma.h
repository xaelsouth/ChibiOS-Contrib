/*
    ChibiOS - Copyright (C) 2024 Xael South

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
 * @file    DMAv1/efr32_dma.h
 * @brief   Enhanced-DMA helper driver header.
 *
 * @addtogroup EFR32_DMA
 * @{
 */

#ifndef EFR32_DMA_H
#define EFR32_DMA_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Total number of DMA channels.
 */
#define EFR32_DMA_STREAMS           8U

/**
 * @brief   Error mask of the ISR bits.
 */
#define EFR32_DMA_ISR_ERROR_MASK    0x80000000U

/**
 * @brief   Done mask of the ISR bits.
 */
#define EFR32_DMA_ISR_DONE_MASK(id) (1U << EFR32_DMA_STREAM(id)->selfindex)

/**
 * @brief   Mask of the ISR bits passed to the DMA callback functions.
 */
#define EFR32_DMA_ISR_MASK(id)      (EFR32_DMA_ISR_ERROR_MASK | EFR32_DMA_ISR_DONE_MASK(id))

/**
 * @brief   Checks if a DMA stream id is within the valid range.
 */
#define EFR32_DMA_IS_VALID_STREAM(id) (((id) >= 0U) &&                      \
                                       ((id) < EFR32_DMA_STREAMS))

/**
 * @name    Special stream identifiers
 * @{
 */
#define EFR32_DMA_STREAM_ID_ANY     EFR32_DMA_STREAMS
/** @} */

/**
 * @name    DMA streams identifiers
 * @{
 */
#define EFR32_DMA_STREAM(id)        (&_efr32_dma_streams[id])

#define EFR32_DMA1_STREAM0          EFR32_DMA_STREAM(0)
#define EFR32_DMA1_STREAM1          EFR32_DMA_STREAM(1)
#define EFR32_DMA1_STREAM2          EFR32_DMA_STREAM(2)
#define EFR32_DMA1_STREAM3          EFR32_DMA_STREAM(3)
#define EFR32_DMA1_STREAM4          EFR32_DMA_STREAM(4)
#define EFR32_DMA1_STREAM5          EFR32_DMA_STREAM(5)
#define EFR32_DMA1_STREAM6          EFR32_DMA_STREAM(6)
#define EFR32_DMA1_STREAM7          EFR32_DMA_STREAM(7)
/** @} */

/*===========================================================================*/
/* CTRL register constants.                                                  */
/*===========================================================================*/

#define EFR32_DMA_CH_CTRL_DSTMODE_ABSOLUTE  (0U << 31)
#define EFR32_DMA_CH_CTRL_DSTMODE_RELATIVE  (1U << 31)

#define EFR32_DMA_CH_CTRL_SRCMODE_ABSOLUTE  (0U << 30)
#define EFR32_DMA_CH_CTRL_SRCMODE_RELATIVE  (1U << 30)

#define EFR32_DMA_CH_CTRL_DSTINC_ONE        (0U << 28)
#define EFR32_DMA_CH_CTRL_DSTINC_TWO        (1U << 28)
#define EFR32_DMA_CH_CTRL_DSTINC_FOUR       (2U << 28)
#define EFR32_DMA_CH_CTRL_DSTINC_NONE       (3U << 28)

#define EFR32_DMA_CH_CTRL_SIZE_BYTE         (0U << 26)
#define EFR32_DMA_CH_CTRL_SIZE_HALFWORD     (1U << 26)
#define EFR32_DMA_CH_CTRL_SIZE_WORD         (2U << 26)

#define EFR32_DMA_CH_CTRL_SRCINC_ONE        (0U << 24)
#define EFR32_DMA_CH_CTRL_SRCINC_TWO        (1U << 24)
#define EFR32_DMA_CH_CTRL_SRCINC_FOUR       (2U << 24)
#define EFR32_DMA_CH_CTRL_SRCINC_NONE       (3U << 24)

#define EFR32_DMA_CH_CTRL_IGNORESREQ        (1U << 23)

#define EFR32_DMA_CH_CTRL_DECLOOPCNT        (1U << 22)

#define EFR32_DMA_CH_CTRL_REQMODE_BLOCK     (0U << 21)
#define EFR32_DMA_CH_CTRL_REQMODE_ALL       (1U << 21)

#define EFR32_DMA_CH_CTRL_DONEIEN           (1U << 20)

/* Block size: number of transfers per DMA request (1-15, encoded as n-1). */
#define EFR32_DMA_CH_CTRL_BLOCKSIZE(n)      (((n) & 0xFU) << 16)
#define EFR32_DMA_CH_CTRL_BLOCKSIZE_ALL     EFR32_DMA_CH_CTRL_BLOCKSIZE(15U)

#define EFR32_DMA_CH_CTRL_BYTESWAP          (1U << 15)

#define EFR32_DMA_CH_CTRL_XFERCNT(size)     ((((size) - 1) & 0x7FFU) << 4)

#define EFR32_DMA_CH_CTRL_STRUCTREQ         (1U << 3)

#define EFR32_DMA_CH_CTRL_STRUCTTYPE_TRANSFER    (0U << 0)
#define EFR32_DMA_CH_CTRL_STRUCTTYPE_SYNCHRONIZE (1U << 0)
#define EFR32_DMA_CH_CTRL_STRUCTTYPE_WRITE       (2U << 0)

/*===========================================================================*/
/* LDMAXBAR request routing constants.                                       */
/*===========================================================================*/
/* On FG23 the peripheral request routing lives in the LDMAXBAR peripheral:
   each channel's CHx_REQSEL = SOURCESEL (bits 16-21, which peripheral) |
   SIGSEL (bits 0-3, which signal of that peripheral).                        */

/** @name Request sources (SOURCESEL field of CHx_REQSEL) @{ */
#define EFR32_DMA_REQ_NONE        _LDMAXBAR_CH_REQSEL_SOURCESEL_NONE
#define EFR32_DMA_REQ_SW          _LDMAXBAR_CH_REQSEL_SOURCESEL_LDMAXBAR /**< software/PRS (mem-to-mem) */
#define EFR32_DMA_REQ_TIMER0      _LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER0
#define EFR32_DMA_REQ_TIMER1      _LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER1
#define EFR32_DMA_REQ_TIMER2      _LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER2
#define EFR32_DMA_REQ_TIMER3      _LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER3
#define EFR32_DMA_REQ_TIMER4      _LDMAXBAR_CH_REQSEL_SOURCESEL_TIMER4
#define EFR32_DMA_REQ_USART0      _LDMAXBAR_CH_REQSEL_SOURCESEL_USART0
#define EFR32_DMA_REQ_I2C0        _LDMAXBAR_CH_REQSEL_SOURCESEL_I2C0
#define EFR32_DMA_REQ_I2C1        _LDMAXBAR_CH_REQSEL_SOURCESEL_I2C1
#define EFR32_DMA_REQ_IADC0       _LDMAXBAR_CH_REQSEL_SOURCESEL_IADC0
#define EFR32_DMA_REQ_MSC         _LDMAXBAR_CH_REQSEL_SOURCESEL_MSC
#define EFR32_DMA_REQ_VDAC0       _LDMAXBAR_CH_REQSEL_SOURCESEL_VDAC0
#define EFR32_DMA_REQ_EUSART0     _LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART0
#define EFR32_DMA_REQ_EUSART1     _LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART1
#define EFR32_DMA_REQ_EUSART2     _LDMAXBAR_CH_REQSEL_SOURCESEL_EUSART2
#define EFR32_DMA_REQ_LESENSE     _LDMAXBAR_CH_REQSEL_SOURCESEL_LESENSE
#define EFR32_DMA_REQ_LCD         _LDMAXBAR_CH_REQSEL_SOURCESEL_LCD
/** @} */

/** @name Request signals (SIGSEL field of CHx_REQSEL) @{ */
#define EFR32_DMA_SIG_TIMER_CC0(n)  _LDMAXBAR_CH_REQSEL_SIGSEL_TIMER##n##CC0
#define EFR32_DMA_SIG_TIMER_CC1(n)  _LDMAXBAR_CH_REQSEL_SIGSEL_TIMER##n##CC1
#define EFR32_DMA_SIG_TIMER_CC2(n)  _LDMAXBAR_CH_REQSEL_SIGSEL_TIMER##n##CC2
#define EFR32_DMA_SIG_TIMER_UFOF(n) _LDMAXBAR_CH_REQSEL_SIGSEL_TIMER##n##UFOF
#define EFR32_DMA_SIG_USART0_RX     _LDMAXBAR_CH_REQSEL_SIGSEL_USART0RXDATAV
#define EFR32_DMA_SIG_USART0_TX     _LDMAXBAR_CH_REQSEL_SIGSEL_USART0TXBL
#define EFR32_DMA_SIG_I2C_RX         _LDMAXBAR_CH_REQSEL_SIGSEL_I2C0RXDATAV
#define EFR32_DMA_SIG_I2C_TX         _LDMAXBAR_CH_REQSEL_SIGSEL_I2C0TXBL
#define EFR32_DMA_SIG_EUSART_RX      _LDMAXBAR_CH_REQSEL_SIGSEL_EUSART0RXFL
#define EFR32_DMA_SIG_EUSART_TX      _LDMAXBAR_CH_REQSEL_SIGSEL_EUSART0TXFL
#define EFR32_DMA_SIG_LESENSE_FIFO   _LDMAXBAR_CH_REQSEL_SIGSEL_LESENSEFIFO
#define EFR32_DMA_SIG_MSC_WDATA      _LDMAXBAR_CH_REQSEL_SIGSEL_MSCWDATA
/** @} */

/*===========================================================================*/
/* LOOP register constants.                                                  */
/*===========================================================================*/

#define EFR32_DMA_CH_LOOP_LOOPCNT(n)        ((n) & 0xFFU)

/*===========================================================================*/
/* LINK register constants.                                                  */
/*===========================================================================*/

#define EFR32_DMA_CH_LINK_LINKMODE_ABSOLUTE (0U << 0)
#define EFR32_DMA_CH_LINK_LINKMODE_RELATIVE (1U << 0)
#define EFR32_DMA_CH_LINK_LINKADDR(addr)    ((uint32_t)(addr) & ~0x3U)

/*===========================================================================*/
/* In-memory descriptor structure (for linked-list / ping-pong).             */
/*===========================================================================*/

/**
 * @brief   LDMA in-memory descriptor.
 * @details Layout matches the LDMA_CH_TypeDef hardware register block so
 *          that a descriptor can be linked via CHx_LINK.
 */
typedef struct {
  volatile uint32_t CTRL;   /**< Control word.                              */
  volatile uint32_t SRC;    /**< Source address.                            */
  volatile uint32_t DST;    /**< Destination address.                       */
  volatile uint32_t LINK;   /**< Link to next descriptor (word-aligned).    */
} efr32_dma_desc_t;

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   EFR32 DMA ISR function type.
 */
typedef void (*efr32_dmaisr_t)(void* p, uint32_t flags);

/**
 * @brief   EFR32 DMA stream descriptor structure.
 */
typedef struct {
  LDMA_CH_TypeDef       *stream;    /**< Associated DMA channel registers. */
  volatile uint32_t     *reqsel;    /**< Associated REQSEL register.        */
  uint8_t               selfindex;  /**< Index to self in array.            */
} efr32_dma_stream_t;

/*===========================================================================*/
/* Driver macros — low-level register access.                                */
/*===========================================================================*/

/**
 * @brief   Associates a peripheral data register to a DMA stream.
 */
#define dmaStreamSetPeripheral(dmastp, addr) {                              \
  (dmastp)->stream->SRC = (uint32_t)(addr);                                 \
}

/**
 * @brief   Associates a memory destination to a DMA stream.
 */
#define dmaStreamSetMemory0(dmastp, addr) {                                 \
  (dmastp)->stream->DST = (uint32_t)(addr);                                 \
}

/**
 * @brief   Programs the stream mode settings.
 */
#define dmaStreamSetMode(dmastp, mode) {                                    \
  (dmastp)->stream->CTRL = (mode);                                          \
}

/**
 * @brief   Sets the source address.
 */
#define dmaStreamSetSource(dmastp, addr) {                                  \
  (dmastp)->stream->SRC = (uint32_t)(addr);                                 \
}

/**
 * @brief   Sets the destination address.
 */
#define dmaStreamSetDestination(dmastp, addr) {                             \
  (dmastp)->stream->DST = (uint32_t)(addr);                                 \
}

/**
 * @brief   Selects the peripheral request source for the channel.
 */
#define dmaStreamSetRequestSource(dmastp, src, sig) {                       \
  *(dmastp)->reqsel = ((uint32_t)(src) << 16) | ((uint32_t)(sig) << 0);     \
}

/**
 * @brief   DMA stream enable (starts the transfer).
 */
#define dmaStreamEnable(dmastp) {                                           \
  dmaStreamClearInterrupt(dmastp);                                          \
  LDMA->IEN_SET = (1U << (dmastp)->selfindex);                              \
  LDMA->CHEN_SET = (1U << (dmastp)->selfindex);                             \
}

/**
 * @brief   DMA stream disable.
 */
#define dmaStreamDisable(dmastp) {                                          \
  LDMA->CHDIS_SET = (1U << (dmastp)->selfindex);                            \
  LDMA->IEN_CLR = (1U << (dmastp)->selfindex);                              \
  dmaStreamClearInterrupt(dmastp);                                          \
  (dmastp)->stream->CFG = _LDMA_CH_CFG_RESETVALUE;                          \
  (dmastp)->stream->LOOP = _LDMA_CH_LOOP_RESETVALUE;                        \
  (dmastp)->stream->LINK = _LDMA_CH_LINK_RESETVALUE;                        \
}

/**
 * @brief   DMA stream interrupt sources clear.
 */
#define dmaStreamClearInterrupt(dmastp) {                                   \
  LDMA->IF_CLR = (1U << (dmastp)->selfindex);                               \
}

/**
 * @brief   Polled wait for DMA transfer end.
 */
#define dmaWaitCompletion(dmastp) {                                         \
  LDMA->IEN_CLR = (1U << (dmastp)->selfindex);                              \
  /* Poll both IF.DONE and CHBUSY: the shared LDMA ISR clears IF, so IF    \
     alone may never be observed even though the transfer completed. */     \
  while (((LDMA->IF & (1U << (dmastp)->selfindex)) == 0U) &&                \
         ((LDMA->CHBUSY & (1U << (dmastp)->selfindex)) != 0U)) {}           \
  dmaStreamClearInterrupt(dmastp);                                          \
}

/*===========================================================================*/
/* High-level transfer macros.                                               */
/*===========================================================================*/

/**
 * @brief   Starts a memory-to-memory copy.
 *
 * @param[in] dmastp    pointer to a efr32_dma_stream_t structure
 * @param[in] mode      extra CTRL bits (size, increment, etc.)
 * @param[in] src       source address
 * @param[in] dst       destination address
 * @param[in] n         number of data units to copy
 */
#define dmaStartMemCopy(dmastp, mode, src, dst, n) {                        \
  dmaStreamSetSource(dmastp, src);                                          \
  dmaStreamSetDestination(dmastp, dst);                                     \
  dmaStreamSetMode(dmastp, (mode) |                                         \
                           EFR32_DMA_CH_CTRL_DSTMODE_ABSOLUTE |             \
                           EFR32_DMA_CH_CTRL_SRCMODE_ABSOLUTE |             \
                           EFR32_DMA_CH_CTRL_SIZE_BYTE        |             \
                           EFR32_DMA_CH_CTRL_DSTINC_ONE       |             \
                           EFR32_DMA_CH_CTRL_SRCINC_ONE       |             \
                           EFR32_DMA_CH_CTRL_REQMODE_ALL      |             \
                           EFR32_DMA_CH_CTRL_BLOCKSIZE_ALL    |             \
                           EFR32_DMA_CH_CTRL_XFERCNT(n)       |             \
                           EFR32_DMA_CH_CTRL_DONEIEN);                      \
  dmaStreamEnable(dmastp);                                                  \
  LDMA->SWREQ_SET = (1U << (dmastp)->selfindex);                           \
}

/**
 * @brief   Starts a memory-to-peripheral (device TX) transfer.
 * @details The source is memory (incremented), destination is a fixed
 *          peripheral register (no increment). A peripheral request signal
 *          must be selected via dmaStreamSetRequestSource() beforehand.
 *
 * @param[in] dmastp    pointer to a efr32_dma_stream_t structure
 * @param[in] src       memory source address
 * @param[in] dst       peripheral register address (e.g. &EUSART0->TXDATA)
 * @param[in] n         number of data units
 * @param[in] size      transfer unit size (BYTE/HALFWORD/WORD)
 */
#define dmaStartMemToPeriph(dmastp, src, dst, n, size) {                    \
  dmaStreamSetSource(dmastp, src);                                          \
  dmaStreamSetDestination(dmastp, dst);                                     \
  dmaStreamSetMode(dmastp,                                                  \
                   EFR32_DMA_CH_CTRL_DSTMODE_ABSOLUTE |                     \
                   EFR32_DMA_CH_CTRL_SRCMODE_ABSOLUTE |                     \
                   (size)                              |                     \
                   EFR32_DMA_CH_CTRL_DSTINC_NONE      |                     \
                   EFR32_DMA_CH_CTRL_SRCINC_ONE      |                     \
                   EFR32_DMA_CH_CTRL_REQMODE_BLOCK    |                     \
                   EFR32_DMA_CH_CTRL_BLOCKSIZE(1U)    |                     \
                   EFR32_DMA_CH_CTRL_XFERCNT(n)       |                     \
                   EFR32_DMA_CH_CTRL_DONEIEN);                              \
  dmaStreamEnable(dmastp);                                                  \
}

/**
 * @brief   Starts a peripheral-to-memory (device RX) transfer.
 * @details The source is a fixed peripheral register (no increment),
 *          destination is memory (incremented).
 *
 * @param[in] dmastp    pointer to a efr32_dma_stream_t structure
 * @param[in] src       peripheral register address (e.g. &EUSART0->RXDATA)
 * @param[in] dst       memory destination address
 * @param[in] n         number of data units
 * @param[in] size      transfer unit size (BYTE/HALFWORD/WORD)
 */
#define dmaStartPeriphToMem(dmastp, src, dst, n, size) {                    \
  dmaStreamSetSource(dmastp, src);                                          \
  dmaStreamSetDestination(dmastp, dst);                                     \
  dmaStreamSetMode(dmastp,                                                  \
                   EFR32_DMA_CH_CTRL_DSTMODE_ABSOLUTE |                     \
                   EFR32_DMA_CH_CTRL_SRCMODE_ABSOLUTE |                     \
                   (size)                              |                     \
                   EFR32_DMA_CH_CTRL_DSTINC_ONE      |                     \
                   EFR32_DMA_CH_CTRL_SRCINC_NONE     |                     \
                   EFR32_DMA_CH_CTRL_REQMODE_BLOCK    |                     \
                   EFR32_DMA_CH_CTRL_BLOCKSIZE(1U)    |                     \
                   EFR32_DMA_CH_CTRL_XFERCNT(n)       |                     \
                   EFR32_DMA_CH_CTRL_DONEIEN);                              \
  dmaStreamEnable(dmastp);                                                  \
}

/**
 * @brief   Starts a ping-pong transfer between two memory buffers.
 * @details Uses the LOOP register to alternate between two descriptor
 *          halves. Each half is `n` transfers; the channel loops `loops`
 *          times between the two halves. The DECLOOPCNT bit makes the
 *          LOOP counter decrement per iteration.
 *
 * @param[in] dmastp    pointer to a efr32_dma_stream_t structure
 * @param[in] desc      pointer to a 2-element efr32_dma_desc_t array
 * @param[in] n         transfers per ping-pong half
 * @param[in] loops     number of ping-pong iterations (0 = infinite)
 */
#define dmaStartPingPong(dmastp, desc, n, loops) {                         \
  (dmastp)->stream->LOOP = EFR32_DMA_CH_LOOP_LOOPCNT(loops);               \
  (dmastp)->stream->LINK = EFR32_DMA_CH_LINK_LINKADDR(&(desc)[1]) |        \
                           EFR32_DMA_CH_LINK_LINKMODE_RELATIVE;            \
  dmaStreamSetMode(dmastp,                                                  \
                   (desc)[0].CTRL |                                         \
                   EFR32_DMA_CH_CTRL_DECLOOPCNT |                           \
                   EFR32_DMA_CH_CTRL_XFERCNT(n) |                           \
                   EFR32_DMA_CH_CTRL_DONEIEN);                              \
  (dmastp)->stream->SRC = (desc)[0].SRC;                                   \
  (dmastp)->stream->DST = (desc)[0].DST;                                   \
  dmaStreamEnable(dmastp);                                                  \
}

/**
 * @brief   Starts a linked-list transfer.
 * @details Each descriptor in the chain is executed in order. The LINK
 *          field of each descriptor points to the next. The last descriptor
 *          should have LINK = 0 (or link to itself for circular).
 *
 * @param[in] dmastp    pointer to a efr32_dma_stream_t structure
 * @param[in] desc      pointer to the first efr32_dma_desc_t in the chain
 */
#define dmaStartLinkedList(dmastp, desc) {                                 \
  LDMA->LINKLOAD = (uint32_t)(desc);                                        \
  dmaStreamEnable(dmastp);                                                  \
}

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(__DOXYGEN__)
extern const efr32_dma_stream_t _efr32_dma_streams[EFR32_DMA_STREAMS];
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void dmaInit(void);
  const efr32_dma_stream_t *dmaStreamAllocI(uint32_t id,
                                            uint32_t priority,
                                            efr32_dmaisr_t func,
                                            void *param);
  const efr32_dma_stream_t *dmaStreamAlloc(uint32_t id,
                                           uint32_t priority,
                                           efr32_dmaisr_t func,
                                           void *param);
  void dmaStreamFreeI(const efr32_dma_stream_t *dmastp);
  void dmaStreamFree(const efr32_dma_stream_t *dmastp);
#ifdef __cplusplus
}
#endif

#endif /* EFR32_DMA_H */

/** @} */
