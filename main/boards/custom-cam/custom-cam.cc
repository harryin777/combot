#include "config.h"
#include "board.h"
#include "display.h"
#include "esp32_camera.h"
#include "no_audio_codec.h"
#include "afe_audio_processor.h"
#include "afe_wake_word.h"

#include <esp_log.h>

#define TAG "CustomCamBoard"

class CustomCamBoard : public WifiBoard {
private:
    spi_bus_handle_t spi_bus_;
    esp_lcd_panel_io_handle_t panel_io_;
    esp_lcd_panel_handle_t panel_;
    Display* display_;
    Button boot_button_;
    Camera* camera_;
    AudioCodec* audio_codec_;
    AudioProcessor* audio_processor_;
    WakeWord* wake_word_;

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_SPI_MOSI_PIN;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_SPI_SCK_PIN;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
        spi_bus_ = SPI3_HOST;
    }

    void InitializeSt7789Display() {
        ESP_LOGD(TAG, "Install panel IO");
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_SPI_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = DISPLAY_SPI_MODE;
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(spi_bus_, &io_config, &panel_io_));

        ESP_LOGD(TAG, "Install LCD driver");
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_PIN;
        panel_config.rgb_ele_order = DISPLAY_RGB_ORDER;
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panel_io_, &panel_config, &panel_));

        ESP_LOGD(TAG, "Reset and initialize LCD panel");
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_, DISPLAY_INVERT_COLOR));
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_, DISPLAY_SWAP_XY));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y));
        ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_, true));

        display_ = new LcdDisplay(panel_io_, panel_, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
    }

    void InitializeCamera() {
        camera_config_t config = {};
        config.ledc_channel = LEDC_CHANNEL_2;
        config.ledc_timer = LEDC_TIMER_2;
        config.pin_d0 = CAMERA_PIN_D0;
        config.pin_d1 = CAMERA_PIN_D1;
        config.pin_d2 = CAMERA_PIN_D2;
        config.pin_d3 = CAMERA_PIN_D3;
        config.pin_d4 = CAMERA_PIN_D4;
        config.pin_d5 = CAMERA_PIN_D5;
        config.pin_d6 = CAMERA_PIN_D6;
        config.pin_d7 = CAMERA_PIN_D7;
        config.pin_xclk = CAMERA_PIN_XCLK;
        config.pin_pclk = CAMERA_PIN_PCLK;
        config.pin_vsync = CAMERA_PIN_VSYNC;
        config.pin_href = CAMERA_PIN_HREF;
        config.pin_sccb_sda = CAMERA_PIN_SIOD;
        config.pin_sccb_scl = CAMERA_PIN_SIOC;
        config.pin_pwdn = CAMERA_PIN_PWDN;
        config.pin_reset = CAMERA_PIN_RESET;
        config.xclk_freq_hz = XCLK_FREQ_HZ;
        config.pixel_format = PIXFORMAT_RGB565;
        config.frame_size = FRAMESIZE_VGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_PSRAM;
        config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

        camera_ = new Esp32Camera(config);
        ESP_LOGI(TAG, "Camera initialized");
    }

    void InitializeButtons() {
        boot_button_.Initialize(BOOT_BUTTON_GPIO, [this]() {
            this->OnButtonPressed();
        });
    }

    void InitializeAudio() {
        audio_codec_ = new NoAudioCodec();
        
#ifdef CONFIG_USE_AUDIO_PROCESSOR
        audio_processor_ = new AfeAudioProcessor();
#else
        audio_processor_ = nullptr;
#endif

#ifdef CONFIG_USE_AFE_WAKE_WORD
        wake_word_ = new AfeWakeWord();
#else
        wake_word_ = nullptr;
#endif
    }

public:
    CustomCamBoard() : display_(nullptr), camera_(nullptr), audio_codec_(nullptr), audio_processor_(nullptr), wake_word_(nullptr) {}

    void Initialize() override {
        ESP_LOGI(TAG, "Initializing CustomCamBoard");
        
        InitializeSpi();
        
#ifdef CONFIG_LCD_ST7789_240X320
        InitializeSt7789Display();
#endif
        
        InitializeCamera();
        InitializeButtons();
        InitializeAudio();
        
        ESP_LOGI(TAG, "CustomCamBoard initialized successfully");
    }

    Display* GetDisplay() override {
        return display_;
    }

    Camera* GetCamera() override {
        return camera_;
    }

    AudioCodec* GetAudioCodec() override {
        return audio_codec_;
    }

    AudioProcessor* GetAudioProcessor() override {
        return audio_processor_;
    }

    WakeWord* GetWakeWord() override {
        return wake_word_;
    }

    void OnButtonPressed() {
        ESP_LOGI(TAG, "Boot button pressed");
        GetApplication()->OnButtonPressed();
    }
};

Board* CreateBoard() {
    return new CustomCamBoard();
}
