project('EdgeFixer', 'c',
    version : '2',
    default_options : ['buildtype=release', 'b_ndebug=if-release', 'c_std=c99'],
)

# Sources
sources = [
    'EdgeFixer/edgefixer.c',
    'EdgeFixer/edgefixer.h',
    'EdgeFixer/vsplugin.c'
]

# Dependencies
vapoursynth_dep = dependency('vapoursynth').partial_dependency(compile_args : true, includes : true)

# Libs
shared_module('EdgeFixer', sources,
    dependencies : [vapoursynth_dep],
    install : true,
    install_dir : join_paths(vapoursynth_dep.get_pkgconfig_variable('libdir'), 'vapoursynth'),
    gnu_symbol_visibility : 'hidden'
)
