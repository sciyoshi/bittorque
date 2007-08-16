import os

#########################
## pkg-config checking ##
#########################

def check_pkgconfig(context, version):
	context.Message('Checking for pkg-config... ')
	ret = context.TryAction('pkg-config --atleast-pkgconfig-version=%s' % version)[0]
	context.Result(ret)
	return ret

def check_pkg(context, name, version=None):
	if version:
		context.Message('Checking for %s >= %s... ' % (name, version))
		ret = context.TryAction('pkg-config --atleast-version=%s %s' % (version, name))[0]
		context.Result(ret)
		return ret
	else:
		context.Message('Checking for %s... ' % name)
		ret = context.TryAction('pkg-config --exists %s' % name)[0]
		context.Result(ret)
		return ret


#####################
## gtk-doc support ##
#####################

# TODO


#################
## main script ##
#################

SConsignFile(os.path.join('.scons', 'sconsign'))

opts = Options('SConfig', ARGUMENTS)

opts.AddOptions(
	BoolOption('enable_debug', 'Enable debug symbols', 1),
	BoolOption('embed_data', 'Embed icons and the glade file into the executable', 1),
	BoolOption('static_gnet', 'Statically link the GNet library', 1),
	PathOption('data_dir', 'Path to search for images and other data', Dir('#/data').abspath))

env = Environment(options=opts, tools=['mingw', 'disttar', 'default'], toolpath=['tools'], ENV=os.environ)

env.Help(opts.GenerateHelpText(env))

conf = env.Configure(
	conf_dir='.scons',
	log_file=os.path.join('.scons', 'log'),
	custom_tests={
		'CheckPKGConfig': check_pkgconfig,
		'CheckPKG': check_pkg})

if not conf.CheckPKGConfig('0.15.0'):
	print 'pkg-config >= 0.15.0 not found'
	Exit(1)

if not conf.CheckPKG('glib-2.0', '2.12.0'):
	print 'glib-2.0 >= 2.12.0 not found'
	Exit(1)

if not conf.CheckPKG('gobject-2.0', '2.12.0'):
	print 'gobject-2.0 >= 2.12.0 not found'
	Exit(1)

if not conf.CheckPKG('gthread-2.0', '2.12.0'):
	print 'gthread-2.0 >= 2.12.0 not found'
	Exit(1)

if not conf.CheckPKG('gmodule-2.0', '2.12.0'):
	print 'gmodule-2.0 >= 2.12.0 not found'
	Exit(1)

if not conf.CheckPKG('gtk+-2.0', '2.10.0'):
	print 'gtk+-2.0 >= 2.10.0 not found'
	Exit(1)

if not conf.CheckPKG('libglade-2.0', '2.4.0'):
	print 'libglade-2.0 >= 2.4.0 not found'
	Exit(1)

if not conf.CheckPKG('gnet-2.0'):
	print 'gnet-2.0 not found'
	Exit(1)

env = conf.Finish()

env.ParseConfig('pkg-config --cflags --libs glib-2.0 gobject-2.0 gthread-2.0')
env.ParseConfig('pkg-config --cflags gnet-2.0')
env.Append(CPPDEFINES=['GNET_EXPERIMENTAL'])

if env['CC'] == 'gcc':
	env.Append(CCFLAGS=['-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', '-O2'])

if not env['static_gnet']:
	env.ParseConfig('pkg-config --libs gnet-2.0')

if env['enable_debug']:
	if env['CC'] == 'gcc':
		env.Append(CCFLAGS=['-g'])
	else:
		env.Append(LINKFLAGS=['/DEBUG', '/PDB:bittorque.pdb'])

envgtk = env.Copy()
envgtk.ParseConfig('pkg-config --cflags --libs gtk+-2.0 libglade-2.0')
envgtk.Append(CPPDEFINES=
	['ENABLE_NLS',
	 r'GETTEXT_PACKAGE=\"bittorque\"',
	 r'BITTORQUE_WEBSITE=\"www.bittorque.org\"',
	 r'BITTORQUE_LOCALE_DIR=\"/home/sciyoshi/Projects/bittorque/data/\"',
	 r'BITTORQUE_DATA_DIR=\"/home/sciyoshi/Projects/bittorque/data/\"'])

if env['embed_data']:
	envgtk.Append(CPPDEFINES=['BITTORQUE_EMBED_DATA'])

envgtk.Append(LINKFLAGS=['-export-dynamic'])
envgtk['LIBS'].insert(0, 'bittorque')
envgtk.Append(LIBPATH=['#/src/lib'])
envgtk.Append(CPPPATH=['#/src/lib'])


if env['CC'] == 'gcc':
	if '/nologo' in env['CCFLAGS']:
		env['CCFLAGS'].remove('/nologo')
	if '/nologo' in envgtk['CCFLAGS']:
		envgtk['CCFLAGS'].remove('/nologo')

#env['CC'] = 'i586-mingw32msvc-gcc'
#envgtk['CC'] = 'i586-mingw32msvc-gcc'

if env['PLATFORM'] == 'win32':
	if 'm' in envgtk['LIBS']:
		envgtk['LIBS'].remove('m')

Export('env', 'envgtk')

SConscript(['src/SConscript', 'doc/SConscript'])

"""
env['DISTTAR_FORMAT'] = 'bz2'

env['DISTTAR_EXCLUDEDIRS'] = ['.svn', '.scons']
env['DISTTAR_EXCLUDEEXTS'] = ['.pyc']

env.DistTar('dist/bittorque', [
	'Authors',
	'Copying',
	'Install',
	'ReadMe',
	'SConfig',
	'SConstruct',
	env.Dir('data'),
	env.Dir('doc'),
	'src/SConscript',
	'src/gtk/SConscript',
	'src/gtk/bittorque.c',
	'src/gtk/bittorque.h',
	'src/gtk/bittorque-callbacks.c',
	'src/gtk/bittorque-ui.c',
	'src/gtk/bittorque-ui.h',
	'src/gtk/egg-editable-toolbar.c',
	'src/gtk/egg-editable-toolbar.h',
	'src/gtk/egg-marshallers.list',
	'src/gtk/egg-toolbar-editor.c',
	'src/gtk/egg-toolbar-editor.h',
	'src/gtk/egg-toolbars-model.c',
	'src/gtk/egg-toolbars-model.h',
	'src/lib/SConscript',
	'src/lib/bt-bencode.c',
	'src/lib/bt-bencode.h',
	'src/lib/bt-manager.c',
	'src/lib/bt-manager.h',
	'src/lib/bt-peer.c',
	'src/lib/bt-peer.h',
	'src/lib/bt-peer-encryption.c',
	'src/lib/bt-peer-encryption.h',
	'src/lib/bt-peer-extension.c',
	'src/lib/bt-peer-extension.h',
	'src/lib/bt-peer-protocol.c',
	'src/lib/bt-peer-protocol.h',
	'src/lib/bt-torrent.c',
	'src/lib/bt-torrent.h',
	'src/lib/bt-torrent-file.c',
	'src/lib/bt-torrent-file.h',
	'src/lib/bt-utils.c',
	'src/lib/bt-utils.h',
	'src/lib/rc4.c',
	'src/lib/rc4.h',
	'src/lib/sha1.c',
	'src/lib/sha1.h',
	env.Dir('tests'),
	env.Dir('tools')])
"""
