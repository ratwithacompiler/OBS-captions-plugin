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


def setup_vcpkg_grpc(target: Path, clean_afterwards: bool):
	triplet = "x64-windows-static-md-release"
	if not target.exists():
		check_call([*spa("git clone --single-branch --branch master https://github.com/Microsoft/vcpkg.git"), str(target)])
		check_call([*spa("git reset --hard 824c4324736156720645819abe4d48b4e740a1cd")], cwd = target)  # grpc 1.48.0
		check_call([*spa("cmd /C bootstrap-vcpkg.bat")], cwd = target)
	else:
		print("vcpkg dir exists", target)

	triplet_path = target.joinpath(rf"triplets\community\{triplet}.cmake")
	if not triplet_path.exists():
		src_triplet = target.joinpath(r"triplets\community\x64-windows-static-md.cmake")
		text = src_triplet.read_text()
		text = text + "\nset(VCPKG_BUILD_TYPE release)\n"
		triplet_path.write_text(text)
		print("created static release triplet", triplet_path)

	check_call([*spa(rf".\vcpkg.exe install  --host-triplet={triplet} --triplet={triplet} grpc:{triplet}")], cwd = target, shell = True)

	if clean_afterwards:
		for dirname in ["downloads", "packages", "buildtrees"]:
			dir = target.joinpath(dirname)
			if dir.exists():
				print("removing", dir)
				shutil.rmtree(dir)

	return triplet


def setup_googleapis(googleapis_cmake_script: Path, target: Path, vcpkg: Path, triplet: str):
	done_file = target.joinpath("done.txt")
	gens = target.joinpath("gens")
	if target.exists() and gens.exists() and done_file.exists():
		print("googleapis exists", target)
		return

	if not target.exists():
		check_call([*spa("git clone --single-branch --branch master https://github.com/googleapis/googleapis"), str(target)])
		check_call([*spa("git reset --hard 9f7c0ffdaa8ceb2f27982bad713a03306157a4d2")], cwd = str(target))

	protoc = vcpkg.joinpath(rf"installed\{triplet}\tools\protobuf\protoc.exe")
	grpc_cpp_path = vcpkg.joinpath(rf"installed\{triplet}\tools\grpc\grpc_cpp_plugin.exe")
	protoc_include = vcpkg.joinpath(rf"installed\{triplet}\include")

	assert googleapis_cmake_script.exists()

	gens.mkdir(exist_ok = True)
	check_call([
		"cmake",
		f"-DPROTOC_PATH={str(protoc)}",
		f"-DPROTOC_CPP_PATH={str(grpc_cpp_path)}",
		f"-DPROTO_INCLUDE_PATH={str(protoc_include)}",
		f"-DGOOGLEAPIS_PATH={str(target)}",
		"-P", str(googleapis_cmake_script),
	], cwd = target)

	done_file.write_text("yep")


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

	vcpkg_dir = build_deps_dir.joinpath("vcpkg")
	print("vcpkg", vcpkg_dir)

	CLEAN_VCPKG = os.environ.get("CLEAN_VCPKG")
	clean_afterwards = CLEAN_VCPKG in ("1", "true")
	print("CLEAN_VCPKG clean_afterwards", (CLEAN_VCPKG, clean_afterwards))
	triplet = setup_vcpkg_grpc(vcpkg_dir, clean_afterwards = clean_afterwards)
	vcpkg_prefix_path = vcpkg_dir.joinpath(rf"installed\{triplet}")

	googleapis_dir = build_deps_dir.joinpath("googleapis")
	googleapis_cmake_script = root_dir.joinpath("googleapis_CMakeLists.txt")
	print("googleapis_dir", googleapis_dir)
	setup_googleapis(googleapis_cmake_script, googleapis_dir, vcpkg_dir, triplet)

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

	release = ci_root_dir.joinpath("release")
	package_zip(release, installed_dir, version)


if __name__ == '__main__':
	main()
