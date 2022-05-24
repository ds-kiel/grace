Import("env")

# Addings this flags through build_flags causes the linker options to be appended to all compilation steps,
# except for the final compilation of the firmware.hex.
env.Append(LINKFLAGS=["-Wl-bDSCR_AREA=0x3e00",  "-Wl-bINT2JT=0x3f00"])
