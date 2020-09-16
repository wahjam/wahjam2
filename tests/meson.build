# SPDX-License-Identifier: Apache-2.0
tests = [
  'test-rcu',
  'test-audiostream',
  'test-audioprocessor',
]

foreach name : tests
  exe = executable(name,
                   name + '.cpp',
		   dependencies : dependency('threads'),
		   include_directories : inc,
		   link_with : libaudio)
  test(name, exe)
endforeach
