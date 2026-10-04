/**
 * @file    stm32f411_dma.h
 * @brief   DMA controller driver interface for STM32F411xE.
 *
 * This header defines a controller/stream based DMA API. It does not choose
 * peripheral request mappings: USART, SPI, and I2C drivers must select a
 * valid channel and stream from RM0383 before calling this interface.
 *
 * Reference: RM0383 Rev 4, Chapter 9.
 */

#ifndef STM32F411_DMA_H
#define STM32F411_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f411_xe.h"

/* ── Error Codes ──────────────────────────────────────────────────── */
#define DMA_ERROR_INVALID_PARAM (-1)
#define DMA_ERROR_TIMEOUT       (-2)
#define DMA_ERROR_UNSUPPORTED   (-3)
#define DMA_ERROR_BUSY          (-4)
#define DMA_ERROR_TRANSFER      (-5)

/* ── Stream and Channel Selection ─────────────────────────────────── */

/** DMA stream index within a DMA controller. */
typedef enum {
  DMA_STREAM_0 = 0U,
  DMA_STREAM_1 = 1U,
  DMA_STREAM_2 = 2U,
  DMA_STREAM_3 = 3U,
  DMA_STREAM_4 = 4U,
  DMA_STREAM_5 = 5U,
  DMA_STREAM_6 = 6U,
  DMA_STREAM_7 = 7U
} DMA_StreamIndex_t;

/** Bit shift of a stream's flag group within LISR/HISR or LIFCR/HIFCR. */
typedef enum {
  DMA_STREAM_OFFSET_0 = 0U,
  DMA_STREAM_OFFSET_1 = 6U,
  DMA_STREAM_OFFSET_2 = 16U,
  DMA_STREAM_OFFSET_3 = 22U
} DMA_StreamOffset_t;

/** DMA channel selection value written to DMA_SxCR.CHSEL. */
typedef enum {
  DMA_CHANNEL_0 = 0U,
  DMA_CHANNEL_1 = 1U,
  DMA_CHANNEL_2 = 2U,
  DMA_CHANNEL_3 = 3U,
  DMA_CHANNEL_4 = 4U,
  DMA_CHANNEL_5 = 5U,
  DMA_CHANNEL_6 = 6U,
  DMA_CHANNEL_7 = 7U
} DMA_Channel_t;

/** DMA data movement direction. */
typedef enum {
  DMA_DIRECTION_PERIPH_TO_MEMORY = 0U,
  DMA_DIRECTION_MEMORY_TO_PERIPH = 1U,
  DMA_DIRECTION_MEMORY_TO_MEMORY = 2U
} DMA_Direction_t;

/** Transfer width configured independently for peripheral and memory ports. */
typedef enum {
  DMA_DATA_WIDTH_BYTE     = 0U,
  DMA_DATA_WIDTH_HALFWORD = 1U,
  DMA_DATA_WIDTH_WORD     = 2U
} DMA_DataWidth_t;

/** Automatic address increment setting for an address port. */
typedef enum {
  DMA_INCREMENT_DISABLED = 0U,
  DMA_INCREMENT_ENABLED  = 1U
} DMA_Increment_t;

/** Stream transfer mode. */
typedef enum {
  DMA_MODE_NORMAL   = 0U,
  DMA_MODE_CIRCULAR = 1U
} DMA_Mode_t;

/** Arbitration priority for a DMA stream. */
typedef enum {
  DMA_PRIORITY_LOW       = 0U,
  DMA_PRIORITY_MEDIUM    = 1U,
  DMA_PRIORITY_HIGH      = 2U,
  DMA_PRIORITY_VERY_HIGH = 3U
} DMA_Priority_t;

/** FIFO operation mode. Memory-to-memory transfers require FIFO mode. */
typedef enum {
  DMA_FIFO_DIRECT = 0U,
  DMA_FIFO_ENABLED = 1U
} DMA_FifoMode_t;

/** FIFO fill level that triggers a memory-port transfer. */
typedef enum {
  DMA_FIFO_THRESHOLD_1_4  = 0U,
  DMA_FIFO_THRESHOLD_1_2  = 1U,
  DMA_FIFO_THRESHOLD_3_4  = 2U,
  DMA_FIFO_THRESHOLD_FULL = 3U
} DMA_FifoThreshold_t;

/* ── Configuration and Transfer Descriptors ───────────────────────── */

/**
 * @brief Stream configuration which normally remains stable across transfers.
 *
 * Burst mode, double buffering, and peripheral flow control are intentionally
 * excluded from the first driver version. Their CR fields are reset to the
 * single-burst, single-buffer, DMA-flow-controller configuration.
 */
typedef struct {
  DMA_Channel_t       channel;
  DMA_Direction_t     direction;
  DMA_DataWidth_t     periph_width;
  DMA_DataWidth_t     memory_width;
  DMA_Increment_t     periph_increment;
  DMA_Increment_t     memory_increment;
  DMA_Mode_t          mode;
  DMA_Priority_t      priority;
  DMA_FifoMode_t      fifo_mode;
  DMA_FifoThreshold_t fifo_threshold;
} DMA_Config_t;

