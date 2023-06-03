/*
   Arduino Watch Lite Version
   You may find full version at: https://github.com/moononournation/ArduinoWatch
*/

/*******************************************************************************
 * Start of Arduino_GFX setting
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <lvgl.h>

// #define GFX_PWD 15
#define GFX_BL 4


Arduino_DataBus *bus = new Arduino_ESP32SPI(16 /* DC */, 5 /* CS */, 18 /* SCK */, 19 /* MOSI */, -1 /* MISO */);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 23 /* RST */, 1 /* rotation */, true /* IPS */, 135 /* width */, 240 /* height */, 52 /* col offset 1 */, 40 /* row offset 1 */, 53 /* col offset 2 */, 40 /* row offset 2 */);


/* Change to your screen resolution */
static uint32_t screenWidth;
static uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
  lv_disp_flush_ready(disp);
}

lv_obj_t *counter_label;
volatile int counter = 0;  // Counter variable
unsigned long lastUpdate = 0; // to store the time of the last update
lv_obj_t * label2;
lv_obj_t * label3;
lv_obj_t * label4;
lv_obj_t * chart;
lv_chart_series_t * ser1;

static lv_style_t style;
lv_obj_t *clock_box;
lv_obj_t *temp_box;
lv_obj_t * cont;

void initLVGL() {
#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  gfx->begin();
  gfx->fillScreen(BLACK);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  lv_init();
  screenWidth = gfx->width();
  screenHeight = gfx->height();
	Serial.printf("width %d height %d\n", screenWidth, screenHeight);
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * 32,
                                                 MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!disp_draw_buf) {
    Serial.println("LVGL disp_draw_buf allocate failed!");
  } else {
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * 32);

    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    /* Change the following line to your display resolution */
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

		lv_style_init(&style);
		lv_style_set_bg_color(&style, lv_palette_lighten(LV_PALETTE_GREY, 3));
		lv_style_set_border_color(&style, lv_palette_main(LV_PALETTE_BLUE_GREY));
		lv_style_set_border_width(&style, 1);
		lv_style_set_radius(&style, 5);

    counter_label = lv_label_create(lv_scr_act());
    lv_label_set_text(counter_label, "This is line 1");

		clock_box = lv_obj_create(lv_scr_act());
		lv_obj_add_style(clock_box, &style, 0);
		lv_obj_set_width(clock_box, LV_PCT(100));  // Set the width of the container to the screen width
		lv_obj_set_pos(clock_box, 0, 0); // Position the container at the top of the screen

		lv_obj_set_parent(counter_label, clock_box);
		lv_obj_align(counter_label, LV_ALIGN_LEFT_MID, 0, 0);

		lv_obj_set_height(clock_box,
				lv_obj_get_height(counter_label)
				+ 2*lv_obj_get_style_pad_top(clock_box, 0)); // Set the height based on the label's height plus some padding

		temp_box = lv_obj_create(lv_scr_act());
		lv_obj_add_style(temp_box, &style, 0);
		lv_obj_set_width(temp_box, LV_PCT(100));  // Set the width of the container to the screen width
		lv_obj_set_height(clock_box, 40);
		lv_obj_set_pos(temp_box, 0,
						lv_obj_get_y(clock_box) + lv_obj_get_height(clock_box)); // Position the container below the first one
			

		/* Create a container */
//		cont = lv_obj_create(lv_scr_act());
//	lv_obj_add_style(cont, &style, 0);
//	lv_obj_set_size(cont, 150, 50);  // Set the size of the container
//   lv_obj_align_to(cont, counter_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

    
    label2 = lv_label_create(lv_scr_act());
    lv_label_set_text(label2, "This is line 2");
		lv_obj_set_parent(label2, clock_box);
		lv_obj_set_pos(label2, 0, 0); // Position the container at the top of the screen

  //  label3 = lv_label_create(lv_scr_act());
//    lv_label_set_text(label3, "This is line 3");
//		lv_obj_set_parent(label3, clock_box);
//    lv_obj_align_to(label3, label2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

//    chart = lv_chart_create(lv_scr_act());
//    lv_obj_set_size(chart, 80, 40);
//    lv_obj_align_to(chart, label4, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
//    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);   /*Show lines and points too*/
//    ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);


  }
}


void setup()
{
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("LVGL Hello World Demo");
  initLVGL();
}


void loop()
{
	if (millis() - lastUpdate > 1000) { // If 5000ms have passed
			counter++;
			lv_label_set_text_fmt(counter_label, "Counter: %d", counter);
//			lv_chart_set_next_value(chart, ser1, counter);
			lastUpdate = millis();
	}
	lv_task_handler();
	delay(5);
}

