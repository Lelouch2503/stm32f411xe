#include "stm32f411_dma.h"
#include "stm32f411_rcc.h"

#include <stddef.h>

static int dma_is_valid_controller(const DMA_TypeDef *controller)
{
    return (controller == DMA1) || (controller == DMA2);
}

static int dma_is_valid_stream(DMA_StreamIndex_t stream)
{
    return (uint32_t)stream <= (uint32_t)DMA_STREAM_7;
}

static DMA_Stream_TypeDef *dma_get_stream(DMA_TypeDef *controller,
                                          DMA_StreamIndex_t stream)
{
    return &controller->Stream[(uint32_t)stream];
}

static void dma_enable_clock(const DMA_TypeDef *controller)
{
    if (controller == DMA1)
    {
        rcc_ahb1_clk_enable(RCC_AHB1ENR_DMA1EN);
    }
    else
    {
        rcc_ahb1_clk_enable(RCC_AHB1ENR_DMA2EN);
    }
}

static uint32_t dma_get_stream_offset(DMA_StreamIndex_t stream)
{
    switch ((uint32_t)stream & 0x3U)
    {
    case 0U:
        return DMA_STREAM_OFFSET_0;
    case 1U:
        return DMA_STREAM_OFFSET_1;
    case 2U:
        return DMA_STREAM_OFFSET_2;
    default:
        return DMA_STREAM_OFFSET_3;
    }
}

static int dma_validate_config(const DMA_TypeDef *controller,
                               const DMA_Config_t *config)
{
    if ((!dma_is_valid_controller(controller)) || (config == NULL))
    {
        return DMA_ERROR_INVALID_PARAM;
    }

    if (((uint32_t)config->channel > (uint32_t)DMA_CHANNEL_7) ||
        ((uint32_t)config->direction >
         (uint32_t)DMA_DIRECTION_MEMORY_TO_MEMORY) ||
        ((uint32_t)config->periph_width > (uint32_t)DMA_DATA_WIDTH_WORD) ||
        ((uint32_t)config->memory_width > (uint32_t)DMA_DATA_WIDTH_WORD) ||
        ((uint32_t)config->periph_increment >
         (uint32_t)DMA_INCREMENT_ENABLED) ||
        ((uint32_t)config->memory_increment >
         (uint32_t)DMA_INCREMENT_ENABLED) ||
        ((uint32_t)config->mode > (uint32_t)DMA_MODE_CIRCULAR) ||
        ((uint32_t)config->priority > (uint32_t)DMA_PRIORITY_VERY_HIGH) ||
        ((uint32_t)config->fifo_mode > (uint32_t)DMA_FIFO_ENABLED) ||
        ((uint32_t)config->fifo_threshold >
         (uint32_t)DMA_FIFO_THRESHOLD_FULL))
    {
        return DMA_ERROR_INVALID_PARAM;
    }

    if (config->direction == DMA_DIRECTION_MEMORY_TO_MEMORY)
    {
        if (controller != DMA2)
        {
            return DMA_ERROR_UNSUPPORTED;
        }

        if ((config->mode == DMA_MODE_CIRCULAR) ||
            (config->fifo_mode != DMA_FIFO_ENABLED))
        {
            return DMA_ERROR_UNSUPPORTED;
        }
    }

    if ((config->fifo_mode == DMA_FIFO_DIRECT) &&
        (config->periph_width != config->memory_width))
    {
        return DMA_ERROR_UNSUPPORTED;
    }

    return 0;
}

