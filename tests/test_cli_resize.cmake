set(frames "${WORK}/cli-frames")
set(digits "[0-9][0-9][0-9][0-9][0-9][0-9]")

function(export_frames columns rows expected)
    execute_process(COMMAND "${APP}" "${INPUT}" --algo greedy --order alternate --max-width 44
        --remove-cols "${columns}" --remove-rows "${rows}"
        --frames-dir "${frames}" --out "${WORK}/cli/result" RESULT_VARIABLE status)
    if(NOT status STREQUAL "0")
        message(FATAL_ERROR "Resize exited with ${status}")
    endif()
    file(GLOB sequence "${frames}/step_${digits}_highlighted.png" "${frames}/step_${digits}_removed.png"
                       "${frames}/step_000000_original.png")
    list(LENGTH sequence count)
    if(NOT count EQUAL expected)
        message(FATAL_ERROR "Expected ${expected} playback frames, found ${count}")
    endif()
endfunction()

export_frames(3 2 10)
string(RANDOM LENGTH 12 marker_id)
set(marker "${frames}/step_note_${marker_id}_removed.png")
file(WRITE "${marker}" "user file")
export_frames(1 0 2)
export_frames(0 0 1)
if(NOT EXISTS "${frames}/step_000000_original.png")
    message(FATAL_ERROR "A zero-seam export must contain the original frame")
endif()
# Leave the 5-seam example for the CI artifact.
export_frames(3 2 10)
if(NOT EXISTS "${marker}")
    message(FATAL_ERROR "Resize removed an unrelated file from the frames folder")
endif()
file(REMOVE "${marker}")
