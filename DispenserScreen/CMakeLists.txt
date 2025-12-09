SET(SOURCES ui_comp_btnnextcomp.c
    ui_comp_selectbasic.c
    ui_comp.c
    ui_SC00Registrar.c
    ui_SC01Pulsera.c
    ui_SC02Selection.c
    ui_SC03Dispensar.c
    ui_SC04Finalizar.c
    ui_theme_manager.c
    ui_themes.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_naos_oscuro_h_png.c
    ui_img_wristband_green_png.c
    ui_img_rfid_png.c
    ui_img_naos_claro_h_png.c
    ui_img_beer_mug_png.c
    ui_img_cocktail_png.c
    ui_img_red_wine_png.c)

add_library(ui ${SOURCES})