int dma_init(DMA_TypeDef *controller, DMA_StreamIndex_t stream,
             const DMA_Config_t *config)
{
    if (!dma_is_valid_stream(stream))
    {
        return DMA_ERROR_INVALID_PARAM;
    }

    int status = dma_validate_config(controller, config);
    if (status != 0)
    {
        return status;
    }

    dma_enable_clock(controller);

    DMA_Stream_TypeDef *dma_stream = dma_get_stream(controller, stream);
    if ((dma_stream->CR.reg & DMA_SxCR_EN) != 0U)
    {
        return DMA_ERROR_BUSY;
    }

    uint32_t cr = 0U;
    cr |= (((uint32_t)config->channel      << DMA_SxCR_CHSEL_Pos)    & DMA_SxCR_CHSEL_Msk);
    cr |= (((uint32_t)config->direction    << DMA_SxCR_DIR_Pos)      & DMA_SxCR_DIR_Msk);
    cr |= (((uint32_t)config->memory_width << DMA_SxCR_MSIZE_Pos)    & DMA_SxCR_MSIZE_Msk);
    cr |= (((uint32_t)config->periph_width << DMA_SxCR_PSIZE_Pos)    & DMA_SxCR_PSIZE_Msk);
    cr |= (((uint32_t)config->priority     << DMA_SxCR_PL_Pos)       &  DMA_SxCR_PL_Msk);

    if (config->memory_increment == DMA_INCREMENT_ENABLED)
    {
        cr |= DMA_SxCR_MINC;
    }
    if (config->periph_increment == DMA_INCREMENT_ENABLED)
    {
        cr |= DMA_SxCR_PINC;
    }
    if (config->mode == DMA_MODE_CIRCULAR)
    {
        cr |= DMA_SxCR_CIRC;
    }

    uint32_t fcr =
        ((uint32_t)config->fifo_threshold << DMA_SxFCR_FTH_Pos) &
        DMA_SxFCR_FTH_Msk;
    if (config->fifo_mode == DMA_FIFO_ENABLED)
    {
        fcr |= DMA_SxFCR_DMDIS;
    }

    dma_stream->CR.reg = cr;
    dma_stream->FCR.reg = fcr;

    return 0;
}

int dma_get_flags(DMA_TypeDef *controller, DMA_StreamIndex_t stream,
                  uint32_t *flags)
{
    if ((!dma_is_valid_controller(controller)) ||
        (!dma_is_valid_stream(stream)) || (flags == NULL))
    {
        return DMA_ERROR_INVALID_PARAM;
    }

    uint32_t raw_flags;
    if ((uint32_t)stream < (uint32_t)DMA_STREAM_4)
    {
        raw_flags = controller->LISR.reg;
    }
    else
    {
        raw_flags = controller->HISR.reg;
    }

    *flags = (raw_flags >> dma_get_stream_offset(stream)) & DMA_EVENT_ALL;
    return 0;
}

int dma_clear_flags(DMA_TypeDef *controller, DMA_StreamIndex_t stream,
                    uint32_t flags)
{
    if ((!dma_is_valid_controller(controller)) ||
        (!dma_is_valid_stream(stream)) ||
        ((flags & ~((uint32_t)DMA_EVENT_ALL)) != 0U))
    {
        return DMA_ERROR_INVALID_PARAM;
    }

    uint32_t clear_mask = flags << dma_get_stream_offset(stream);

    /* LIFCR/HIFCR are write-one-to-clear registers: never use read-modify-write. */
    if ((uint32_t)stream < (uint32_t)DMA_STREAM_4)
    {
        controller->LIFCR.reg = clear_mask;
    }
    else
    {
        controller->HIFCR.reg = clear_mask;
    }

    return 0;
}

int dma_get_remaining(DMA_TypeDef *controller, DMA_StreamIndex_t stream,
                      uint16_t *remaining)
{
    if ((!dma_is_valid_controller(controller)) ||
        (!dma_is_valid_stream(stream)) || (remaining == NULL))
    {
        return DMA_ERROR_INVALID_PARAM;
    }

    DMA_Stream_TypeDef *dma_stream = dma_get_stream(controller, stream);
    *remaining = (uint16_t)(dma_stream->NDTR.reg & DMA_SxNDTR_NDT_Msk);

    return 0;
}

