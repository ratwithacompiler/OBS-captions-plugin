import os
import shutil
import sys
import subprocess

from pathlib import Path


def spa(line: str):
	args = line.split(" ")
	return [i for i in args if i]


def download(url: str, target_path: Path):
	check_call([*spa("curl -L -f --retry 10 -o"), str(target_path), url])


def unzip(zipfile: Path, target_path: Path):
	check_call(["7z", "x", str(zipfile), f"-o{str(target_path)}"])


def check_call(args: list, cwd = None, shell = False):
	cwd = str(cwd) if cwd else None
	print(f"calling {args !r} in cwd: {cwd !r}, shell={shell}")
	sys.stdout.flush()
	return subprocess.check_call(args, cwd = cwd, shell = shell)


def eprint(*args, **kwargs):
	print(*args, **kwargs, file = sys.stderr)


def package_zip(release: Path, installed_dir: Path, version: str):
	release.mkdir(parents = True, exist_ok = True)

	obs_plugin = release.joinpath(rf"Closed_Captions_Plugin__v{version}_Windows\obs-plugins")
	print(f"obs_plugin plugin path: {obs_plugin}")
	obs_plugin.mkdir(parents = True, exist_ok = True)

	obs_plugin_64bit = obs_plugin.joinpath("64bit")
	obs_plugin_64bit.mkdir(exist_ok = True)

	plugin_dll = installed_dir.joinpath(r"lib\obs_google_caption_plugin.dll")
	print(f"copying  {plugin_dll!r} -> {obs_plugin_64bit!r}")
	shutil.copy(plugin_dll, obs_plugin_64bit)

	check_call(["7z", "a", "-r", release.joinpath(rf"Closed_Captions_Plugin__v{version}_Windows.zip"), obs_plugin.parent])
	check_call(["7z", "a", "-r", release.joinpath(rf"Closed_Captions_Plugin__v{version}_Windows_plugins.zip"), obs_plugin])


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
