import os
import sys
from pathlib import Path

sys.path.append(str(Path(__file__).parent.parent.parent))
os.chdir(Path(__file__).parent.parent)  # everything runs from the variant dir (CI/http)

from win_shared import package_zip, get_version_string


def main():
	root_dir = Path(os.getcwd())
	ci_root_dir = root_dir.joinpath("CI_build")

	version = get_version_string(root_dir.parent.parent.joinpath("CMakeLists.txt"))

	release = ci_root_dir.joinpath("release")
	installed_dir = ci_root_dir.joinpath("installed")
	package_zip(release, installed_dir, version)


if __name__ == '__main__':
	main()
