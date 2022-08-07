# import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

DEPS = "https://github.com/obsproject/obs-deps/releases/download/2022-08-02/windows-deps-2022-08-02-x64.zip"
DEPS_QT = "https://github.com/obsproject/obs-deps/releases/download/2022-08-02/windows-deps-qt6-2022-08-02-x64.zip"


def eprint(*args, **kwargs):
	print(*args, **kwargs, file = sys.stderr)


CMAKE_VS_ARGS = ["-G", "Visual Studio 17 2022", "-A", "x64"]


def spa(line: str):
	args = line.split(" ")
	return [i for i in args if i]


def check_call(args: list, cwd = None, shell = False):
	cwd = str(cwd) if cwd else None
	print(f"calling {args !r} in cwd: {cwd !r}, shell={shell}")
	sys.stdout.flush()
	return subprocess.check_call(args, cwd = cwd, shell = shell)


def download(url: str, target_path: Path):
	check_call([*spa("curl -L -f --retry 10 -o"), str(target_path), url])


def unzip(zipfile: Path, target_path: Path):
	check_call(["7z", "x", str(zipfile), f"-o{str(target_path)}"])


def setup_obs(obs_studio: Path, clean_afterwards: bool):
	obs_deps_dir = obs_studio.joinpath("obs_deps_dir")
	obs_deps_unpacked = obs_deps_dir.joinpath("obs_deps_unpacked")
	obs_studio_src = obs_studio.joinpath("src")
	build_dir = obs_studio_src.joinpath("build")
	build_installed_dir = obs_studio_src.joinpath("build_installed")
	done_file = obs_studio_src.joinpath("done.txt")

	if obs_studio_src.exists() and build_installed_dir.exists() and obs_deps_dir.exists() and done_file.exists():
		print("obs-studio src exists already, skipping", obs_studio_src)
		patch_w32(build_installed_dir)
		return obs_studio_src, obs_deps_unpacked, build_installed_dir

	obs_deps_dir.mkdir(parents = True, exist_ok = True)
	deps_zip = obs_deps_dir.joinpath("deps.zip")
	if not deps_zip.exists():
		print("downloading deps", deps_zip)
		download(DEPS, deps_zip)

	deps_qt_zip = obs_deps_dir.joinpath("deps_qt.zip")
	if not deps_qt_zip.exists():
		print("downloading deps qt", deps_qt_zip)
		download(DEPS_QT, deps_qt_zip)

	if not obs_deps_unpacked.exists():
		unzip(deps_zip, obs_deps_unpacked)
		unzip(deps_qt_zip, obs_deps_unpacked)

	print("deps", os.listdir(obs_deps_unpacked))
	print("setting up OBS build/source in", obs_studio_src)

	if not obs_studio_src.exists():
		check_call([*spa("git clone --single-branch --branch master https://github.com/obsproject/obs-studio.git"), str(obs_studio_src)])
		check_call([*spa("git checkout 28.0.0-beta1")], cwd = obs_studio_src)
		check_call([*spa("git submodule update --init --recursive")], cwd = obs_studio_src)

	build_dir.mkdir(True, exist_ok = True)
	print("generating")

	check_call([
		"cmake", "..",
		*CMAKE_VS_ARGS,
		r"-DENABLE_BROWSER=OFF",
		r"-DENABLE_VLC=OFF",
		r"-DENABLE_SCRIPTING=OFF",
		r"-DENABLE_UI=OFF",
		r"-DENABLE_PLUGINS=OFF",
		r"-DQT_VERSION=6",
		r"-DCMAKE_BUILD_TYPE=RelWithDebInfo",
		r"-DCMAKE_GENERATOR_PLATFORM=x64",
		f"-DCMAKE_PREFIX_PATH={str(obs_deps_unpacked)}",
		f"-DCMAKE_INSTALL_PREFIX:PATH={str(build_installed_dir)}",
	], cwd = build_dir)

	print("building")
	check_call(spa("cmake --build . --config RelWithDebInfo -t obs-frontend-api"), cwd = build_dir)
	check_call(spa("cmake --install . --config RelWithDebInfo --component obs_libraries"), cwd = build_dir)

	patch_w32(build_installed_dir)

	if clean_afterwards:
		if build_dir.exists():
			print("removing", build_dir)
			shutil.rmtree(build_dir)

	done_file.write_text("yep")
	return obs_studio_src, obs_deps_unpacked, build_installed_dir


def patch_w32(installed_dir: Path):
	target_file = installed_dir.joinpath("cmake\\libobsTargets.cmake")
	full_contents = target_file.read_text()
	new_contents = full_contents
	ok = False
	for pos, (line, new_line) in enumerate((
			(
					r"""if(DEFINED ${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE_targets)""",
					r"""if(0) #ignore"""
			),
			(
					r"""message(FATAL_ERROR "The imported target \"${target}\" references the file"""
					, r"""message(WARNING "The imported target \"${target}\" references the file"""
			)
	)):
		if new_line in new_contents:
			print("ignoring already patched libobsTargets.cmake", target_file)
			ok = True
		elif line in new_contents:
			print(f"patching line {line !r} -> {new_line !r}")
			new_contents = new_contents.replace(line, new_line)
			ok = True

	if not ok:
		raise ValueError("invalid libobsTargets.cmake file", full_contents)

	target_file.write_text(new_contents)
	print("patched libobsTargets.cmake", target_file)


def get_google_api_key_arg():
	google_api_key = os.environ.get("GOOGLE_API_KEY")
	if google_api_key == "$(GOOGLE_API_KEY)":
		google_api_key = None
		eprint("mkdir ignoring azure env arg", google_api_key)

	if google_api_key:
		print("building with hardcoded API key", len(google_api_key))
		google_api_key_arg = f"-DGOOGLE_API_KEY={google_api_key}"
	else:
		print("building with UI for user provided API key")
		google_api_key_arg = "-DENABLE_CUSTOM_API_KEY=ON"

	return google_api_key_arg


def package_zip(release: Path, build_dir: Path, version: str):
	release.mkdir(parents = True, exist_ok = True)

	obs_plugin = release.joinpath(rf"Closed_Captions_Plugin__v{version}_Windows\obs-plugins")
	print(f"obs_plugin plugin path: {obs_plugin}")
	obs_plugin.mkdir(parents = True, exist_ok = True)

	obs_plugin_64bit = obs_plugin.joinpath("64bit")
	obs_plugin_64bit.mkdir(exist_ok = True)

	plugin_dll = build_dir.joinpath(r"RelWithDebInfo\obs_google_caption_plugin.dll")
	print(f"copying  {plugin_dll!r} -> {obs_plugin_64bit!r}")
	shutil.copy(plugin_dll, obs_plugin_64bit)

	check_call(["7z", "a", "-r", release.joinpath(rf"Closed_Captions_Plugin__v{version}_Windows.zip"), obs_plugin.parent])
	check_call(["7z", "a", "-r", release.joinpath(rf"Closed_Captions_Plugin__v{version}_Windows_plugins.zip"), obs_plugin])