/**
 * @brief Addresses and item count for one DMA transfer.
 *
 * @p item_count is in units of the configured peripheral width, not bytes, and
 * must be in the range 1..65535. Both addresses must be aligned to their
 * corresponding width. In FIFO mode, when PSIZE is smaller than MSIZE, the
 * count must also be a multiple of MSIZE / PSIZE.
 * For memory-to-memory mode, @p periph_address is the source address written
 * to PAR and @p memory_address is the destination address written to M0AR.
 */
typedef struct {
  uintptr_t periph_address;
  uintptr_t memory_address;
  uint32_t  item_count;
} DMA_Transfer_t;

/** DMA stream event bits returned by dma_get_flags(). */

#define DMA_EVENT_FIFO_ERROR        DMA_FEIF
#define DMA_EVENT_DIRECT_ERROR      DMA_DMEIF
#define DMA_EVENT_TRANSFER_ERROR    DMA_TEIF
#define DMA_EVENT_HALF_TRANSFER     DMA_HTIF
#define DMA_EVENT_TRANSFER_COMPLETE DMA_TCIF
#define DMA_EVENT_ALL               (DMA_EVENT_FIFO_ERROR |       \
                                     DMA_EVENT_DIRECT_ERROR |     \
                                     DMA_EVENT_TRANSFER_ERROR |   \
                                     DMA_EVENT_HALF_TRANSFER |    \
                                     DMA_EVENT_TRANSFER_COMPLETE)

/* ── Public API ───────────────────────────────────────────────────── */

/**
 * @brief Configure one disabled DMA stream without starting a transfer.
 * @param controller DMA1 or DMA2.
 * @param stream Stream index within @p controller.
 * @param config Stream configuration.
 * @return 0 on success; DMA_ERROR_INVALID_PARAM, DMA_ERROR_BUSY, or
 *         DMA_ERROR_UNSUPPORTED otherwise.
 */
int dma_init(DMA_TypeDef *controller, DMA_StreamIndex_t stream,
             const DMA_Config_t *config);

/**
 * @brief Load transfer registers, clear stale flags, and enable a stream.
 * @param controller DMA1 or DMA2.
 * @param stream Stream index within @p controller.
 * @param transfer Addresses and number of data items.
 * @return 0 on success; DMA_ERROR_INVALID_PARAM or DMA_ERROR_BUSY otherwise.
 */
int dma_start(DMA_TypeDef *controller, DMA_StreamIndex_t stream,
              const DMA_Transfer_t *transfer);

/**
 * @brief Wait for normal-mode completion or a DMA error.
 *
 * This function does not clear event flags and does not abort the stream after
 * an error or timeout.
 * @param controller DMA1 or DMA2.
 * @param stream Stream index within @p controller.
 * @param timeout Maximum polling iterations; must be non-zero.
 * @return 0 on transfer complete; DMA_ERROR_INVALID_PARAM,
 *         DMA_ERROR_UNSUPPORTED, DMA_ERROR_TIMEOUT, or DMA_ERROR_TRANSFER
 *         otherwise.
 */
int dma_poll_complete(DMA_TypeDef *controller, DMA_StreamIndex_t stream,
                      uint32_t timeout);

/**
 * @brief Disable a running stream and wait until its EN bit is cleared.
 * @param controller DMA1 or DMA2.
 * @param stream Stream index within @p controller.
 * @param timeout Maximum polling iterations; must be non-zero.
 * @return 0 on success; DMA_ERROR_INVALID_PARAM or DMA_ERROR_TIMEOUT
 *         otherwise.
 */
int dma_abort(DMA_TypeDef *controller, DMA_StreamIndex_t stream,
              uint32_t timeout);

/**
 * @brief Read raw event flags for one stream without clearing them.
 * @param controller DMA1 or DMA2.
 * @param stream Stream index within @p controller.
 * @param flags Output event mask composed of DMA_EVENT_* values.
 * @return 0 on success or DMA_ERROR_INVALID_PARAM.
 */
int dma_get_flags(DMA_TypeDef *controller, DMA_StreamIndex_t stream,
                  uint32_t *flags);

/**
 * @brief Clear selected event flags for one stream.
 * @param controller DMA1 or DMA2.
 * @param stream Stream index within @p controller.
 * @param flags DMA_EVENT_* mask to clear.
 * @return 0 on success or DMA_ERROR_INVALID_PARAM.
 */
int dma_clear_flags(DMA_TypeDef *controller, DMA_StreamIndex_t stream,
                    uint32_t flags);

/**
 * @brief Read the number of data items not yet transferred.
 * @param controller DMA1 or DMA2.
 * @param stream Stream index within @p controller.
 * @param remaining Output number of remaining items.
 * @return 0 on success or DMA_ERROR_INVALID_PARAM.
 */
int dma_get_remaining(DMA_TypeDef *controller, DMA_StreamIndex_t stream,
                      uint16_t *remaining);

#ifdef __cplusplus
}
#endif

#endif /* STM32F411_DMA_H */
