import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

root = Path(__file__).resolve().parents[1]

class PrereleaseChecks(unittest.TestCase):
    def test_workflow_separates_stable_and_alpha_publishers(self):
        workflow = (root / '.github/workflows/build.yaml').read_text()
        self.assertIn('branches: [ "main", "alpha", "aur-packaging" ]', workflow)
        stable = workflow.split('\n  release:\n', 1)[1].split('\n  prerelease:\n', 1)[0]
        alpha = workflow.split('\n  prerelease:\n', 1)[1]
        self.assertIn("if: github.ref == 'refs/heads/main'", stable)
        self.assertIn('--latest=true', stable)
        self.assertIn("if: github.ref == 'refs/heads/alpha'", alpha)
        self.assertIn('bash scripts/publish-prerelease.sh', alpha)
        self.assertNotIn('gh release delete', alpha)
        self.assertNotIn('git push origin', alpha)
        self.assertNotIn('--latest=true', alpha)

    def test_channel_selection(self):
        workflow = (root / '.github/workflows/build.yaml').read_text()
        selection = workflow.split('      - name: Select distribution channel\n', 1)[1].split('      - name: Compute version\n', 1)[0]
        script = '\n'.join(line[10:] for line in selection.split('        run: |\n', 1)[1].splitlines())
        with tempfile.TemporaryDirectory(prefix='dota-channel-') as directory:
            output = Path(directory) / 'output'
            for branch, channel in [('main', 'stable'), ('alpha', 'alpha'), ('aur-packaging', 'stable'), ('other', None)]:
                output.unlink(missing_ok=True)
                result = subprocess.run(['bash', '-c', script], env=dict(os.environ, GITHUB_REF='refs/heads/' + branch, GITHUB_OUTPUT=str(output)), capture_output=True)
                if channel:
                    self.assertEqual(result.returncode, 0)
                    self.assertEqual(output.read_text().strip(), 'channel=' + channel)
                else:
                    self.assertNotEqual(result.returncode, 0)
                    self.assertFalse(output.exists())

    def test_stable_publisher_keeps_update_contract(self):
        workflow = (root / '.github/workflows/build.yaml').read_text()
        stable = workflow.split('\n  release:\n', 1)[1].split('\n  prerelease:\n', 1)[0]
        script = '\n'.join(line[10:] for line in stable.split('        run: |\n', 1)[1].splitlines())
        with tempfile.TemporaryDirectory(prefix='dota-stable-release-') as directory:
            folder = Path(directory)
            (folder / 'bin').mkdir()
            capture = folder / 'commands.jsonl'
            for command in ('git', 'gh'):
                mock = folder / 'bin' / command
                mock.write_text('#!/usr/bin/env python3\nimport json, os, sys\nwith open(os.environ["CAPTURE"], "a") as output:\n output.write(json.dumps(["' + command + '"] + sys.argv[1:]) + "\\n")\n')
                mock.chmod(0o700)
            for file in ('DotaManager-Windows/DotaManager.exe', 'DotaManager-Linux/test.AppImage'):
                path = folder / file
                path.parent.mkdir(exist_ok=True)
                path.write_bytes(b'fixture')
            env = dict(os.environ, MOCK_BIN=str(folder / 'bin'), RELEASE_CHANNEL='stable', GITHUB_REF='refs/heads/main', VERSION='v1.24', GITHUB_SHA='a' * 40, CAPTURE=str(capture))
            script = 'export PATH="$(cd "$MOCK_BIN" && pwd):$PATH"\n' + script
            subprocess.run(['bash', '-c', script], cwd=folder, env=env, check=True)
            calls = [json.loads(line) for line in capture.read_text().splitlines()]
            self.assertIn(['git', 'tag', 'v1.24'], calls)
            self.assertIn(['git', 'push', 'origin', 'v1.24'], calls)
            self.assertIn(['gh', 'release', 'delete', 'latest', '--yes', '--cleanup-tag'], calls)
            published = next(call for call in calls if call[:4] == ['gh', 'release', 'create', 'latest'])
            self.assertIn('--latest=true', published)
            self.assertNotIn('--prerelease', published)
            self.assertEqual(published[published.index('--title') + 1], 'Latest Build (v1.24)')
            self.assertEqual(published[published.index('--target') + 1], 'a' * 40)
            self.assertEqual(published[-2:], ['DotaManager-Windows/DotaManager.exe', 'DotaManager-Linux/test.AppImage'])
            for override in ({'GITHUB_REF': 'refs/heads/alpha'}, {'RELEASE_CHANNEL': 'alpha'}, {'VERSION': 'latest'}, {'GITHUB_SHA': '../bad'}):
                capture.unlink(missing_ok=True)
                result = subprocess.run(['bash', '-c', script], cwd=folder, env=dict(env, **override), capture_output=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse(capture.exists())

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
