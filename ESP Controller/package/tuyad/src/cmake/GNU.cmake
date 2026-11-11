
# Specific settings for GNU compiler

add_compile_options(-Wall;-Wextra;-Wno-unused;-Wno-unknown-pragmas;-Wno-sign-compare;-Wwrite-strings;-Wno-unused;-Wno-narrowing;-Wno-ignored-qualifiers;-Wno-inline)

add_compile_options("$<$<CONFIG:RELEASE>:-O3>")
add_compile_options("$<$<CONFIG:DEBUG>:-O0;-g3>")
