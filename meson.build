project(
  'dud',
  'c',
  version: '0.0.0',
  license: 'Apache',
  default_options: ['warning_level=3', 'werror=true', 'c_std=c17'],
)

cc = meson.get_compiler('c')

m_dep = cc.find_library('m', required: false)

src = [
  'src/allocator.c',
]

executable('dud', ['src/main.c', src], dependencies: m_dep, install: true)

tests = [
  'test_allocator',
]

foreach test_name : tests
  test_exe = executable(
    test_name,
    files('tests' / test_name + '.c'),
    src,
    include_directories: include_directories('src')
  )
  test(test_name, test_exe)
endforeach
