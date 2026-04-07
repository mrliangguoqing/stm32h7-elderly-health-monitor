SET(SOURCES components/ui_comp_uilabeltime.c
    components/ui_comp.c
    screens/ui_Screen1.c
    screens/ui_Screen2.c
    screens/ui_Screen3.c
    screens/ui_Screen4.c
    screens/ui_Screen5.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_background_image_png.c
    images/ui_img_home_active_page_png.c
    images/ui_img_weather_active_page_png.c
    images/ui_img_heart_rate_active_page_png.c
    images/ui_img_alerts_active_page_png.c
    images/ui_img_settings_active_page_png.c
    fonts/ui_font_RobotoMonoBold14.c
    fonts/ui_font_RobotoMonoBold26.c
    fonts/ui_font_RobotoMonoBold40.c
    fonts/ui_font_SourceHanSansBold26.c)

add_library(ui ${SOURCES})
