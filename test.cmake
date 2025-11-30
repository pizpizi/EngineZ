macro(MacroAppend arr elm)
    set(${arr} ${${arr}};elem)
endmacro()


set(Letters "Alpha;Beta")
MacroAppend(Letters "gammad")
message("Letters contains: ${Letters}")
foreach(string IN LISTS Letters)
    message(${string})
endforeach()
