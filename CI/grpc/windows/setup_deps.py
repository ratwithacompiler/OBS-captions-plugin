import os
import shutil
import sys
from pathlib import Path

sys.path.append(str(Path(__file__).parent.parent.parent))
os.chdir(Path(__file__).parent.parent)  # everything runs from the variant dir (CI/grpc)

from win_build_obs import check_call, spa, setup_obs

TRIPLET = "x64-windows-static-md-release"


def setup_vcpkg_grpc(target: Path, clean_afterwards: bool):
	triplet = TRIPLET
	if not target.exists():
		check_call([*spa("git clone --single-branch --branch master https://github.com/Microsoft/vcpkg.git"), str(target)])
		check_call([*spa("git reset --hard 9e593bb18ea69cc5095e012465dcd675a822ed0d")], cwd = target)  # grpc 1.81.1, vcpkg release 2026.07.29
		check_call([*spa("cmd /C bootstrap-vcpkg.bat")], cwd = target)
	else:
		print("vcpkg dir exists", target)

	triplet_path = target.joinpath(rf"triplets\community\{triplet}.cmake")
	if not triplet_path.exists():
		src_triplet = target.joinpath(r"triplets\x64-windows-static-md.cmake")
		text = src_triplet.read_text()
		text = text + "\nset(VCPKG_BUILD_TYPE release)\n"
		triplet_path.write_text(text)
		print("created static release triplet", triplet_path)

	# keep vcpkg's buildtrees at a short path when requested (set in CI):
	# the github runner workspace prefix is long enough that grpc's deepest
	# generated sources exceed windows MAX_PATH (260) otherwise
	bt_root = os.environ.get("VCPKG_BUILDTREES_ROOT")
	bt_arg = f" --x-buildtrees-root={bt_root}" if bt_root else ""

	check_call([*spa(rf".\vcpkg.exe install{bt_arg} --host-triplet={triplet} --triplet={triplet} grpc:{triplet}")], cwd = target, shell = True)

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

	obs_studio = build_deps_dir.joinpath("obs-studio")
	CLEAN_OBS = os.environ.get("CLEAN_OBS", "1")  # clean by default to keep size small
	clean_afterwards = CLEAN_OBS in ("1", "true")
	print("CLEAN_OBS clean_afterwards", (CLEAN_OBS, clean_afterwards))
	setup_obs(obs_studio, clean_afterwards = clean_afterwards)

	vcpkg_dir = build_deps_dir.joinpath("vcpkg")
	print("vcpkg", vcpkg_dir)

	CLEAN_VCPKG = os.environ.get("CLEAN_VCPKG", "1")  # clean by default to keep size small
	clean_afterwards = CLEAN_VCPKG in ("1", "true")
	print("CLEAN_VCPKG clean_afterwards", (CLEAN_VCPKG, clean_afterwards))
	triplet = setup_vcpkg_grpc(vcpkg_dir, clean_afterwards = clean_afterwards)

	googleapis_dir = build_deps_dir.joinpath("googleapis")
	googleapis_cmake_script = root_dir.joinpath("googleapis_CMakeLists.txt")
	print("googleapis_dir", googleapis_dir)
	setup_googleapis(googleapis_cmake_script, googleapis_dir, vcpkg_dir, triplet)


if __name__ == '__main__':
	main()
