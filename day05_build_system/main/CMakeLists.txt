idf_component_register(
    SRCS 
    "main.c" "dummy_module.c"
    INCLUDE_DIRS 
    "."
    #REQUIRES mathlib # public dependency on mathlib
    )

# Apply compiler flags for this component only
#target_compile_options(${COMPONENT_LIB} PRIVATE -O2)