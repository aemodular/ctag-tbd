# add sources to config, headers added for intellij
set(SRCS_FILES
        DaisySP/Source/Control/adenv.cpp
        DaisySP/Source/Control/adsr.cpp
        DaisySP/Source/Control/line.cpp
        DaisySP/Source/Control/phasor.cpp
        DaisySP/Source/Drums/analogbassdrum.cpp
        DaisySP/Source/Drums/analogsnaredrum.cpp
        DaisySP/Source/Drums/hihat.cpp
        DaisySP/Source/Drums/synthbassdrum.cpp
        DaisySP/Source/Drums/synthsnaredrum.cpp
        DaisySP/Source/Dynamics/balance.cpp
        DaisySP/Source/Dynamics/compressor.cpp
        DaisySP/Source/Dynamics/crossfade.cpp
        DaisySP/Source/Dynamics/limiter.cpp
        DaisySP/Source/Effects/autowah.cpp
        DaisySP/Source/Effects/bitcrush.cpp
        DaisySP/Source/Effects/chorus.cpp
        DaisySP/Source/Effects/decimator.cpp
        DaisySP/Source/Effects/flanger.cpp
        DaisySP/Source/Effects/fold.cpp
        DaisySP/Source/Effects/overdrive.cpp
        DaisySP/Source/Effects/reverbsc.cpp
        DaisySP/Source/Effects/phaser.cpp
        DaisySP/Source/Effects/sampleratereducer.cpp
        DaisySP/Source/Effects/tremolo.cpp
        DaisySP/Source/Filters/allpass.cpp
        DaisySP/Source/Filters/atone.cpp
        DaisySP/Source/Filters/biquad.cpp
        DaisySP/Source/Filters/comb.cpp
        DaisySP/Source/Filters/mode.cpp
        DaisySP/Source/Filters/moogladder.cpp
        DaisySP/Source/Filters/nlfilt.cpp
        DaisySP/Source/Filters/svf.cpp
        DaisySP/Source/Filters/tone.cpp
        DaisySP/Source/Noise/clockednoise.cpp
        DaisySP/Source/Noise/grainlet.cpp
        DaisySP/Source/Noise/particle.cpp
        DaisySP/Source/PhysicalModeling/drip.cpp
        DaisySP/Source/PhysicalModeling/modalvoice.cpp
        DaisySP/Source/PhysicalModeling/pluck.cpp
        DaisySP/Source/PhysicalModeling/resonator.cpp
        DaisySP/Source/PhysicalModeling/KarplusString.cpp
        DaisySP/Source/PhysicalModeling/stringvoice.cpp
        DaisySP/Source/Synthesis/blosc.cpp
        DaisySP/Source/Synthesis/fm2.cpp
        DaisySP/Source/Synthesis/formantosc.cpp
        DaisySP/Source/Synthesis/oscillator.cpp
        DaisySP/Source/Synthesis/oscillatorbank.cpp
        DaisySP/Source/Synthesis/variablesawosc.cpp
        DaisySP/Source/Synthesis/variableshapeosc.cpp
        DaisySP/Source/Synthesis/vosim.cpp
        DaisySP/Source/Synthesis/zoscillator.cpp
        DaisySP/Source/Utility/dcblock.cpp
        DaisySP/Source/Utility/jitter.cpp
        DaisySP/Source/Utility/metro.cpp
        DaisySP/Source/Utility/port.cpp
        )

set(INC_DIRS
        "DaisySP/Source"
        "DaisySP/Source/Control"
        "DaisySP/Source/Drums"
        "DaisySP/Source/Dynamics"
        "DaisySP/Source/Effects"
        "DaisySP/Source/Filters"
        "DaisySP/Source/Noise"
        "DaisySP/Source/PhysicalModeling"
        "DaisySP/Source/Synthesis"
        "DaisySP/Source/Utility"
        )

if (TBD_SIM)
    add_library(daisysp STATIC ${SRCS_FILES})
    target_include_directories(daisysp PRIVATE ${INC_DIRS})
else ()
    idf_component_register(SRCS ${SRCS_FILES}
            INCLUDE_DIRS ${INC_DIRS}
            PRIV_REQUIRES )
endif ()