# import argparse
import os
import re
import shutil
import sys
from pathlib import Path

sys.path.append(str(Path(__file__).parent.parent))

from sys import exit
from win_build_obs import check_call, CMAKE_VS_ARGS, spa, setup_obs
from win_shared import package_zip, get_google_api_key_arg


def main():
	root_dir = Path(os.getcwd())
	ci_root_dir = root_dir.joinpath("CI_build")
	build_deps_dir = ci_root_dir.joinpath("build_deps")
	ci_root_dir.mkdir(exist_ok = True)
	build_deps_dir.mkdir(exist_ok = True)
	print("root_dir:", repr(str(root_dir)))
	print("ci_root_dir:", repr(str(ci_root_dir)))
	print("build_deps_dir:", repr(str(build_deps_dir)))

	cmake_text = root_dir.parent.parent.joinpath("CMakeLists.txt").read_text()
	cmake_text = re.sub(r"\s+", "", cmake_text)
	version = re.search('set\(VERSION_STRING"(.*?)"\)', cmake_text).group(1)
	if not version:
		raise ValueError("no version found")
	print(f"VERSION_STRING: {version!r}")

	obs_studio = build_deps_dir.joinpath("obs-studio")
	CLEAN_OBS = os.environ.get("CLEAN_OBS")
	clean_afterwards = CLEAN_OBS in ("1", "true")
	print("CLEAN_OBS clean_afterwards", (CLEAN_OBS, clean_afterwards))
	obs_studio_src, obs_deps_dir, build_installed_dir = setup_obs(obs_studio, clean_afterwards = clean_afterwards)

	check_call(["cmd", "/C", str(root_dir.joinpath("clone_plibsys.cmd"))], cwd = ci_root_dir)
	build_dir = ci_root_dir.joinpath("build")
	installed_dir = ci_root_dir.joinpath("installed")
	build_dir.mkdir(exist_ok = True)
	check_call([
		"cmake",
		*CMAKE_VS_ARGS,
		r"-DCMAKE_BUILD_TYPE=RelWithDebInfo",
		r"-DCMAKE_GENERATOR_PLATFORM=x64",
		r"-DSPEECH_API_GOOGLE_HTTP_OLD=ON",
		"-DBUILD_SHARED_LIBS=ON",
		f"-DOBS_SOURCE_DIR={str(obs_studio_src)}",
		f"-DOBS_LIB_DIR={str(build_installed_dir)}",
		f"-DOBS_DEPS_DIR={str(obs_deps_dir)}",
		f"-DCMAKE_INSTALL_PREFIX:PATH={str(installed_dir)}",
		get_google_api_key_arg(),
		str(root_dir.parent.parent),
	], cwd = build_dir)
	check_call(spa("cmake --build . --config RelWithDebInfo"), cwd = build_dir)
	check_call(spa("cmake --install . --config RelWithDebInfo"), cwd = build_dir)

	release = ci_root_dir.joinpath("release")
	package_zip(release, installed_dir, version)


if __name__ == '__main__':
	main()
