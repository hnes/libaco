macro (aco_register_bench bench)
  set(name ${bench})

  add_executable(bench_${name} ${name}.c)
  add_dependencies(bench_${name} libaco::aco)

  aco_target_configure(bench_${name} libaco::aco ${ARGN})
endmacro ()

aco_register_bench(benchmark)
