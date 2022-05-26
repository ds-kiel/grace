Import("env", "projenv")

from os.path import join

VID = int(env.GetProjectOption("vid"), 16)
PID = int(env.GetProjectOption("pid"), 16)

libdeps_dir = env.get("PROJECT_LIBDEPS_DIR")
board = env.get("BOARD")
fx2lib_utils = join(libdeps_dir, board, "fx2lib", "utils")

# Addings this flags through build_flags causes the linker options to be appended to all compilation steps,
# except for the final compilation of the firmware.hex.
env.Append(LINKFLAGS=["-Wl-bDSCR_AREA=0x3e00",  "-Wl-bINT2JT=0x3f00", "--xram-loc", env.BoardConfig().get("build")["loc_xram"]])

env.AddCustomTarget(
    "bix",
    "$BUILD_DIR/${PROGNAME}.hex",
    "$OBJCOPY -I ihex -O binary $BUILD_DIR/${PROGNAME}.hex $BUILD_DIR/${PROGNAME}.bix"
)

env.AddCustomTarget(
    "iix",
    "$BUILD_DIR/${PROGNAME}.hex",
    "{} -v {} -p {} {} {}".format(join(fx2lib_utils, "ihx2iic.py"), VID, PID, "$BUILD_DIR/${PROGNAME}.hex", "$BUILD_DIR/${PROGNAME}.iic")
)