int dma_abort(DMA_TypeDef *controller, DMA_StreamIndex_t stream,
              uint32_t timeout)
{
    if ((!dma_is_valid_controller(controller)) ||
        (!dma_is_valid_stream(stream)) || (timeout == 0U))
    {
        return DMA_ERROR_INVALID_PARAM;
    }

    DMA_Stream_TypeDef *dma_stream = dma_get_stream(controller, stream);
    dma_stream->CR.reg &= ~DMA_SxCR_EN;

    while ((dma_stream->CR.reg & DMA_SxCR_EN) != 0U)
    {
        --timeout;
        if (timeout == 0U)
        {
            return DMA_ERROR_TIMEOUT;
        }
    }

    return 0;
}

int dma_start(DMA_TypeDef *controller, DMA_StreamIndex_t stream,
              const DMA_Transfer_t *transfer)
{
    if ((!dma_is_valid_controller(controller)) ||
        (!dma_is_valid_stream(stream)) || (transfer == NULL))
    {
        return DMA_ERROR_INVALID_PARAM;
    }

    if ((transfer->item_count == 0U) ||
        (transfer->item_count > DMA_SxNDTR_NDT_Msk))
    {
        return DMA_ERROR_INVALID_PARAM;
    }

    DMA_Stream_TypeDef *dma_stream = dma_get_stream(controller, stream);
    if ((dma_stream->CR.reg & DMA_SxCR_EN) != 0U)
    {
        return DMA_ERROR_BUSY;
    }

    uint32_t psize =
        (dma_stream->CR.reg & DMA_SxCR_PSIZE_Msk) >> DMA_SxCR_PSIZE_Pos;
    uint32_t msize =
        (dma_stream->CR.reg & DMA_SxCR_MSIZE_Msk) >> DMA_SxCR_MSIZE_Pos;

    if ((psize > (uint32_t)DMA_DATA_WIDTH_WORD) ||
        (msize > (uint32_t)DMA_DATA_WIDTH_WORD))
    {
        return DMA_ERROR_INVALID_PARAM;
    }

    uint32_t periph_alignment = 1U << psize;
    uint32_t memory_alignment = 1U << msize;

    if (((transfer->periph_address & (periph_alignment - 1U)) != 0U) ||
        ((transfer->memory_address & (memory_alignment - 1U)) != 0U))
    {
        return DMA_ERROR_INVALID_PARAM;
    }

    if (psize < msize)
    {
        uint32_t required_multiple = 1U << (msize - psize);
        if ((transfer->item_count % required_multiple) != 0U)
        {
            return DMA_ERROR_INVALID_PARAM;
        }
    }

    dma_stream->PAR = (uint32_t)transfer->periph_address;
    dma_stream->M0AR = (uint32_t)transfer->memory_address;
    dma_stream->NDTR.reg = transfer->item_count;

    int status = dma_clear_flags(controller, stream, DMA_EVENT_ALL);
    if (status != 0)
    {
        return status;
    }

    dma_stream->CR.reg |= DMA_SxCR_EN;
    return 0;
}

int dma_poll_complete(DMA_TypeDef *controller, DMA_StreamIndex_t stream,
                      uint32_t timeout)
{
    if ((!dma_is_valid_controller(controller)) ||
        (!dma_is_valid_stream(stream)) || (timeout == 0U))
    {
        return DMA_ERROR_INVALID_PARAM;
    }

    DMA_Stream_TypeDef *dma_stream = dma_get_stream(controller, stream);
    if ((dma_stream->CR.reg & DMA_SxCR_CIRC) != 0U)
    {
        return DMA_ERROR_UNSUPPORTED;
    }

    while (timeout > 0U)
    {
        uint32_t flags = 0U;
        int status = dma_get_flags(controller, stream, &flags);
        if (status != 0)
        {
            return status;
        }

        if ((flags & (DMA_EVENT_FIFO_ERROR |
                      DMA_EVENT_DIRECT_ERROR |
                      DMA_EVENT_TRANSFER_ERROR)) != 0U)
        {
            return DMA_ERROR_TRANSFER;
        }

        if ((flags & DMA_EVENT_TRANSFER_COMPLETE) != 0U)
        {
            return 0;
        }

        --timeout;
    }

    return DMA_ERROR_TIMEOUT;
}
