# SPDX-License-Identifier: Apache-2.0
tests = [
  'test-rcu',
  'test-audiostream',
  'test-audioprocessor',
]

qt_tests = [
  'test-localchannel',
  'test-oggvorbisdecoder',
  'test-oggvorbisencoder',
  'test-resampler',
]

# Execute tests in the source directory so they can access data files
tests_dir = meson.current_source_dir()

foreach name : tests
  exe = executable(name,
                   name + '.cpp',
		   dependencies : dependency('threads'),
		   include_directories : inc,
		   link_with : libaudio)
  test(name, exe, workdir : tests_dir)
endforeach

foreach name : qt_tests
  exe = executable(name,
                   name + '.cpp',
	               dependencies : dependencies,
	               include_directories : inc,
	               link_with : [libaudio, libcore])
  test(name, exe, workdir : tests_dir)
endforeach
