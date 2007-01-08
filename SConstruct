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
		ret = context.TryAction('pkg-config --atleast-version=%s \'%s\'' % (version, name))[0]
		context.Result(ret)
		return ret
	else:
		context.Message('Checking for %s... ' % name)
		ret = context.TryAction('pkg-config --exists \'%s\'' % name)[0]
		context.Result(ret)
		return ret


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
env.ParseConfig('pkg-config --cflags gnet-2.0')
if not env['static_gnet']:
	env.ParseConfig('pkg-config --libs gnet-2.0')
env.Append(CPPDEFINES=['-DBT_DATA_DIR=\\"' + env['data_dir'] + '\\"'])
env.Append(CPPDEFINES=['-DGNET_EXPERIMENTAL', '-DGETTEXT_PACKAGE=\\"bittorque\\"', '-DBTLOCALEDIR=\\"/usr/local/share/locale\\"'])
env.Append(CCFLAGS=['-Wall', '-Wextra'])
if env['enable_debug']:
	env.Append(CCFLAGS=['-g'])

envgtk = env.Copy()
envgtk.ParseConfig('pkg-config --cflags --libs gtk+-2.0 gdk-pixbuf-2.0 libglade-2.0')
envgtk.Append(CPPPATH=['#/src/lib'])
envgtk.Append(LIBS=['bittorque'])
envgtk.Append(LIBPATH=['#/src/lib'])
envgtk.Append(LINKFLAGS=['-Wl,--export-dynamic'])
if envgtk['embed_data']:
	envgtk.Append(CPPDEFINES=['-DBT_EMBED_DATA'])

envlib = env.Copy()
envlib.Append(CPPDEFINES=['-DG_LOG_DOMAIN=\\"BitTorque\\"'])

envdoc = env.Copy()
envdoc.Append(CPPPATH=['#/src/lib'])
envdoc.Append(LIBS=['bittorque'])
envdoc.Append(LINKFLAGS=['-L#/src/lib'])

Export('env', 'envlib', 'envdoc', 'envgtk')

SConscript(['src/SConscript', 'docs/SConscript'])
