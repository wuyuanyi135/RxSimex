function(register_target s_function_name sources extra_libs)
    matlab_add_mex(
            NAME ${s_function_name}
            SRC ${_SOURCES} ${sources}
            LINK_TO ${Matlab_MEX_LIBRARY} ${extra_libs}
    )
    target_compile_definitions(${s_function_name} PRIVATE S_FUNCTION_NAME=${s_function_name})
endfunction()