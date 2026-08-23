# import argparse
import os
import shutil
import stat
import sys
from pathlib import Path

from win_shared import check_call, download, unzip, spa

DEPS = "https://github.com/obsproject/obs-deps/releases/download/2025-08-23/windows-deps-2025-08-23-x64.zip"
DEPS_QT = "https://github.com/obsproject/obs-deps/releases/download/2025-08-23/windows-deps-qt6-2025-08-23-x64.zip"

CMAKE_VS_ARGS = ["-G", "Visual Studio 17 2022", "-A", "x64"]


def rmtree_force(path: Path):
	# git checkouts contain read-only files (.git/objects/pack/*) which plain
	# rmtree refuses to delete on windows - clear the bit and retry
	def _make_writable_and_retry(func, p, _exc):
		os.chmod(p, stat.S_IWRITE)
		func(p)

	if sys.version_info >= (3, 12):
		shutil.rmtree(path, onexc = _make_writable_and_retry)
	else:
		shutil.rmtree(path, onerror = _make_writable_and_retry)


def setup_obs(obs_studio: Path, clean_afterwards: bool):
	obs_deps_dir = obs_studio.joinpath("obs_deps_dir")
	obs_deps_unpacked = obs_deps_dir.joinpath("obs_deps_unpacked")
	obs_studio_src = obs_studio.joinpath("src")
	build_dir = obs_studio_src.joinpath("build")
	# install dir and sentinel live at the obs-studio level (not inside src/) so
	# src/ and the deps zips can be cleaned away after install
	build_installed_dir = obs_studio.joinpath("build_installed")
	done_file = obs_studio.joinpath("done.txt")

	if done_file.exists():
		print("obs build done, skipping", obs_studio)
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
		# shallow tag clone; submodules are plugin-only (browser/websocket/dshow)
		# and never reached with ENABLE_PLUGINS=OFF
		check_call([*spa("git clone --depth 1 --branch 32.0.0 https://github.com/obsproject/obs-studio.git"), str(obs_studio_src)])

	build_dir.mkdir(True, exist_ok = True)
	print("generating")

	check_call([
		"cmake", "..",
		*CMAKE_VS_ARGS,
		r"-DENABLE_PLUGINS:BOOL=OFF",
		r"-DENABLE_FRONTEND:BOOL=OFF",
		r"-DENABLE_SCRIPTING:BOOL=OFF",
		r"-DOBS_VERSION_OVERRIDE:STRING=32.0.0",
		r"-DCMAKE_BUILD_TYPE=RelWithDebInfo",
		r"-DCMAKE_GENERATOR_PLATFORM=x64",
		f"-DCMAKE_PREFIX_PATH={str(obs_deps_unpacked)}",
		f"-DCMAKE_INSTALL_PREFIX:PATH={str(build_installed_dir)}",
	], cwd = build_dir)

	print("building")
	check_call(spa("cmake --build . --config RelWithDebInfo -t obs-frontend-api"), cwd = build_dir)
	check_call(spa("cmake --install . --config RelWithDebInfo --component Development"), cwd = build_dir)

	if clean_afterwards:
		# src/ (source + build tree) and the deps zips are only needed to build
		# OBS itself; the plugin builds against build_installed/ plus
		# obs_deps_unpacked/ (Qt), which survive together with the sentinel
		if obs_studio_src.exists():
			print("removing", obs_studio_src)
			rmtree_force(obs_studio_src)
		for zip_file in (deps_zip, deps_qt_zip):
			if zip_file.exists():
				print("removing", zip_file)
				zip_file.unlink()

	done_file.write_text("yep")
	return obs_studio_src, obs_deps_unpacked, build_installed_dir
