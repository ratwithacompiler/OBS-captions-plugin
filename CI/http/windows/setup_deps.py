import os
import sys
from pathlib import Path

sys.path.append(str(Path(__file__).parent.parent.parent))
os.chdir(Path(__file__).parent.parent)  # everything runs from the variant dir (CI/http)

from win_build_obs import check_call, setup_obs
from win_shared import ensure_plibsys


def main():
	root_dir = Path(os.getcwd())
	ci_root_dir = root_dir.joinpath("CI_build")
	build_deps_dir = ci_root_dir.joinpath("build_deps")
	ci_root_dir.mkdir(exist_ok = True)
	build_deps_dir.mkdir(exist_ok = True)
	print("root_dir:", repr(str(root_dir)))
	print("ci_root_dir:", repr(str(ci_root_dir)))
	print("build_deps_dir:", repr(str(build_deps_dir)))

	obs_studio = build_deps_dir.joinpath("obs-studio")
	CLEAN_OBS = os.environ.get("CLEAN_OBS", "1")  # clean by default to keep size small
	clean_afterwards = CLEAN_OBS in ("1", "true")
	print("CLEAN_OBS clean_afterwards", (CLEAN_OBS, clean_afterwards))
	setup_obs(obs_studio, clean_afterwards = clean_afterwards)

	ensure_plibsys(root_dir, ci_root_dir)


if __name__ == '__main__':
	main()
