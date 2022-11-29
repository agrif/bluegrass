# tiny script to grab a git hash and define it during compile

import subprocess

try:
    revision = subprocess.check_output(['git', 'describe', '--exclude', '*', '--always', '--dirty']).strip().decode('utf-8')
except Exception:
    revision = 'unknown'

print("'-DBLUEGRASS_GIT=\"" + revision + "\"'")
