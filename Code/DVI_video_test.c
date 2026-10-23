
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "software\libdvi\dvi.h"
#include "software\libdvi\dvi_serialiser.h"
#include "software\libdvi\tmds_encode.h"
#include "software\libsprite\sprite.h"

#include "blink.pio.h"



#define FRAME_WIDTH 640
#define FRAME_HEIGHT 240
#define VREG_VSEL VREG_VOLTAGE_1_20
#define DVI_TIMING dvi_timing_640x480p_60hz

struct dvi_inst dvi0;

static uint16_t line_buffer[640];


volatile int8_t frame[144][160];

#define NB_PAL 6

const uint16_t color[NB_PAL][5] = {
{0xf7be, 0x528a, 0x9cd3, 0x0000},
{0xc6f1, 0x3b07, 0x852c, 0x08c2},
{0xfeb2, 0xfc53, 0xfded, 0x7377},
{0xe6b6, 0x6466, 0x2e2b, 0x2945},
{0xa508, 0x72c0, 0x4460, 0x4aa0},
{0xffff, 0xf800, 0xffe0, 0x0000},//!!!!!!!!!!!
{0xe6a8, 0x6b4d, 0xe588, 0x0000}
};

uint8_t pal_offset = 0;
bool pal_button = false;

static uint scanline = 0;

static const struct dvi_serialiser_cfg dvi_cfg = {
    .pio = pio1,              
    .sm_tmds = {0, 1, 2},
    .pins_tmds = {12, 18, 16}, // D0, D1, D2
    .pins_clk = 14,            
    .invert_diffpairs = false,
    .prog_offs = 0
};



// --- RENDER LOOP (Core 1) --
void __not_in_flash("core1_main") core1_main() {
    dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0); 
    dvi_start(&dvi0); 
    printf("[Core 1] DVI démarré !\n");
    dvi_scanbuf_main_16bpp(&dvi0); 
    __builtin_unreachable();
}

void core1_scanline_callback() {

    if(gpio_get(9)&&(pal_button==false)){
            pal_button=true;
            pal_offset = (pal_offset + 1) % NB_PAL;
    }

    if(!gpio_get(9)){
            pal_button=false;
    }

    for(int x=0;x<160;x++){
        if(scanline >= 48 && scanline < 144+48 && x < 160){
        line_buffer[x+80] = color[pal_offset][frame[scanline-48][x]];
        }
        if(scanline >= 144+48){
            line_buffer[x+80] = 0x0000;
        }
        if(scanline < 48){
            line_buffer[x+80] = 0x0000;
        }
    }

    const uint32_t *tmds_buf= (uint32_t *)line_buffer;
        
    queue_try_add_u32(&dvi0.q_colour_valid, &tmds_buf);
    while (queue_try_remove_u32(&dvi0.q_colour_free, &tmds_buf));
    scanline = (scanline + 1) % FRAME_HEIGHT;
}

// --- DECODE LOGIC (Core 0) ---
int main() {
    // Overclock
    vreg_set_voltage(VREG_VSEL);
    sleep_ms(10);
    set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);

    stdio_init_all();

    const PIO pio = pio0;
    const uint sm = 0;

    int pixel16 = 0;
    int line = 0;

    uint32_t data;

    int i;

    bool vsync = false;

    int capture;

    uint offset = pio_add_program(pio, &capture_propre_program);


    setup_entrees_init(pio, sm, offset);

    pio_sm_put_blocking(pio, sm, 159);

    // Configuration DVI
    dvi0.timing = &DVI_TIMING;
    dvi0.ser_cfg = dvi_cfg;
    dvi0.scanline_callback = core1_scanline_callback;
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

    sprite_fill16(line_buffer, 0x0000, FRAME_WIDTH);
    uint16_t *tmds_buf = line_buffer;
    queue_add_blocking_u32(&dvi0.q_colour_valid, &tmds_buf);
    tmds_buf += FRAME_WIDTH;
    queue_add_blocking_u32(&dvi0.q_colour_valid, &tmds_buf);
  
    multicore_launch_core1(core1_main);
    //sleep_ms(200);

    // Main Loop
    while (1) {
        if (!pio_sm_is_rx_fifo_empty(pio, sm)) {
            data = pio_sm_get_blocking(pio, sm);
        
            for (i = 0; i < 16; i++) {
            // On isole les 2 bits du pixel actuel
                int x_target = (pixel16 * 16) + i;
                if (x_target < 160) {
                    frame[line][x_target] = (data >> (i * 2)) & 0b11;
                }
            } 
            pixel16++; 
        }

        if(pixel16 == 10){
            pixel16=0;
            line ++;
        }

        if(line>143){
            line=0;
            capture++;
            if(capture<300){
            queue_try_add_u32(&dvi0.q_colour_valid, &tmds_buf);
            while (queue_try_remove_u32(&dvi0.q_colour_free, &tmds_buf));
            }
        }

        if(gpio_get(4)&&(vsync==false)){
            line=0;
            pixel16=0;
            vsync=true;
        }

        if(!gpio_get(4)){
            vsync=false;
        }

        if (pio_sm_get_tx_fifo_level(pio, sm) < 1) {
            pio_sm_put(pio, sm, 159); 
        }
       // __wfi(); // Mise en veille en attendant une interruption
    }
    __builtin_unreachable();
}
