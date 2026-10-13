idf_component_register(SRCS "main.c" "display.c" "touch.c" "clock.c" "wifi_time.c"
                            "../../common/app.c"
                            "../../common/assets_data.c"
                            "../../common/brain.c"
                            "../../common/gfx.c"
                            "../../common/light.c"
                            "../../common/render.c"
                            "../../common/scene.c"
                            "../../common/sky.c"
                            "../../common/species.c"
                            "../../common/tank.c"
                       INCLUDE_DIRS "." "../../common")

# The moment of this build, in seconds since 1970, for the clock's fallback.
string(TIMESTAMP BUILD_EPOCH "%s" UTC)
target_compile_definitions(${COMPONENT_LIB} PRIVATE BUILD_EPOCH=${BUILD_EPOCH})
