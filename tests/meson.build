# SPDX-License-Identifier: Apache-2.0
test('rcu', executable('test-rcu', 'test-rcu.cpp', '../rcu.cpp', include_directories : inc, dependencies : dependency('threads')))
test('audiostream', executable('test-audiostream', 'test-audiostream.cpp', '../AudioStream.cpp', include_directories : inc, dependencies : dependency('threads')))
test('audioprocessor', executable('test-audioprocessor', 'test-audioprocessor.cpp', '../AudioProcessor.cpp', '../AudioStream.cpp', '../rcu.cpp', include_directories : inc, dependencies : dependency('threads')))
