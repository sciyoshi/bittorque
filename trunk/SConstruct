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

env = Environment(options=opts, tools=['mingw', 'default'], ENV=os.environ)

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

if not conf.CheckPKG('libglade-2.0', '2.6.0'):
	print 'libglade-2.0 >= 2.6.0 not found'
	Exit(1)

if not conf.CheckPKG('gnet-2.0'):
	print 'gnet-2.0 not found'
	Exit(1)

env = conf.Finish()

env.ParseConfig('pkg-config --cflags --libs glib-2.0 gobject-2.0 gthread-2.0')
env.ParseConfig('pkg-config --cflags --libs gnet-2.0')
env.Append(CPPDEFINES=['-DGNET_EXPERIMENTAL'])
env.Append(CCFLAGS=['-Wall', '-Wextra', '-Werror', '-O0', '-g'])

envgtk = env.Copy()
envgtk.ParseConfig('pkg-config --cflags --libs gtk+-2.0 libglade-2.0')
envgtk.Append(CPPDEFINES=
	['-DENABLE_NLS',
	 '-DGETTEXT_PACKAGE=\\"bittorque\\"',
	 '-DBITTORQUE_WEBSITE=\\"www.bittorque.org\\"',
	 '-DBITTORQUE_LOCALE_DIR=\\"/home/sciyoshi/Projects/bittorque/build/linux/locale\\"',
	 '-DBITTORQUE_DATA_DIR=\\"/home/sciyoshi/Projects/bittorque/data\\"'])
envgtk.Append(LINKFLAGS=['-export-dynamic'])
envgtk.Append(LIBS=['bittorque'])
envgtk.Append(LIBPATH=['#/src/lib'])
envgtk.Append(CPPPATH=['#/src/lib'])

Export('env', 'envgtk')

SConscript(['src/SConscript', 'doc/SConscript'])
