import os
import sys
from pathlib import Path

sys.path.append(str(Path(__file__).parent.parent.parent))
os.chdir(Path(__file__).parent.parent)  # everything runs from the variant dir (CI/grpc)

from win_build_obs import check_call, CMAKE_VS_ARGS, spa, setup_obs
from win_shared import get_google_api_key_arg

TRIPLET = "x64-windows-static-md-release"


def main():
	root_dir = Path(os.getcwd())
	ci_root_dir = root_dir.joinpath("CI_build")
	build_deps_dir = ci_root_dir.joinpath("build_deps")
	ci_root_dir.mkdir(exist_ok = True)
	build_deps_dir.mkdir(exist_ok = True)

	# re-derive the OBS paths; skips the actual build via the done.txt sentinel
	obs_studio = build_deps_dir.joinpath("obs-studio")
	obs_studio_src, obs_deps_dir, build_installed_dir = setup_obs(obs_studio, clean_afterwards = False)

	# static paths produced by setup_deps.py
	vcpkg_dir = build_deps_dir.joinpath("vcpkg")
	vcpkg_prefix_path = vcpkg_dir.joinpath(rf"installed\{TRIPLET}")
	googleapis_dir = build_deps_dir.joinpath("googleapis")

	build_dir = ci_root_dir.joinpath("build")
	installed_dir = ci_root_dir.joinpath("installed")
	build_dir.mkdir(exist_ok = True)
	check_call([
		"cmake",
		*CMAKE_VS_ARGS,
		r"-DCMAKE_BUILD_TYPE=RelWithDebInfo",
		r"-DCMAKE_GENERATOR_PLATFORM=x64",
		r"-DSPEECH_API_GOOGLE_GRPC_V1=ON",
		"-DBUILD_SHARED_LIBS=ON",
		f"-DOBS_BUILD_DIR={str(build_installed_dir)}",
		f"-DOBS_DEPS_DIR={str(obs_deps_dir)}",
		f"-DGOOGLEAPIS_DIR={str(googleapis_dir)}",
		f"-DCMAKE_PREFIX_PATH={str(vcpkg_prefix_path)}",
		f"-DCMAKE_INSTALL_PREFIX:PATH={str(installed_dir)}",
		get_google_api_key_arg(),
		str(root_dir.parent.parent),
	], cwd = build_dir)
	check_call(spa("cmake --build . --config RelWithDebInfo"), cwd = build_dir)
	check_call(spa("cmake --install . --config RelWithDebInfo"), cwd = build_dir)

	if "--package" in sys.argv[1:]:
		check_call([sys.executable, str(Path(__file__).parent.joinpath("post_build.py"))])


if __name__ == '__main__':
	main()
