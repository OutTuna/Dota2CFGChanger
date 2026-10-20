import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

root = Path(__file__).resolve().parents[1]

class PrereleaseChecks(unittest.TestCase):
    def test_alpha_workflow_has_no_stable_publisher(self):
        workflow = (root / '.github/workflows/build.yaml').read_text()
        self.assertNotIn('gh release delete', workflow)
        self.assertNotIn('git push origin', workflow)
        self.assertNotIn('--latest=true', workflow)
        self.assertIn('branches: [ "alpha" ]', workflow)
        self.assertIn("if: github.ref == 'refs/heads/alpha'", workflow)

    def test_release_channel_isolation(self):
        with tempfile.TemporaryDirectory(prefix='dota-release-') as directory:
            folder = Path(directory)
            (folder / 'bin').mkdir()
            gh = folder / 'bin/gh'
            gh.write_text('#!/usr/bin/env python3\nimport json, os, sys\nfrom pathlib import Path\nPath(os.environ["CAPTURE"]).write_text(json.dumps(sys.argv[1:]))\n')
            gh.chmod(0o700)
            for file in ('DotaManager-Windows/DotaManager.exe', 'DotaManager-Linux/test.AppImage'):
                path = folder / file
                path.parent.mkdir(exist_ok=True)
                path.write_bytes(b'fixture')
            capture = folder / 'arguments.json'
            for channel in ('alpha',):
                env = dict(os.environ, PATH=str(folder / 'bin') + os.pathsep + os.environ['PATH'],
                           RELEASE_CHANNEL=channel, GITHUB_REF='refs/heads/' + channel, VERSION='v1.24',
                           GITHUB_RUN_ID='123', GITHUB_RUN_ATTEMPT='2', GITHUB_SHA='a' * 40, CAPTURE=str(capture))
                subprocess.run(['bash', str(root / 'scripts/publish-prerelease.sh')], cwd=folder, env=env, check=True)
                args = json.loads(capture.read_text())
                self.assertEqual(args[:3], ['release', 'create', channel + '-v1.24-123-2'])
                self.assertIn('--prerelease', args)
                self.assertIn('--latest=false', args)
                self.assertNotIn('latest', args)
                self.assertNotIn('delete', args)
                for override in ({'RELEASE_CHANNEL': 'stable'}, {'RELEASE_CHANNEL': 'beta'}, {'GITHUB_REF': 'refs/heads/main'},
                                 {'VERSION': 'latest'}, {'GITHUB_SHA': '../bad'}):
                    capture.unlink(missing_ok=True)
                    result = subprocess.run(['bash', str(root / 'scripts/publish-prerelease.sh')], cwd=folder,
                                            env=dict(env, **override), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                    self.assertNotEqual(result.returncode, 0, override)
                    self.assertFalse(capture.exists())

if __name__ == '__main__':
    unittest.main()
