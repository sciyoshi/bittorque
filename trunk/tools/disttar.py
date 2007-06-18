import os
import sys
import stat

def disttar_emitter(target, source, env):
	source, origsource = [], source

	excludeexts = env.Dictionary().get('DISTTAR_EXCLUDEEXTS', [])
	excludedirs = env.Dictionary().get('DISTTAR_EXCLUDEDIRS', [])

	for item in origsource:
		if stat.S_ISREG(os.stat(str(item))[stat.ST_MODE]):
			source.append(str(item))
		for root, dirs, files in os.walk(str(item)):
			if root in source:
				source.remove(root)

			for name in files:
				ext = os.path.splitext(name)
				if not ext[1] in excludeexts:
					relpath = os.path.join(root, name)
					source.append(relpath)
			for d in excludedirs:
				if d in dirs:
					dirs.remove(d)
	return target, source

def disttar_string(target, source, env):
	return 'Creating archive %s' % str(target[0])

def disttar(target, source, env):
	import tarfile

	env_dict = env.Dictionary()

	if env_dict.get("DISTTAR_FORMAT") in ["gz", "bz2"]:
		tar_format = env_dict["DISTTAR_FORMAT"]
	else:
		tar_format = ""

	base_name = str(target[0]).split('.tar')[0]
	target_dir, dir_name = os.path.split(base_name)

	if target_dir and not os.path.exists(target_dir):
		os.makedirs(target_dir)

	tar = tarfile.open(str(target[0]), "w:%s" % (tar_format,))

	for item in source:
		item = str(item)
		tar.add(item,'%s/%s' % (dir_name,item))

	tar.close()

def disttar_suffix(env, sources):
	env_dict = env.Dictionary()
	if env_dict.has_key("DISTTAR_FORMAT") and env_dict["DISTTAR_FORMAT"] in ["gz", "bz2"]:
		return ".tar." + env_dict["DISTTAR_FORMAT"]
	else:
		return ".tar"

def generate(env):
	disttar_action = env.Action(disttar, disttar_string)

	env['BUILDERS']['DistTar'] = env.Builder(
		action=disttar_action,
		emitter=disttar_emitter,
		suffix=disttar_suffix,
		target_factory=env.fs.Entry)

	env.AppendUnique(DISTTAR_FORMAT='gz')

def exists(env):
	try:
		import os
		import tarfile
	except ImportError:
		return False
	else:
		return True

