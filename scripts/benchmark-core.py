import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import tempfile
import time

project = Path(__file__).resolve().parents[1]
reference, json_include, crypto = sys.argv[1:]
files = ['src/core/backups.cpp', 'src/core/backups.h', 'src/core/file_ops.cpp',
         'src/core/file_ops.h', 'src/core/checksum.cpp', 'src/core/checksum.h']
result = {'baseline_ref': reference, 'machine': platform.machine(),
          'compiler': subprocess.check_output([os.environ.get('CXX', 'c++'), '--version'], text=True).splitlines()[0],
          'fixture': '128 x 4096 bytes, 7 operations per scenario, warm filesystem cache, median', 'measurements': {}}
with tempfile.TemporaryDirectory(prefix='dota-benchmark-') as directory:
    temporary = Path(directory)
    for name in ['baseline', 'working']:
        source = project
        if name == 'baseline':
            source = temporary / 'baseline'
            for file in files:
                path = source / file
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(subprocess.check_output(['git', 'show', reference + ':' + file], cwd=project))
        executable = temporary / (name + '-benchmark')
        command = [os.environ.get('CXX', 'c++'), '-O2', '-DNDEBUG', '-std=c++17',
                   '-I' + str(source / 'src/core'), '-I' + json_include,
                   '-I' + str(Path(crypto) / 'include'), '-L' + str(Path(crypto) / 'lib'),
                   str(project / 'tests/transfer_benchmark.cpp')]
        command += [str(source / file) for file in files if file.endswith('.cpp')]
        if name == 'working':
            command += [str(project / 'src/core/config_preview.cpp')]
        command += ['-lcrypto', '-o', str(executable)]
        subprocess.run(command, check=True)
        fixture = temporary / (name + '-fixture')
        started = time.monotonic()
        process = subprocess.Popen([str(executable), str(fixture)], stdout=subprocess.PIPE, text=True)
        _, status, usage = os.wait4(process.pid, 0)
        process.returncode = os.waitstatus_to_exitcode(status)
        output = process.stdout.read()
        process.stdout.close()
        if process.returncode:
            raise RuntimeError(name + ' benchmark failed')
        result['measurements'][name] = {key: float(value) for key, value in
                                       (line.split('=') for line in output.splitlines())}
        result['measurements'][name].update(process_wall_seconds=time.monotonic() - started,
            process_user_seconds=usage.ru_utime, process_system_seconds=usage.ru_stime,
            process_peak_rss_bytes=usage.ru_maxrss * (1 if sys.platform == 'darwin' else 1024))
print(json.dumps(result, indent=2))
